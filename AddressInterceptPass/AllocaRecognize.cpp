#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
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

#include "AllocaRecognize.h"
#include "Logger.h"


namespace adin {

void AllocaRecognize::markEscapedLocalAllocas(Function &F) {
    // Find the one possible call to llvm.localescape and pre-mark allocas passed
    // to it as uninteresting. This assumes we haven't started processing allocas
    // yet. This check is done up front because iterating the use list in
    // isInterestingAlloca would be algorithmically slower.
    ADIN_LOG(_DEBUG) << "ProcessedAllocas.size() = " << ProcessedAllocas.size();
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

static uint64_t getAllocaSizeInBytes(const AllocaInst &AI){
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

bool AllocaRecognize::isProbablyNotAllocaOperation(Value *PtrOperand){

    AllocaInst * AIPointer = dyn_cast_or_null<AllocaInst>(PtrOperand);
    if (!AIPointer){
            return true;
    }

    AllocaInst & AI = *AIPointer;

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

} //namespace
