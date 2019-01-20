#ifndef MEMORYOPERATIONRECOGNIZE_H
#define MEMORYOPERATIONRECOGNIZE_H

#include "llvm/IR/Instruction.h"


namespace adin {

using namespace llvm;

struct AttributMemOperation {
    Value *PtrOperand = nullptr;
    Value *PtrValue = nullptr;
    bool IsWrite = false;
    uint64_t TypeSize = 0;
    unsigned Alignment = 0;
};


bool isInterestingMemoryAccess(Instruction *I, AttributMemOperation &op);

} //namespace

#endif // MEMORYOPERATIONRECOGNIZE_H


       /*
class MemoryOperationRecognize
{
public:
MemoryOperationRecognize();
};
*/
