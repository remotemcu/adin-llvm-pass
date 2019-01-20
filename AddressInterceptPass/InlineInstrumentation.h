#ifndef INLINEINSTRUMENTATION_H
#define INLINEINSTRUMENTATION_H

#include "llvm/IR/Instruction.h"


namespace adin {

using namespace llvm;

void initMemFn(Module &M, std::string NameStore, std::string NameLoad);

bool instrumentMemAccess(Instruction *I);

} //namespace


#endif // INLINEINSTRUMENTATION_H
