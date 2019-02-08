#ifndef MEMORYOPERATIONRECOGNIZE_H
#define MEMORYOPERATIONRECOGNIZE_H

#include "llvm/IR/Instruction.h"


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

enum MemoryInstr_t {
    _NOT_MEMORY_INSTR = 0,
    _MEMORY_INSTR,
    _UNSUPPORTED_MEMORY_INSTR
};


MemoryInstr_t instructionMemRecognize(Instruction *I, AttributMemOperation &op);

} //namespace

#endif // MEMORYOPERATIONRECOGNIZE_H
