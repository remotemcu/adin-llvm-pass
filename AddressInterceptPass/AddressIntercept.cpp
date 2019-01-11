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

using namespace llvm;

#define DEBUG_TYPE "AddressInterceptPass"

namespace {
  struct AddressInterceptPass : public FunctionPass {
    static char ID;
    AddressInterceptPass() : FunctionPass(ID) {}

    Type *IntptrTy;
    Function *CalleeF;
    Value *MemmoveFn;

    bool doInitialization(Module &M) override {
        DEBUG(dbgs() << "Init " << M.getName() << "\n");
          auto &DL = M.getDataLayout();

          LLVMContext *C;
          C = &(M.getContext());
          IRBuilder<> IRB(*C);
          IntptrTy = IRB.getIntPtrTy(DL);

          MemmoveFn = M.getOrInsertFunction("__ttt__", IRB.getVoidTy(), IRB.getInt8PtrTy(),
                                            IRB.getInt32Ty(), IRB.getInt32Ty(), IRB.getInt1Ty());
    }

    virtual bool runOnFunction(Function &F) {
      dbgs() << "Function: " << F.getName() << "\n";

      bool Changed = false;
      SmallVector<Instruction*, 16> ToInstrument;

      for (auto &BB : F) {
          for (auto &Inst : BB) {
            Value *MaybeMask = nullptr;
            bool IsWrite;
            unsigned Alignment;
            uint64_t TypeSize;
            Value *Addr = isInterestingMemoryAccess(&Inst, &IsWrite, &TypeSize,
                                                    &Alignment, &MaybeMask);
            if (Addr || isa<MemIntrinsic>(Inst)){
                dbgs() << IsWrite << " | " << Alignment << " | " << TypeSize  << "\n";

                ToInstrument.push_back(&Inst);
            }
        }
      }

      for (auto Inst : ToInstrument)
          Changed |= instrumentMemAccess(Inst);

      return false;
    }

    Value * isInterestingMemoryAccess(Instruction *I,
                                                       bool *IsWrite,
                                                       uint64_t *TypeSize,
                                                       unsigned *Alignment,
                                                       Value **MaybeMask) {

      Value *PtrOperand = nullptr;
      const DataLayout &DL = I->getModule()->getDataLayout();
      if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
        *IsWrite = false;
        *TypeSize = DL.getTypeStoreSizeInBits(LI->getType());
        *Alignment = LI->getAlignment();
        PtrOperand = LI->getPointerOperand();
      } else if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
        *IsWrite = true;
        *TypeSize = DL.getTypeStoreSizeInBits(SI->getValueOperand()->getType());
        *Alignment = SI->getAlignment();
        PtrOperand = SI->getPointerOperand();
      }
#if 1
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

    bool instrumentMemAccess(Instruction *I) {
      dbgs() << "Instrumenting: " << *I << "\n";
      bool IsWrite = false;
      unsigned Alignment = 0;
      uint64_t TypeSize = 0;
      Value *MaybeMask = nullptr;
      Value *Addr =
          isInterestingMemoryAccess(I, &IsWrite, &TypeSize, &Alignment, &MaybeMask);

      if (!Addr)
        return false;

      if (MaybeMask)
        return false; //FIXME

      IRBuilder<> IRB(I);


      Value *AddrLong = IRB.CreatePointerCast(Addr, IntptrTy);

      const bool isPowerOf2 = isPowerOf2_64(TypeSize);
      const bool typeLessThan_16s8 = (TypeSize / 8 <= (1UL << (kNumberOfAccessSizes - 1)));
      const bool AlignmentNormilize = (Alignment >= (1UL << kShadowScale) || Alignment == 0 ||
       Alignment >= TypeSize / 8);

      if (isPowerOf2_64(TypeSize) &&
            (TypeSize / 8 <= (1UL << (kNumberOfAccessSizes - 1))) &&
            (Alignment >= (1UL << kShadowScale) || Alignment == 0 ||
             Alignment >= TypeSize / 8)) {
          size_t AccessSizeIndex = TypeSizeToSizeIndex(TypeSize);

          dbgs() << "isPowerOf2_64" << " |AccessSizeIndex  " << AccessSizeIndex << " |isPowerOf2  " << isPowerOf2 << " |typeLessThan_16s8  "
                 << typeLessThan_16s8 << " |AlignmentNormilize " << AlignmentNormilize << "\n";

          std::vector<Value *> ArgsV;
          ArgsV.push_back(AddrLong);

          ConstantInt * TypeSizeArg = IRB.getInt32(TypeSize);
          ArgsV.push_back(TypeSizeArg);

          ConstantInt * AlignmentArg = IRB.getInt32(Alignment);
           ArgsV.push_back(AlignmentArg);

          ConstantInt * IsWrite = IRB.getInt1(IsWrite);
           ArgsV.push_back(IsWrite);

          IRB.CreateCall(MemmoveFn,ArgsV);

          I->eraseFromParent();
          //I->removeFromParent();


      } else {
          dbgs() << "CreateCall" << " | "  << " | " << isPowerOf2 << " | "
                 << typeLessThan_16s8 << " | " << typeLessThan_16s8 << " | " << AlignmentNormilize << "\n";
      }

      return true;
    }

    Function *callback;
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

