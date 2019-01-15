#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/Support/Debug.h"
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

#include "InlineInstrumentation.h"
#include "MemoryOperationRecognize.h"
#include "AllocaRecognize.h"


using namespace llvm;

#define DEBUG_TYPE "AddressInterceptPass"


static cl::opt<int>  VerboseLevel(
    "adin-verbose-level",
    cl::desc("Set Level of verbose for AddressIntercept Pass"),
    cl::Hidden, cl::init(0));

static cl::opt<std::string> NameCallbackStore(
    "adin-name-callback-store",
    cl::desc("Set name callback of store operation. Default __adin_store_"),
    cl::Hidden, cl::init("__adin_store_"));

static cl::opt<std::string> NameCallbackLoad(
    "adin-name-callback-load",
    cl::desc("Set name callback of load operation. Default __adin_load_"),
    cl::Hidden, cl::init("__adin_load_"));

static cl::opt<bool> ClInstrumentReads("adin-alloca-address-skip",
                                       cl::desc("Skip intercept address on alloca frame (Stack var)"),
                                       cl::Hidden, cl::init(true));



namespace aip{

  struct AddressInterceptPass : public FunctionPass {

      static char ID;

      AllocaRecognize AllocaRecognizer;

    AddressInterceptPass() : FunctionPass(ID) {}


    bool doInitialization(Module &M) override {
        dbgs() << "Init " << M.getName() << "\n";

        initMemFn(M, "__store__","__load__");

        return false;
    }

    virtual bool runOnFunction(Function &F) {
        dbgs() << "Function: " << F.getName() << "\n";

        AllocaRecognizer.markEscapedLocalAllocas(F);

        bool Changed = false;
        SmallVector<Instruction*, 16> ToInstrument;

        for (auto &BB : F) {
            for (auto &Inst : BB) {

                AttributMemOperation op;

                if(isInterestingMemoryAccess(&Inst, op) == false)
                    continue;

                if (auto AI = dyn_cast_or_null<AllocaInst>(op.PtrOperand)){
                     dbgs() << "dyn_cast_or_null Inst Alloca : " << Inst << "\n";
                }

                if(AllocaRecognizer.isProbablyAllocaOperation(op.PtrOperand)){
                    dbgs() << "Inst Alloca : " << Inst << "\n";
                    continue;
                }

                if (op.PtrOperand || isa<MemIntrinsic>(Inst)){
                    ToInstrument.push_back(&Inst);
                }
            }
        }

        dbgs() << "ToInstrument : " << ToInstrument.size() << "\n";


        for (auto Inst : ToInstrument){
            Changed |= instrumentMemAccess(Inst);
        }

        AllocaRecognizer.exitFunction();

        return Changed;
    }

  };
}

using namespace aip;

char AddressInterceptPass::ID = 0;

// Automatically enable the pass.

static void registerAddressInterceptPass(const PassManagerBuilder &,
                         legacy::PassManagerBase &PM) {
  PM.add(new AddressInterceptPass());
}
static RegisterStandardPasses
  RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                 registerAddressInterceptPass);

