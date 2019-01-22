#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"

#include "InlineInstrumentation.h"
#include "MemoryOperationRecognize.h"
#include "Logger.h"

namespace adin {

// Accesses sizes are powers of two: 1, 2, 4, 8, 16.
static const size_t kNumberOfAccessSizes = 5;
// 1 << 4 ... 16
static const size_t kShadowScale = 4;

static Value *MemStoreFn;
static Value *MemLoadFn;

void initMemFn(Module &M, std::string NameStore, std::string NameLoad){

    IRBuilder<> IRB(M.getContext());

    MemStoreFn = M.getOrInsertFunction(NameStore, IRB.getVoidTy(), IRB.getInt8PtrTy(), IRB.getInt32Ty(),
                                      IRB.getInt32Ty(), IRB.getInt32Ty());


    MemLoadFn = M.getOrInsertFunction(NameLoad, IRB.getInt32Ty(), IRB.getInt8PtrTy(),
                                      IRB.getInt32Ty(), IRB.getInt32Ty());
}

bool isNormalAddressAlignment(Instruction *I){
    AttributMemOperation op;
    if(!isInterestingMemoryAccess(I, op))
        return false;
    const bool NormalAlignment = (op.Alignment >= (1UL << kShadowScale) || op.Alignment == 0 ||
                                     op.Alignment >= op.TypeSize / 8);
    return NormalAlignment;
}

bool instrumentMemAccess(Instruction *I)
{
    ADIN_LOG(_DEBUG) << "instrumenting: " << *I;
    AttributMemOperation op;

    if(!isInterestingMemoryAccess(I, op))
        return false;

    if (!op.PtrOperand)
        return false;

    const bool isPowerOf2 = isPowerOf2_64(op.TypeSize);
    const bool typeLessThan_16s8 = (op.TypeSize / 8 <= (1UL << (kNumberOfAccessSizes - 1)));

    const bool instrumentEnable = isPowerOf2 && typeLessThan_16s8;

    if(instrumentEnable == false){
        ADIN_LOG(_ERROR) << "current instruction: " << *I;
        ADIN_LOG(_ERROR) << "current operation size: " <<  op.TypeSize << " should be power of 2";
        llvm_unreachable("strange fortune - check bitcode^");
        return false;
    }

    IRBuilder<> IRB(I);

    Value *AddrLong = IRB.CreatePointerCast(op.PtrOperand, IRB.getInt8PtrTy());

    std::vector<Value *> ArgsV;
    ArgsV.push_back(AddrLong);

    if(op.IsWrite){
        Value *ValueLong = IRB.CreateIntCast(op.PtrValue, IRB.getInt32Ty(), true);
        ArgsV.push_back(ValueLong);
    }

    ConstantInt * TypeSizeArg = IRB.getInt32(op.TypeSize);
    ArgsV.push_back(TypeSizeArg);

    ConstantInt * AlignmentArg = IRB.getInt32(op.Alignment);
    ArgsV.push_back(AlignmentArg);

    if(op.IsWrite){
        IRB.CreateCall(MemStoreFn,ArgsV);
    } else {

        Instruction * rep = IRB.CreateCall(MemLoadFn,ArgsV);

        Type * typeLoadOperand = rep->getType();

        switch (op.TypeSize) {
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
            ADIN_LOG(_ERROR) << "current instruction: " << *I;
            ADIN_LOG(_ERROR) << "operand size: " << op.TypeSize;
            llvm_unreachable("unknown size of operation^");
            break;
        }

        Value * trunc = IRB.CreateTrunc(rep, typeLoadOperand);

        I->replaceAllUsesWith(trunc);
    }

    I->eraseFromParent();

}


} //namespace
