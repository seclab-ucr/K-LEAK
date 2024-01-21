#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/SourceMgr.h"

#include "SCC.h"
#include "Utils.h"
#include "Visitor.h"

#include <map>
#include <set>
#include <vector>

using namespace llvm;
using namespace std;

extern uint32_t maxCallDepth;

FunctionVisitor::FunctionVisitor(GlobalState &glbState, Context context, const CGMap &callGraph, PointsToRecords &ptoRecords, DDG &ddg) : glbState(glbState), context(context), callGraph(callGraph), ptoRecords(ptoRecords), ddg(ddg)
#ifdef NUM_VISITED_BBS
, numVisitedBBs(0)
#endif
{
    this->dataLayout = &context.entry->getParent()->getDataLayout();
}

void FunctionVisitor::analyze()
{
    Function &F = *(this->context.getCurrentFunction());
    vector<vector<BasicBlock *>> SCCs = getSCCTraversalOrder(F);
    for (vector<BasicBlock *> &SCC : SCCs)
    {
        for (BasicBlock *BB : SCC)
        {
            this->visit(*BB);
#ifdef NUM_VISITED_BBS
            this->numVisitedBBs++;
#endif
        }
    }
}

void FunctionVisitor::visitAllocaInst(AllocaInst &allocaInst)
{
    // new object
    Type *allocatedType = allocaInst.getAllocatedType();
    uint64_t typeSize = this->dataLayout->getTypeStoreSize(allocatedType);
    ObjId objId = ObjectManager::createStackObject(typeSize);

    // update pto records for allocaInst
    ObjLoc pointee(objId, 0); // must point to object start
    this->ptoRecords.addPteesForValPtr(this->context, &allocaInst, set<ObjLoc>{pointee});
}

void FunctionVisitor::visitBinaryOperator(BinaryOperator &biOp)
{
    Value *opd0 = biOp.getOperand(0);
    Value *opd1 = biOp.getOperand(1);

    // TODO: pointer arithmetic

    // DDG
    processBiOp(this->ddg, this->context, opd0, opd1, &biOp);
}

void FunctionVisitor::visitPHINode(llvm::PHINode &phiNode)
{
    // merge all ptees of incoming values
    set<Value *> incomingValues;
    set<ObjLoc> phiNodePtees;
    for (unsigned i = 0; i < phiNode.getNumIncomingValues(); ++i) {
        Value *val = phiNode.getIncomingValue(i);
        incomingValues.insert(val);

        set<ObjLoc> valPtees = getPteesOfValPtr(this->dataLayout, this->ptoRecords, this->context, val, false);
        phiNodePtees.insert(valPtees.begin(), valPtees.end());
    }

    // update pto records for phiNode
    this->ptoRecords.addPteesForValPtr(this->context, &phiNode, phiNodePtees);

    // DDG
    processPHINode(this->ddg, this->context, incomingValues, &phiNode);
}

void FunctionVisitor::visitLoadInst(LoadInst &loadInst)
{
    Value *src = loadInst.getPointerOperand();

    // get src ptees
    // create a dummy pointee if no src ptees
    set<ObjLoc> srcPtees = getPteesOfValPtr(this->dataLayout, this->ptoRecords, this->context, src, true);

    // get ptees of src ptees
    // create a dummy pointee if the loadInst loads a pointer
    bool createDummyPointee = loadInst.getType()->isPointerTy();
    InstLoc currLoc = InstLoc(this->context, &loadInst);
    set<ObjLoc> srcPteesPtees;
    for (ObjLoc srcPtee : srcPtees)
    {
        set<ObjLoc> srcPteePtees = getPteesOfObjPtr(this->ptoRecords, srcPtee, currLoc, createDummyPointee, currLoc);
        srcPteesPtees.insert(srcPteePtees.begin(), srcPteePtees.end());
    }

    // update pto records for loadInst
    this->ptoRecords.addPteesForValPtr(this->context, &loadInst, srcPteesPtees);

    // DDG
    uint64_t loadSize = this->dataLayout->getTypeStoreSize(loadInst.getType());
    processLoad(this->ddg, this->context, srcPtees, &loadInst, src, loadSize);
}

void FunctionVisitor::visitStoreInst(StoreInst &storeInst)
{
    Value *dst = storeInst.getPointerOperand();
    Value *val = storeInst.getValueOperand();

    // get val ptees
    // create a dummy pointee if the storeInst stores a pointer that has no pointee
    bool createDummyPointee = val->getType()->isPointerTy();
    set<ObjLoc> valPtees = getPteesOfValPtr(this->dataLayout, this->ptoRecords, this->context, val, createDummyPointee);

    // get dst ptees
    // create a dummy pointee if no dst ptees
    set<ObjLoc> dstPtees = getPteesOfValPtr(this->dataLayout, this->ptoRecords, this->context, dst, true);

    // update pto records for dst ptees
    InstLoc currLoc = InstLoc(this->context, &storeInst);
    for (ObjLoc dstPtee : dstPtees)
    {
        this->ptoRecords.addPteesForObjPtr(dstPtee, valPtees, currLoc);
    }

    // DDG
    uint64_t storeSize = this->dataLayout->getTypeStoreSize(val->getType());
    processStore(this->ddg, this->context, val, dstPtees, &storeInst, dst, storeSize);
}

void FunctionVisitor::visitMemCpyInst(MemCpyInst &memCpyInst)
{
    Value *src = memCpyInst.getSource();
    Value *dst = memCpyInst.getDest();
    Value *n = memCpyInst.getLength();

    // get src ptees
    // create a dummy pointee if no src ptees
    set<ObjLoc> srcPtees = getPteesOfValPtr(this->dataLayout, this->ptoRecords, this->context, src, true);

    // get dst ptees
    // create a dummy pointee if no dst ptees
    set<ObjLoc> dstPtees = getPteesOfValPtr(this->dataLayout, this->ptoRecords, this->context, dst, true);

    // TODO: update pto records. However, this is rare for memcpy

    // DDG
    processMemCpy(this->ddg, this->context, &memCpyInst, src, srcPtees, dst, dstPtees, n);
}

void FunctionVisitor::visitGetElementPtrInst(GetElementPtrInst &gepInst)
{
    int64_t offset = 0;
    if (gepInst.hasAllConstantIndices())
    {
        APInt ap_offset(64, 0, true);
        bool success = gepInst.accumulateConstantOffset(*this->dataLayout, ap_offset);
        assert(success);
        offset = ap_offset.getSExtValue();
    }
    else // TODO model variable GEP indices
    {
        // iterate over indices
        vector<long> indices;
        for (auto i = gepInst.idx_begin(); i != gepInst.idx_end(); ++i)
        {
            if (ConstantInt *constantInt = dyn_cast<ConstantInt>(*i))
            {
                long index = constantInt->getSExtValue();
                indices.push_back(index);
            }
            else
            {
                // TODO: variable index
                indices.push_back(0); // TODO: better solution
            }
        }
    }

    // src element type
    Type *srcElementTy = gepInst.getSourceElementType();

    // pointer operand
    Value *ptr = gepInst.getPointerOperand();

    // get ptr ptees
    // create a dummy pointee if the castInst cast a pointer that has no pointee
    set<ObjLoc> ptrPtees = getPteesOfValPtr(this->dataLayout, this->ptoRecords, this->context, ptr, true);
    set<ObjLoc> gepPtees;
    for (const ObjLoc &ptrPtee : ptrPtees)
    {
        // shift offset
        ObjLoc gepPtee(ptrPtee.objId, ptrPtee.offset + offset);
        gepPtees.insert(gepPtee);
    }

    // set ptr ptees
    this->ptoRecords.addPteesForValPtr(this->context, &gepInst, gepPtees);

    // DDG
    processGEP(this->ddg, this->context, ptr, &gepInst);
}

void FunctionVisitor::visitCastInst(CastInst &castInst)
{
    Value *src = castInst.getOperand(0);

    // create a dummy pointee if no src ptees and castInst is a pointer
    bool createDummyPointee = castInst.getType()->isPointerTy();

    set<ObjLoc> srcPtees = getPteesOfValPtr(this->dataLayout, this->ptoRecords, this->context, src, createDummyPointee);
    this->ptoRecords.addPteesForValPtr(this->context, &castInst, srcPtees);

    // DDG
    processCast(this->ddg, this->context, src, &castInst);
}

void FunctionVisitor::visitCallInst(CallInst &callInst)
{
    set<Function *> calledFuncs; // There may exist many called functions. The set can contain either declarations or definitions

    Function *calledFunc = getCalledFunction(&callInst);
    if (calledFunc) // direct call
    {
        calledFuncs.insert(calledFunc);
    }
    else
    {
        Function *currFunc = this->context.getCurrentFunction();
        std::pair<Function *, CallInst *> caller(currFunc, &callInst);
        if (this->callGraph.find(caller) != this->callGraph.end())
        {
            calledFuncs = this->callGraph.at(caller);
        }
        else
        {
            return; // return if no called function found
        }
    }

    for (Function *each : calledFuncs)
    {
        this->handleCallFunction(callInst, each);
    }
}

static const set<string> kernelAllocations{"kmalloc", "kzalloc", "kvmalloc"}; // kernel allocation functions

static set<string> initHookedFuncNames()
{
    set<string> funcs = kernelAllocations;
    funcs.insert("_copy_to_user");
    funcs.insert("printk");
    funcs.insert("kfree"); // TODO: what to do with kfree?
    return funcs;
}

static set<string> hookedFuncNames = initHookedFuncNames(); // all hooked functions

void FunctionVisitor::handleCallFunction(CallInst &callInst, Function *calledFunc)
{
    // intrinsic funciton
    if (calledFunc->isIntrinsic())
    {
    }
    else
    {
        string calledFuncName = calledFunc->getName().str();

        // is a hooked function
        if (hookedFuncNames.find(calledFuncName) != hookedFuncNames.end())
        {
            if (kernelAllocations.find(calledFuncName) != kernelAllocations.end())
            {
                ObjId objId = ObjectManager::createHeapObject(InstLoc(this->context, &callInst));
                this->ptoRecords.addPteesForValPtr(this->context, &callInst, set<ObjLoc>{ObjLoc(objId, 0)});
            }
            else if (calledFuncName == "_copy_to_user")
            {
                // from
                Value *from = callInst.getArgOperand(1);
                set<ObjLoc> fromPtees = getPteesOfValPtr(this->dataLayout, this->ptoRecords, this->context, from, true);

                // n
                Value *nVal = callInst.getArgOperand(2);

                processCopyOut(this->ddg, this->context, &callInst, from, fromPtees, nVal);
            }
            else if (calledFuncName == "printk")
            {
                User::op_iterator arg = callInst.arg_begin();
                User::op_iterator argEnd = callInst.arg_end();

                assert(arg != argEnd); // have a least one argument
                Value *fmtStrcVal = (*arg).get()->stripPointerCasts(); // argument 0 is a format specifier
                string fmtStr = getPrintkFmtStr(fmtStrcVal);
                if (fmtStr.empty()) // cannot statically get format string
                {
                    return;
                }
                vector<bool> isCopyOutFmtSpecs = getIfIsCopyOutFmtSpec(fmtStr);

                arg++; // starting from argument 1
                unsigned i = 0;
                map<Value *, set<ObjLoc>> argPteesMap;
                for (; arg != argEnd; arg++, i++)
                {
                    Value *argVal = (*arg).get();
                    if (isCopyOutFmtSpecs[i])
                    {
                        // copy out
                        set<ObjLoc> argPtees = getPteesOfValPtr(this->dataLayout, this->ptoRecords, this->context, argVal, true);
                        argPteesMap[argVal] = argPtees;
                    }
                    else
                    {
                        // value out
                        processValueOut(this->ddg, this->context, argVal, &callInst);
                    }
                }
                processPrintk(this->ddg, this->context, argPteesMap, &callInst);
            }
        }

        // normal functions
        else if (this->context.getDepth() < maxCallDepth)
        {
            // should has definition
            calledFunc = this->glbState.getFuncDef(calledFunc);
            if (!calledFunc)
            {
                return;
            }
            Context calleeCtx = this->context.add(&callInst, calledFunc);

            // set up points-to for arguments and parameters
            User::op_iterator arg = callInst.arg_begin();
            User::op_iterator argEnd = callInst.arg_end();
            Function::arg_iterator parm = calledFunc->arg_begin();
            Function::arg_iterator parmEnd = calledFunc->arg_end();
            for (; arg != argEnd && parm != parmEnd; arg++, parm++)
            {
                Value *argVal = (*arg).get();

                // create a dummy pointee if argVal is a pointer and it has no ptees
                bool createDummyPointee = argVal->getType()->isPointerTy();
                set<ObjLoc> argPtees = getPteesOfValPtr(this->dataLayout, this->ptoRecords, this->context, argVal, createDummyPointee);

                // update pto records for parameters
                this->ptoRecords.addPteesForValPtr(calleeCtx, &(*parm), argPtees);

                // DDG
                processCall(this->ddg, this->context, argVal, calleeCtx, &(*parm));
            }

            // visit called function
            FunctionVisitor vis(this->glbState, calleeCtx, this->callGraph, this->ptoRecords, this->ddg);
            vis.analyze();
#ifdef NUM_VISITED_BBS
            this->numVisitedBBs += vis.numVisitedBBs;
#endif
        }
    }
}

void FunctionVisitor::visitReturnInst(ReturnInst &retInst)
{
    // No need to handle the retInst of entry function
    if (this->context.getDepth() == 1)
    {
        return;
    }

    Value *retVal = retInst.getReturnValue();
    if (retVal)
    {
        // create a dummy pointee if retVal is a pointer and it has no ptees
        bool createDummyPointee = retVal->getType()->isPointerTy();
        set<ObjLoc> retValPtees = getPteesOfValPtr(this->dataLayout, this->ptoRecords, this->context, retVal, createDummyPointee);

        InstLoc callerInstLoc = this->context.getCallerInstLoc();
        this->ptoRecords.addPteesForValPtr(callerInstLoc.context, callerInstLoc.inst, retValPtees);

        // DDG
        processRet(this->ddg, this->context, retVal, callerInstLoc.context, callerInstLoc.inst);
    }
    else
    {
        // return void?
    }
}

#ifdef NUM_VISITED_BBS
uint64_t FunctionVisitor::getNumVisitedBBs()
{
    return this->numVisitedBBs;
}
#endif

string getPrintkFmtStr(llvm::Value *printkFirstArg)
{
    GlobalVariable *fmtStrGlbVar = dyn_cast<GlobalVariable>(printkFirstArg);
    if (!fmtStrGlbVar) // most likely a global variable
    {
        return std::string(); // also can be phi node
    }
    ConstantDataArray *fmtStrConstArr = dyn_cast<ConstantDataArray>(fmtStrGlbVar->getInitializer());
    if (!fmtStrConstArr) // most likely a constant data array
    {
        return std::string(); // also can be zero initializer
    }
    return fmtStrConstArr->getAsCString().str();
}

vector<bool> getIfIsCopyOutFmtSpec(string fmtStr)
{
    vector<bool> res;
    vector<unsigned> fmtSpecIndices = findAllOccurrences(fmtStr, '%');
    for (unsigned fmtSpecIdx : fmtSpecIndices)
    {
        if (fmtStr[fmtSpecIdx + 1] == 's') // %s
        {
            res.push_back(true);
        }
        else
        {
            res.push_back(false);
        }
    }
    return res;
}

set<ObjLoc> getPteesOfValPtr(const DataLayout *dataLayout, PointsToRecords &ptoRecords, Context context, Value *pointer, bool createDummyPointee)
{
    if (isa<Instruction>(pointer) || isa<Argument>(pointer))
    {
        set<ObjLoc> pointees = ptoRecords.getPteesOfValPtr(context, pointer);
        if (pointees.empty() && createDummyPointee)
        {
            ObjId objId = ObjectManager::createDummyObject();
            pointees = set<ObjLoc>{ObjLoc(objId, 0)};
            ptoRecords.addPteesForValPtr(context, pointer, pointees);
        }
        return pointees;
    }
    else if (GlobalVariable *glbVar = dyn_cast<GlobalVariable>(pointer))
    {
        ObjId objId = ObjectManager::getOrCreateGlobalObject(glbVar);
        return set<ObjLoc>{ObjLoc(objId, 0)};
    }
    else if (ConstantExpr *constantExpr = dyn_cast<ConstantExpr>(pointer))
    {
        switch (constantExpr->getOpcode())
        {
        case Instruction::GetElementPtr:
        {
            GEPOperator *gepOpr = cast<GEPOperator>(constantExpr);
            APInt ap_offset(64, 0, true);
            bool success = gepOpr->accumulateConstantOffset(*dataLayout, ap_offset);
            assert(success);
            int64_t offset = ap_offset.getSExtValue();
            set<ObjLoc> ptrPtees = getPteesOfValPtr(dataLayout, ptoRecords, context, gepOpr->getPointerOperand(), createDummyPointee);
            set<ObjLoc> gepPtees;
            for (ObjLoc ptrPtee : ptrPtees)
            {
                // shift offset
                ObjLoc gepPtee(ptrPtee.objId, ptrPtee.offset + offset);
                gepPtees.insert(gepPtee);
            }
            return gepPtees;
            break;
        }
        case Instruction::BitCast:
            break;
        case Instruction::PtrToInt:
            break;
        case Instruction::IntToPtr:
            break;
        case Instruction::Trunc:
            break;
        case Instruction::Add:
            break;
        case Instruction::Sub:
            break;
        case Instruction::SDiv:
            break;
        case Instruction::Or:
            break;
        default:
            outs() << "unhandled opcode: " << constantExpr->getOpcode() << "\n";
            assert(0);
            break;
        }
    }
    else if (isa<ConstantData>(pointer))
    {
    }
    else if (isa<Function>(pointer))
    {
    }
    else if (isa<BlockAddress>(pointer))
    {
    }
    else
    {
        outs() << "unhandled value: " << *pointer << "\n";
        assert(0);
    }
    return set<ObjLoc>();
}

set<ObjLoc> getPteesOfObjPtr(PointsToRecords &ptoRecords, ObjLoc pointer, InstLoc currLoc, bool createDummyPointee, InstLoc dummyPtoUpdateLoc)
{
    set<ObjLoc> pointees = ptoRecords.getPteesOfObjPtr(pointer, currLoc);
    if (pointees.empty() && createDummyPointee)
    {
        ObjId objId = ObjectManager::createDummyObject();
        pointees = set<ObjLoc>{ObjLoc(objId, 0)};
        ptoRecords.addPteesForObjPtr(pointer, pointees, dummyPtoUpdateLoc);
    }
    return pointees;
}
