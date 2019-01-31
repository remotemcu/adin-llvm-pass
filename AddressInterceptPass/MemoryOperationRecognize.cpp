#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"

#include "MemoryOperationRecognize.h"
#include "Logger.h"



namespace adin {


MemoryInstr_t instructionMemRecognize(Instruction *I, AttributMemOperation &op)
{
    const DataLayout &DL = I->getModule()->getDataLayout();

    bool unsupportedInstr = false;

    if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
        op.IsWrite = false;
        op.TypeSize = DL.getTypeStoreSizeInBits(LI->getType());
        op.Alignment = LI->getAlignment();
        op.PtrOperand = LI->getPointerOperand();
        op.ReturnType = LI->getType();
    }
    else
        if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
        op.IsWrite = true;
        op.TypeSize = DL.getTypeStoreSizeInBits(SI->getValueOperand()->getType());
        op.Alignment = SI->getAlignment();
        op.PtrOperand = SI->getPointerOperand();
        op.PtrValue = SI->getValueOperand();
    }
    /*
     *  This feature will be in the future
     */
      else if (AtomicRMWInst *RMW = dyn_cast<AtomicRMWInst>(I)) {
        ADIN_LOG(_ERROR) << *RMW;
        unsupportedInstr = true;
      } else if (AtomicCmpXchgInst *XCHG = dyn_cast<AtomicCmpXchgInst>(I)) {
        ADIN_LOG(_ERROR) << *XCHG;
        unsupportedInstr = true;
      } else if (auto CI = dyn_cast<CallInst>(I)) {
          auto *F = dyn_cast<Function>(CI->getCalledValue());
          if (F && (F->getName().startswith("llvm.masked.load.") ||
                    F->getName().startswith("llvm.masked.store."))) {
              ADIN_LOG(_ERROR) << F->getName();
              unsupportedInstr = true;
          }
      } else{
        return _NOT_MEMORY_INSTR;
    }

    if(unsupportedInstr){
        return _UNSUPPORTED_MEMORY_INSTR;
    }

    if ( op.PtrOperand == nullptr) {
        return _NOT_MEMORY_INSTR;
    }

    // Do not instrument acesses from different address spaces; we cannot deal
    // with them.
    Type *PtrTy = cast<PointerType>(op.PtrOperand->getType()->getScalarType());
    if (PtrTy->getPointerAddressSpace() != 0){
        return _NOT_MEMORY_INSTR;
    }

    // Ignore swifterror addresses.
    // swifterror memory addresses are mem2reg promoted by instruction
    // selection. As such they cannot have regular uses like an instrumentation
    // function and it makes no sense to track them as memory.
    if (op.PtrOperand->isSwiftError()){
        return _NOT_MEMORY_INSTR;
    }

    return _MEMORY_INSTR;
}


} //namespace
