#ifndef ALLOCARECOGNIZE_H
#define ALLOCARECOGNIZE_H

#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Instructions.h"


namespace aip {

using namespace llvm;

class AllocaRecognize{

    bool enable;
    llvm::DenseMap<const llvm::AllocaInst *, bool> ProcessedAllocas;

    bool isProbablyNotAllocaOperation(Value *PtrOperand);

public:

    explicit AllocaRecognize(const bool enable= true)
        : enable(enable) {}

    void markEscapedLocalAllocas(Function &F);

    void exitFunction(){
        ProcessedAllocas.clear();
    }

    bool isProbablyAllocaOperation(Value *PtrOperand){
        return !isProbablyNotAllocaOperation(PtrOperand);
    }

};

} //namespace
#endif
