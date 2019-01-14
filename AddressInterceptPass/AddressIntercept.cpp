#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/Support/Debug.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"

#include "llvm/Analysis/ValueTracking.h"

#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include <cassert>


using namespace llvm;

#define DEBUG_TYPE "AddressInterceptPass"

namespace {

  struct AddressInterceptPass : public FunctionPass {
    static char ID;
    AddressInterceptPass() : FunctionPass(ID) {}

    Type *IntptrTy;
    Function *CalleeF;
    Value *MemmoveFn;
    Value *MemLoadFn;

    bool doInitialization(Module &M) override {
        DEBUG(dbgs() << "Init " << M.getName() << "\n");
          auto &DL = M.getDataLayout();

          LLVMContext *C;
          C = &(M.getContext());
          IRBuilder<> IRB(*C);
          IntptrTy = IRB.getIntPtrTy(DL);

          MemmoveFn = M.getOrInsertFunction("__store__", IRB.getVoidTy(), IRB.getInt8PtrTy(), IRB.getInt32Ty(),
                                            IRB.getInt32Ty(), IRB.getInt32Ty());

          //MemmoveFn = M.getOrInsertFunction("__test__", IRB.getVoidTy(),IRB.getInt32Ty(), IRB.getInt32Ty());

          MemLoadFn = M.getOrInsertFunction("__load__", IRB.getInt32Ty(), IRB.getInt8PtrTy(),
                                            IRB.getInt32Ty(), IRB.getInt32Ty());

          assert(ProcessedAllocas.empty() &&
                       "last pass forgot to clear cache");
    }

    virtual bool runOnFunction(Function &F) {
      dbgs() << "Function: " << F.getName() << "\n";

      markEscapedLocalAllocas(F);

      bool Changed = false;
      SmallVector<Instruction*, 16> ToInstrument;

      for (auto &BB : F) {
          for (auto &Inst : BB) {
            Value *MaybeMask = nullptr;
            Value * ValueIns;
            bool IsWrite;
            unsigned Alignment;
            uint64_t TypeSize;
            Value *Addr = isInterestingMemoryAccess(&Inst, &ValueIns, &IsWrite, &TypeSize,
                                                    &Alignment, &MaybeMask);
            if (Addr || isa<MemIntrinsic>(Inst)){
                dbgs() << Inst << " | IsWrite  " << IsWrite << "!!!!!!!!!!!!!\n";

                ToInstrument.push_back(&Inst);
            }
        }
      }

      dbgs() << "ToInstrument : " << ToInstrument.size() << "\n";


      for (auto Inst : ToInstrument){
          Changed |= instrumentMemAccessWrite(Inst)
              ||
              instrumentMemAccessRead(Inst);
      }

      ProcessedAllocas.clear();

      return false;
    }

    Value * isInterestingMemoryAccess(Instruction *I,
                                      Value ** ValueIns,
                                                       bool *IsWrite,
                                                       uint64_t *TypeSize,
                                                       unsigned *Alignment,
                                                       Value **MaybeMask) {

      Value *PtrOperand = nullptr;
      Value *PtrValue = nullptr;
      const DataLayout &DL = I->getModule()->getDataLayout();

      if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
        *IsWrite = false;
        *TypeSize = DL.getTypeStoreSizeInBits(LI->getType());
        *Alignment = LI->getAlignment();
        PtrOperand = LI->getPointerOperand();
        dbgs() << "LoadInst "   << "\n";
      }
      else
          if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
        *IsWrite = true;
        *TypeSize = DL.getTypeStoreSizeInBits(SI->getValueOperand()->getType());
        *Alignment = SI->getAlignment();
        PtrOperand = SI->getPointerOperand();
        *ValueIns = SI->getValueOperand();
        dbgs() << "StoreInst "  << *ValueIns << "\n";
      }
#if 0
      else if (AtomicRMWInst *RMW = dyn_cast<AtomicRMWInst>(I)) {
        *IsWrite = true;
        *TypeSize = DL.getTypeStoreSizeInBits(RMW->getValOperand()->getType());
        *Alignment = 0;
        PtrOperand = RMW->getPointerOperand();
      } else if (AtomicCmpXchgInst *XCHG = dyn_cast<AtomicCmpXchgInst>(I)) {
        *IsWrite = true;
        *TypeSize = DL.getTypeStoreSizeInBits(XCHG->getCompareOperand()->getType());
        *Alignment = 0;
        PtrOperand = XCHG->getPointerOperand();
      }
#endif
      if (PtrOperand) {
        // Do not instrument acesses from different address spaces; we cannot deal
        // with them.
        Type *PtrTy = cast<PointerType>(PtrOperand->getType()->getScalarType());
        if (PtrTy->getPointerAddressSpace() != 0)
          return nullptr;

        // Ignore swifterror addresses.
        // swifterror memory addresses are mem2reg promoted by instruction
        // selection. As such they cannot have regular uses like an instrumentation
        // function and it makes no sense to track them as memory.
        if (PtrOperand->isSwiftError())
          return nullptr;
      }

      if (auto AI = dyn_cast_or_null<AllocaInst>(PtrOperand))
          if(isProbablyNotAlloca(*AI) == false){
               dbgs() << " ###########AI: " << *AI << "\n";
                return nullptr;
          }

      //ValueIns = &PtrValue;

      return PtrOperand;
    }

    // Accesses sizes are powers of two: 1, 2, 4, 8, 16.
    static const size_t kNumberOfAccessSizes = 5;
    // 1 << 4 ... 16
    static const size_t kShadowScale = 4;

    static size_t TypeSizeToSizeIndex(uint32_t TypeSize) {
      size_t Res = countTrailingZeros(TypeSize / 8);
      assert(Res < kNumberOfAccessSizes);
      return Res;
    }

    bool instrumentMemAccessRead(Instruction *I) {

        dbgs() << "instrumentMemAccessRead: " << *I << "\n";
        bool IsWrite = false;
        unsigned Alignment = 0;
        uint64_t TypeSize = 0;
        Value *MaybeMask = nullptr;
        Value * ValueIns = nullptr;
        Value *Addr =
            isInterestingMemoryAccess(I, &ValueIns, &IsWrite, &TypeSize, &Alignment, &MaybeMask);

        if (!Addr)
          return false;

        if (MaybeMask)
          return false; //FIXME

        if(IsWrite)
            return false;

         IRBuilder<> IRB(I);

        if (ValueIns){
            dbgs() << "ValueIns   " << *ValueIns <<  "\n";
        }

        Value *AddrLong = IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy());

        const bool isPowerOf2 = isPowerOf2_64(TypeSize);
        const bool typeLessThan_16s8 = (TypeSize / 8 <= (1UL << (kNumberOfAccessSizes - 1)));
        const bool AlignmentNormilize = (Alignment >= (1UL << kShadowScale) || Alignment == 0 ||
         Alignment >= TypeSize / 8);

        if (isPowerOf2_64(TypeSize) &&
              (TypeSize / 8 <= (1UL << (kNumberOfAccessSizes - 1))) &&
              (Alignment >= (1UL << kShadowScale) || Alignment == 0 ||
               Alignment >= TypeSize / 8)) {
            size_t AccessSizeIndex = TypeSizeToSizeIndex(TypeSize);

        dbgs() << *AddrLong << "   " << AddrLong->getType() <<
                      " |AccessSizeIndex  " << AccessSizeIndex << " |isPowerOf2  " << isPowerOf2 << " |typeLessThan_16s8  "
                   << typeLessThan_16s8 << " |AlignmentNormilize " << AlignmentNormilize << "\n";

            std::vector<Value *> ArgsV;
            ArgsV.push_back(AddrLong);

            ConstantInt * TypeSizeArg = IRB.getInt32(TypeSize);
            ArgsV.push_back(TypeSizeArg);

            ConstantInt * AlignmentArg = IRB.getInt32(Alignment);
             ArgsV.push_back(AlignmentArg);

            Instruction * rep = IRB.CreateCall(MemLoadFn,ArgsV);

            Type * typeLoadOperand = rep->getType();

            switch (TypeSize) {
            case 1:
                typeLoadOperand = IRB.getInt1Ty();
                break;
            case 8:
                typeLoadOperand = IRB.getInt8Ty();
                break;
            case 16:
                typeLoadOperand = IRB.getInt16Ty();
                break;
            case 32:
                break;
            default:
                llvm_unreachable("Size issue!");
                break;
            }


            Value * trunc = IRB.CreateTrunc(rep, typeLoadOperand);

            I->replaceAllUsesWith(trunc);

            I->eraseFromParent();

        }

        return true;
    }

    bool instrumentMemAccessWrite(Instruction *I) {
      dbgs() << "instrumentMemAccessWrite: " << *I << "\n";
      bool IsWrite = false;
      unsigned Alignment = 0;
      uint64_t TypeSize = 0;
      Value *MaybeMask = nullptr;
      Value * ValueIns = nullptr;
      Value *Addr =
          isInterestingMemoryAccess(I, &ValueIns, &IsWrite, &TypeSize, &Alignment, &MaybeMask);

      if (!Addr)
        return false;

      if (MaybeMask)
        return false; //FIXME

      if(IsWrite == false)
          return false;

      IRBuilder<> IRB(I);
#if 1
      if (ValueIns){
          dbgs() << "ValueIns   " << *ValueIns <<  "\n";
      }
#endif

      Value *AddrLong = IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy());

      Value *ValueLong = IRB.CreateIntCast(ValueIns, IRB.getInt32Ty(), true);


      if (ValueLong){
          dbgs() << "ValueLong   " << *ValueLong << "   getType  " << *ValueIns->getType() <<   "\n";

      }

      const bool isPowerOf2 = isPowerOf2_64(TypeSize);
      const bool typeLessThan_16s8 = (TypeSize / 8 <= (1UL << (kNumberOfAccessSizes - 1)));
      const bool AlignmentNormilize = (Alignment >= (1UL << kShadowScale) || Alignment == 0 ||
       Alignment >= TypeSize / 8);

      if (isPowerOf2_64(TypeSize) &&
            (TypeSize / 8 <= (1UL << (kNumberOfAccessSizes - 1))) &&
            (Alignment >= (1UL << kShadowScale) || Alignment == 0 ||
             Alignment >= TypeSize / 8)) {
          size_t AccessSizeIndex = TypeSizeToSizeIndex(TypeSize);

          dbgs() <<
                    " |AccessSizeIndex  " << AccessSizeIndex << " |isPowerOf2  " << isPowerOf2 << " |typeLessThan_16s8  "
                 << typeLessThan_16s8 << " |AlignmentNormilize " << AlignmentNormilize << "\n";

          std::vector<Value *> ArgsV;

          #if 1
          ArgsV.push_back(AddrLong);

          ArgsV.push_back(ValueLong);
#endif

          ConstantInt * TypeSizeArg = IRB.getInt32(TypeSize);
          ArgsV.push_back(TypeSizeArg);

          ConstantInt * AlignmentArg = IRB.getInt32(Alignment);
           ArgsV.push_back(AlignmentArg);

          IRB.CreateCall(MemmoveFn,ArgsV);

          I->eraseFromParent();


      } else {
          dbgs() << "CreateCall" << " | "  << " | " << isPowerOf2 << " | "
                 << typeLessThan_16s8 << " | " << typeLessThan_16s8 << " | " << AlignmentNormilize << "\n";
      }

      return true;
    }

    Function *callback;

    DenseMap<const AllocaInst *, bool> ProcessedAllocas;

    /// Check if we want (and can) handle this alloca.
    bool isProbablyNotAlloca(const AllocaInst &AI) {
      auto PreviouslySeenAllocaInfo = ProcessedAllocas.find(&AI);

      if (PreviouslySeenAllocaInfo != ProcessedAllocas.end())
        return PreviouslySeenAllocaInfo->getSecond();
      bool IsInteresting =
          (AI.getAllocatedType()->isSized() &&
           // alloca() may be called with 0 size, ignore it.
           ((!AI.isStaticAlloca()) || getAllocaSizeInBytes(AI) > 0) &&
           // We are only interested in allocas not promotable to registers.
           // Promotable allocas are common under -O0.
           (!isAllocaPromotable(&AI)) &&
           // inalloca allocas are not treated as static, and we don't want
           // dynamic alloca instrumentation for them as well.
           !AI.isUsedWithInAlloca() &&
           // swifterror allocas are register promoted by ISel
           !AI.isSwiftError());

      ProcessedAllocas[&AI] = IsInteresting;
      return IsInteresting;
    }

    uint64_t getAllocaSizeInBytes(const AllocaInst &AI) const {
        uint64_t ArraySize = 1;
        if (AI.isArrayAllocation()) {
          const ConstantInt *CI = dyn_cast<ConstantInt>(AI.getArraySize());
          assert(CI && "non-constant array size");
          ArraySize = CI->getZExtValue();
        }
        Type *Ty = AI.getAllocatedType();
        uint64_t SizeInBytes =
            AI.getModule()->getDataLayout().getTypeAllocSize(Ty);
        return SizeInBytes * ArraySize;
      }

    void markEscapedLocalAllocas(Function &F) {
      // Find the one possible call to llvm.localescape and pre-mark allocas passed
      // to it as uninteresting. This assumes we haven't started processing allocas
      // yet. This check is done up front because iterating the use list in
      // isInterestingAlloca would be algorithmically slower.
      dbgs() << "ProcessedAllocas.size() = " << ProcessedAllocas.size() << "\n";
      assert(ProcessedAllocas.empty() && "must process localescape before allocas");

      // Try to get the declaration of llvm.localescape. If it's not in the module,
      // we can exit early.
      if (!F.getParent()->getFunction("llvm.localescape")) return;

      // Look for a call to llvm.localescape call in the entry block. It can't be in
      // any other block.
      for (Instruction &I : F.getEntryBlock()) {
        IntrinsicInst *II = dyn_cast<IntrinsicInst>(&I);
        if (II && II->getIntrinsicID() == Intrinsic::localescape) {
          // We found a call. Mark all the allocas passed in as uninteresting.
          for (Value *Arg : II->arg_operands()) {
            AllocaInst *AI = dyn_cast<AllocaInst>(Arg->stripPointerCasts());
            assert(AI && AI->isStaticAlloca() &&
                   "non-static alloca arg to localescape");
            ProcessedAllocas[AI] = false;
          }
          break;
        }
      }
    }

  };
}

char AddressInterceptPass::ID = 0;

// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerAddressInterceptPass(const PassManagerBuilder &,
                         legacy::PassManagerBase &PM) {
  PM.add(new AddressInterceptPass());
}
static RegisterStandardPasses
  RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                 registerAddressInterceptPass);

