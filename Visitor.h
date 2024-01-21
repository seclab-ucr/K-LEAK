#pragma once

#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstVisitor.h"

#include "CallGraph.h"
#include "DDG.h"
#include "GlobalState.h"
#include "InstLoc.h"
#include "PointsTo.h"

#define NUM_VISITED_BBS

class FunctionVisitor : public llvm::InstVisitor<FunctionVisitor>
{
private:
    GlobalState &glbState;
    Context context;
    const llvm::DataLayout *dataLayout;
    const CGMap &callGraph; // precomputed call graph
    PointsToRecords &ptoRecords;
    DDG &ddg;
#ifdef NUM_VISITED_BBS
    uint64_t numVisitedBBs;
#endif

public:
    FunctionVisitor(GlobalState &glbState, Context context, const CGMap &callGraph, PointsToRecords &ptoRecords, DDG &ddg);

    void analyze();
    void visitAllocaInst(llvm::AllocaInst &allocaInst);
    void visitBinaryOperator(llvm::BinaryOperator &biOp);
    void visitPHINode(llvm::PHINode &phiNode);
    void visitLoadInst(llvm::LoadInst &loadInst);
    void visitStoreInst(llvm::StoreInst &storeInst);
    void visitMemCpyInst(llvm::MemCpyInst &memCpyInst);
    void visitGetElementPtrInst(llvm::GetElementPtrInst &gepInst);
    void visitCastInst(llvm::CastInst &castInst);
    void visitCallInst(llvm::CallInst &callInst);
    void visitReturnInst(llvm::ReturnInst &retInst);
    // TODO: icmp, conditional/unconditional br
#ifdef NUM_VISITED_BBS
    uint64_t getNumVisitedBBs();
#endif
private:
    void handleCallFunction(llvm::CallInst &callInst, llvm::Function *calledFunc); // invoked by visitCallInst
};

// printk
std::string getPrintkFmtStr(llvm::Value *printkFirstArg);
std::vector<bool> getIfIsCopyOutFmtSpec(std::string fmtStr);

std::set<ObjLoc> getPteesOfValPtr(const llvm::DataLayout *dataLayout, PointsToRecords &ptoRecords, Context context, llvm::Value *pointer, bool createDummyPointee);
std::set<ObjLoc> getPteesOfObjPtr(PointsToRecords &ptoRecords, ObjLoc pointer, InstLoc currLoc, bool createDummyPointee, InstLoc dummyPtoUpdateLoc);
