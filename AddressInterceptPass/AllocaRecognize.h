#ifndef ALLOCARECOGNIZE_H
#define ALLOCARECOGNIZE_H

#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Instructions.h"


namespace aip {

using namespace llvm;

class AllocaRecognize{

    llvm::DenseMap<const llvm::AllocaInst *, bool> ProcessedAllocas;

    bool isProbablyNotAllocaOperation(Value *PtrOperand);

public:

    explicit AllocaRecognize(Function &F) {
        markEscapedLocalAllocas(F);
    }

    ~AllocaRecognize() {
        exitFunction();
    }

    bool isProbablyAllocaOperation(Value *PtrOperand){
        return !isProbablyNotAllocaOperation(PtrOperand);
    }

private:

    void markEscapedLocalAllocas(Function &F);

    void exitFunction(){
        ProcessedAllocas.clear();
    }

};

} //namespace
#endif
