#ifndef MEMORYOPERATIONRECOGNIZE_H
#define MEMORYOPERATIONRECOGNIZE_H

#include "llvm/IR/Instruction.h"
#include "Settings.h"

namespace adin {

using namespace llvm;

struct AttributMemOperation {
    Value *PtrOperand = nullptr;
    Value *PtrValue = nullptr;
    Type * ReturnType = nullptr;
    bool IsWrite = false;
    uint64_t TypeSize = 0;
    unsigned Alignment = 0;
};


bool isInterestingMemoryAccess(Instruction *I, AttributMemOperation &op, const Settings);

} //namespace

#endif // MEMORYOPERATIONRECOGNIZE_H
