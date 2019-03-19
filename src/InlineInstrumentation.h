#ifndef INLINEINSTRUMENTATION_H
#define INLINEINSTRUMENTATION_H

#include "llvm/IR/IntrinsicInst.h"

namespace adin {

using namespace llvm;

void initMemFn(Module &M,  const std::string NameStore,  const std::string NameLoad,
               const std::string NameMemmove,  const std::string NameMemcpy,
               const std::string NameMemset);

bool instrumentMemAccess(Instruction *I);

bool instrumentMemIntrinsic(MemIntrinsic *MI);

bool isNormalAddressAlignment(Instruction *I);

} //namespace


#endif // INLINEINSTRUMENTATION_H
