#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"

#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

#include "InlineInstrumentation.h"
#include "MemoryOperationRecognize.h"
#include "Logger.h"

namespace adin {

// Accesses sizes are powers of two: 1, 2, 4, 8, 16.
static const size_t kNumberOfAccessSizes = 5;
// 1 << 4 ... 16
static const size_t kShadowScale = 4;

static Function *MemStoreFn;
static Function *MemLoadFn;
//static Function *MemMemmoveFn;
static Function *MemMemcpyFn;
static Function *MemMemsetFn;

void initMemFn(Module &M, const std::string NameStore,  const std::string NameLoad,
               const std::string NameMemmove,  const std::string NameMemcpy,
               const std::string NameMemset){

    IRBuilder<> IRB(M.getContext());

    MemStoreFn = cast<Function>(M.getOrInsertFunction(NameStore, IRB.getVoidTy(), IRB.getInt8PtrTy(), IRB.getInt64Ty(),
                                                      IRB.getInt32Ty(), IRB.getInt32Ty())
                                );

    MemLoadFn = cast<Function>(M.getOrInsertFunction(NameLoad, IRB.getInt64Ty(), IRB.getInt8PtrTy(),
                                      IRB.getInt32Ty(), IRB.getInt32Ty())
                               );
#if 0
    MemMemmoveFn = checkSanitizerInterfaceFunction( M.getOrInsertFunction(NameMemmove,
                                              IRB.getVoidTy(), IRB.getInt8PtrTy(),
                                              IRB.getInt8PtrTy(), IRB.getInt32Ty()
                                                                             ));
#endif
    MemMemcpyFn = checkSanitizerInterfaceFunction((M.getOrInsertFunction(NameMemcpy,
                                              IRB.getVoidTy(), IRB.getInt8PtrTy(),
                                                     IRB.getInt8PtrTy(), IRB.getInt32Ty())
                                                       ));

    MemMemsetFn = checkSanitizerInterfaceFunction((M.getOrInsertFunction(NameMemset,
                                              IRB.getVoidTy(), IRB.getInt8PtrTy(),
                                              IRB.getInt8Ty(), IRB.getInt32Ty())
                                                       ));
}

bool instrumentMemIntrinsic(MemIntrinsic *MI) {
    IRBuilder<> IRB(MI);
    if (isa<MemTransferInst>(MI) && isa<MemCpyInst>(MI)) {
        IRB.CreateCall(
            MemMemcpyFn,
            //isa<MemMoveInst>(MI) ? MemMemmoveFn : MemMemcpyFn,
            {IRB.CreatePointerCast(MI->getOperand(0), IRB.getInt8PtrTy(), "dest_i8ptr_"),
             IRB.CreatePointerCast(MI->getOperand(1), IRB.getInt8PtrTy(), "src_i8ptr_"),
             IRB.CreateIntCast(MI->getOperand(2), IRB.getInt32Ty(), false, "size_int32_")});
    } else if (isa<MemSetInst>(MI)) {
        IRB.CreateCall(
            MemMemsetFn,
            {IRB.CreatePointerCast(MI->getOperand(0), IRB.getInt8PtrTy(), "mem_set_dest_i8ptr_"),
             IRB.CreateIntCast(MI->getOperand(1), IRB.getInt8Ty(), false, "mem_set_value_i8_"),
             IRB.CreateIntCast(MI->getOperand(2), IRB.getInt32Ty(), false, "size_int32_")});
    } else {
        return false;
    }
    MI->eraseFromParent();
    return true;
}

bool isNormalAddressAlignment(Instruction *I){
    AttributMemOperation op;
    if(instructionMemRecognize(I, op) != _MEMORY_INSTR)
        return false;
    const bool NormalAlignment = (op.Alignment >= (1UL << kShadowScale) || op.Alignment == 0 ||
                                     op.Alignment >= op.TypeSize / 8);
    return NormalAlignment;
}

static std::string getFormatNameType(const Type * type){
    std::string name;
    raw_string_ostream os(name);
    os << *type;
    os.flush();
    if(name[0] == '%'){
        name[0] = '_';
    }
    if(name.at(name.size()-1) == '*'){
        name.at(name.size()-1) = '_';
        name.push_back('p');
    }
    name.push_back('_');
    return name;
}

bool instrumentMemAccess(Instruction *I)
{
    ADIN_LOG(__DEBUG) << "instrumenting: " << *I;
    AttributMemOperation op;

    if(instructionMemRecognize(I, op) != _MEMORY_INSTR)
        return false;

    if (!op.PtrOperand)
        return false;

    const bool isPowerOf2 = isPowerOf2_64(op.TypeSize);
    const bool typeLessThan_16s8 = (op.TypeSize / 8 <= (1UL << (kNumberOfAccessSizes - 1)));

    const bool instrumentEnable = isPowerOf2 && typeLessThan_16s8;

    if(instrumentEnable == false){
        ADIN_LOG(__ERROR) << "current instruction: " << *I;
        ADIN_LOG(__ERROR) << "current operation size: " <<  op.TypeSize << " should be power of 2";
        llvm_unreachable("strange fortune - check bitcode^");
    }

    IRBuilder<> IRB(I);

    std::string namePtrCast = "cast_Ptr_" + getFormatNameType(op.PtrOperand->getType()) + "to_i8p_";

    Value *AddrLong = IRB.CreatePointerCast(op.PtrOperand, IRB.getInt8PtrTy(), namePtrCast);

    std::vector<Value *> ArgsV;
    ArgsV.push_back(AddrLong);

    if(op.IsWrite){
        std::string nameValueCast = "cast_Val_" + getFormatNameType(op.PtrValue->getType()) + "_to_i64_";
        Value *ValueLong = nullptr;
        if(op.PtrValue->getType()->isPointerTy()){
            ValueLong = IRB.CreatePtrToInt(op.PtrValue, IRB.getInt64Ty(), nameValueCast);
        } else {
            ValueLong = IRB.CreateIntCast(op.PtrValue, IRB.getInt64Ty(), false, nameValueCast);
        }
        ArgsV.push_back(ValueLong);
    }

    ConstantInt * TypeSizeArg = IRB.getInt32(op.TypeSize);
    ArgsV.push_back(TypeSizeArg);

    ConstantInt * AlignmentArg = IRB.getInt32(op.Alignment);
    ArgsV.push_back(AlignmentArg);

    if(op.IsWrite){
        IRB.CreateCall(MemStoreFn,ArgsV);
    } else {

        std::string nameLoad = "load_" + getFormatNameType(op.ReturnType);

        Instruction * loadInstr = IRB.CreateCall(MemLoadFn,ArgsV, nameLoad);

        const Type * typeLoadOperand = loadInstr->getType();

        Value * convertOperation = nullptr;
        Type * convertType = nullptr;

        if(typeLoadOperand->isIntegerTy() && op.ReturnType->isIntegerTy()){
            if(op.ReturnType->isIntegerTy(1)){
                convertType = IRB.getInt1Ty();
            } else if(op.ReturnType->isIntegerTy(8)){
                convertType = IRB.getInt8Ty();
            } else if (op.ReturnType->isIntegerTy(16)) {
                convertType = IRB.getInt16Ty();
            } else if (op.ReturnType->isIntegerTy(32)) {
                convertType = IRB.getInt32Ty();
            } else if (op.ReturnType->isIntegerTy(64)) {
                convertType = IRB.getInt64Ty();
            } else {
                ADIN_LOG(__ERROR) << "current instruction: " << *I;
                ADIN_LOG(__ERROR) << "operand type of return" << *op.ReturnType;
                llvm_unreachable("error integer type of operand^");
            }

            std::string nameTrunc = "truncated_" + getFormatNameType(op.ReturnType);
            convertOperation = IRB.CreateTrunc(loadInstr, convertType, nameTrunc);
        } else if(
            typeLoadOperand->isIntegerTy() && op.ReturnType->isPointerTy()
        ){
            std::string nameTrunc = "converted" + getFormatNameType(op.ReturnType) + "_to_prt" ;
            convertOperation = IRB.CreateIntToPtr(loadInstr, op.ReturnType, nameTrunc);
        } else {
            // float blyat!!!!
            ADIN_LOG(__ERROR) << "current instruction: " << *I;
            llvm_unreachable("operand error - please, checks operand of instruction^");
        }

        I->replaceAllUsesWith(convertOperation);

    }

    I->eraseFromParent();

    return true;
}

#if 0
float BSP_CurrentGet( void )
{
   BCP_Packet pkt;
   float      *pcurrent;

   pkt.type          = BSP_BCP_CURRENT_REQ;
   pkt.payloadLength = 0;

   /* Send Request/Get reply */
   BSP_BccPacketSend( &pkt );
   BSP_BccPacketReceive( &pkt );

   /* Process reply */
   pcurrent = (float *)pkt.data;
   if ( pkt.type != BSP_BCP_CURRENT_REPLY )
   {
      *pcurrent = 0.0f;
   }

   return *pcurrent;
}

/**************************************************************************//**
 * @brief Request AEM (Advanced Energy Monitoring) voltage from board controller.
 *
 * @note Assumes that BSP_Init() has been called with @ref BSP_INIT_BCC
 *       bitmask.
 *
 * @return
 *   The voltage. Returns 0.0 on board controller communication
 *   error.
 *****************************************************************************/
float BSP_VoltageGet( void )
{
   BCP_Packet pkt;
   float      *pvoltage;

   pkt.type          = BSP_BCP_VOLTAGE_REQ;
   pkt.payloadLength = 0;

   /* Send Request/Get reply */
   BSP_BccPacketSend( &pkt );
   BSP_BccPacketReceive( &pkt );

   /* Process reply */
   pvoltage = (float *)pkt.data;
   if ( pkt.type != BSP_BCP_VOLTAGE_REPLY )
   {
      *pvoltage = 0.0f;
   }

   return *pvoltage;
}
#endif

} //namespace
