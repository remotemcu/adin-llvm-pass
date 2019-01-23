#ifndef INLINEINSTRUMENTATION_H
#define INLINEINSTRUMENTATION_H

#include "llvm/IR/Instruction.h"


namespace adin {

using namespace llvm;

void initMemFn(Module &M,  const std::string NameStore,  const std::string NameLoad);

bool instrumentMemAccess(Instruction *I);

bool isNormalAddressAlignment(Instruction *I);

} //namespace


#endif // INLINEINSTRUMENTATION_H
