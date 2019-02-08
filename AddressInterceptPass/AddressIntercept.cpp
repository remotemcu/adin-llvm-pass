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
#include "Logger.h"



using namespace llvm;

#define DEBUG_TYPE "AddressInterceptPass"


static cl::opt<int>  VerboseLevel(
    "adin-verbose-level",
    cl::desc("Set Level of verbose for AddressIntercept Pass"),
    cl::NotHidden, cl::init(0));

static cl::opt<std::string> NameCallbackStore(
    "adin-name-callback-store",
    cl::desc("Set name callback of store operation. Default __adin_store_"),
    cl::NotHidden, cl::init("__adin_store_"));

static cl::opt<std::string> NameCallbackLoad(
    "adin-name-callback-load",
    cl::desc("Set name callback of load operation. Default __adin_load_"),
    cl::NotHidden, cl::init("__adin_load_"));

static cl::opt<bool> AllocaAddressSkip("adin-alloca-address-skip",
                                       cl::desc("Skip intercept address on alloca frame (Stack var)"),
                                       cl::NotHidden, cl::init(true));

static cl::opt<bool> CheckNormalAddressAlignment("adin-check-normal-address-aligment",
                                       cl::desc("Checks normal alignment of address attempt"),
                                       cl::NotHidden, cl::init(false));

static cl::opt<bool> SkipUnsupportedInstr("adin-skip-unsupported-instructions",
                                                 cl::desc("if equal true - skip this unsupported instruction, else throw error"),
                                                 cl::NotHidden, cl::init(false));



namespace adin{

  struct AddressInterceptor : public FunctionPass {

      static char ID;

    AddressInterceptor() : FunctionPass(ID) {
        Log::setGLevel(static_cast<LevelDebug>(VerboseLevel.getValue()));
        ADIN_LOG(__DEBUG) << "Set verbose level : " << VerboseLevel;
    }

    bool doInitialization(Module &M) override {

        ADIN_LOG(__DEBUG) << "Init module" << M.getName();

        initMemFn(M, NameCallbackStore, NameCallbackLoad);

        return false;
    }

    virtual bool runOnFunction(Function &F) {

        ADIN_LOG(__DEBUG) << "Examine function: " << F.getName();

        AllocaRecognize AllocaRecognizer(F);

        bool Changed = false;
        SmallVector<Instruction*, 16> ToInstrument;

        for (auto &BB : F) {
            for (auto &Inst : BB) {

                AttributMemOperation op;

                const MemoryInstr_t iType = instructionMemRecognize(&Inst, op);

                if(iType == _NOT_MEMORY_INSTR)
                    continue;

                if(iType == _UNSUPPORTED_MEMORY_INSTR && SkipUnsupportedInstr.getValue()){
                    ADIN_LOG(__ERROR) << " Instruction:  " << Inst;
                    llvm_unreachable("it is unsupported instruction^ - try to replace this");
                }

                if(AllocaAddressSkip.getValue() &&
                        AllocaRecognizer.isProbablyAllocaOperation(op.PtrOperand)){
                    ADIN_LOG(__DEBUG) << "Inst Alloca skip: " << Inst;
                    continue;
                }

                if (op.PtrOperand || isa<MemIntrinsic>(Inst)){
                    ToInstrument.push_back(&Inst);
                }
            }
        }

        ADIN_LOG(__DEBUG) << "Qty of instrumenting parts: " << ToInstrument.size();

        for (auto Inst : ToInstrument){
            if(CheckNormalAddressAlignment.getValue() &&
                isNormalAddressAlignment(Inst) == false){
                ADIN_LOG(__ERROR) << "current instruction: " << *Inst;
                llvm_unreachable("operand of address must be normal align with type size");
            }
            Changed |= instrumentMemAccess(Inst);
        }

        return Changed;
    }

  };
}

using namespace adin;

char AddressInterceptor::ID = 0;

#ifdef ADIN_LOAD_MODULE

static RegisterPass<AddressInterceptor> X("adin", "Hello World Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);

// Automatically enable the pass.

static void registerAddressInterceptor(const PassManagerBuilder &,
                         legacy::PassManagerBase &PM) {
  PM.add(new AddressInterceptor());
}
static RegisterStandardPasses
  RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                 registerAddressInterceptor);

#else

INITIALIZE_PASS_BEGIN(AddressInterceptor, "adin",
                      "adin: detect memory bugs using tagged addressing.", false, false)
INITIALIZE_PASS_END(AddressInterceptor, "adin",
                    "adin: detect memory bugs using tagged addressing.", false, false)


FunctionPass *llvm::createAddressInterceptor() {
    return new AddressInterceptor();
}

/// initializeVectorizationPasses - Initialize all passes linked into the
/// Vectorization library.
void llvm::initializeAddressInterception(PassRegistry &Registry) {
    initializeAddressInterceptorPass(Registry);
}

#endif
