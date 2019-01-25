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


bool isInterestingMemoryAccess(Instruction *I, AttributMemOperation &op)
{
    const DataLayout &DL = I->getModule()->getDataLayout();

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
    else{
        return false;
    }

    if ( op.PtrOperand == nullptr) {
        return false;
    }

    // Do not instrument acesses from different address spaces; we cannot deal
    // with them.
    Type *PtrTy = cast<PointerType>(op.PtrOperand->getType()->getScalarType());
    if (PtrTy->getPointerAddressSpace() != 0){
        return false;
    }

    // Ignore swifterror addresses.
    // swifterror memory addresses are mem2reg promoted by instruction
    // selection. As such they cannot have regular uses like an instrumentation
    // function and it makes no sense to track them as memory.
    if (op.PtrOperand->isSwiftError()){
        return false;
    }

    return true;
}


} //namespace
