#pragma once

#include "llvm/IR/Value.h"

#include "InstLoc.h"
#include "Object.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <map>
#include <set>
#include <string>

typedef unsigned long NodeId;

// Node of Data Dependence Graph

// Different kinds of nodes
enum NodeKind
{
    value,
    load,
    store,
    mcpy,
    copyout,
    valout,
};

// Value node
class ValNode
{
public:
    NodeId id;
    Context ctx;
    llvm::Value *val;

    ValNode(NodeId id, Context ctx, llvm::Value *val);
};

typedef boost::multi_index::multi_index_container<
    ValNode,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            boost::multi_index::member<ValNode, NodeId, &ValNode::id>>,
        boost::multi_index::ordered_unique<
            boost::multi_index::composite_key<
                ValNode,
                boost::multi_index::member<ValNode, Context, &ValNode::ctx>,
                boost::multi_index::member<ValNode, llvm::Value *, &ValNode::val>>>>>
    ValNodeSet;

// Load node
class LoadNode
{
public:
    NodeId id;
    Context ctx;
    llvm::LoadInst *loadInst;

    LoadNode(NodeId id, Context ctx, llvm::LoadInst *loadInst);
};

typedef boost::multi_index::multi_index_container<
    LoadNode,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            boost::multi_index::member<LoadNode, NodeId, &LoadNode::id>>,
        boost::multi_index::ordered_unique<
            boost::multi_index::composite_key<
                LoadNode,
                boost::multi_index::member<LoadNode, Context, &LoadNode::ctx>,
                boost::multi_index::member<LoadNode, llvm::LoadInst *, &LoadNode::loadInst>>>>>
    LoadNodeSet;

// Store node
class StoreNode
{
public:
    NodeId id;
    Context ctx;
    llvm::StoreInst *storeInst;

    StoreNode(NodeId id, Context ctx, llvm::StoreInst *storeInst);
};

typedef boost::multi_index::multi_index_container<
    StoreNode,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            boost::multi_index::member<StoreNode, NodeId, &StoreNode::id>>,
        boost::multi_index::ordered_unique<
            boost::multi_index::composite_key<
                StoreNode,
                boost::multi_index::member<StoreNode, Context, &StoreNode::ctx>,
                boost::multi_index::member<StoreNode, llvm::StoreInst *, &StoreNode::storeInst>>>>>
    StoreNodeSet;

// MemCpy node
class MemCpyNode
{
public:
    NodeId id;
    Context ctx;
    llvm::Instruction *memCpyInst;

    MemCpyNode(NodeId id, Context ctx, llvm::Instruction *memCpyInst);
};

typedef boost::multi_index::multi_index_container<
    MemCpyNode,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            boost::multi_index::member<MemCpyNode, NodeId, &MemCpyNode::id>>,
        boost::multi_index::ordered_unique<
            boost::multi_index::composite_key<
                MemCpyNode,
                boost::multi_index::member<MemCpyNode, Context, &MemCpyNode::ctx>,
                boost::multi_index::member<MemCpyNode, llvm::Instruction *, &MemCpyNode::memCpyInst>>>>>
    MemCpyNodeSet;

// Copy-out node
class CopyOutNode
{
public:
    NodeId id;
    Context ctx;
    llvm::CallInst *callInst;

    CopyOutNode(NodeId id, Context ctx, llvm::CallInst *callInst);
};

typedef boost::multi_index::multi_index_container<
    CopyOutNode,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            boost::multi_index::member<CopyOutNode, NodeId, &CopyOutNode::id>>,
        boost::multi_index::ordered_unique<
            boost::multi_index::composite_key<
                CopyOutNode,
                boost::multi_index::member<CopyOutNode, Context, &CopyOutNode::ctx>,
                boost::multi_index::member<CopyOutNode, llvm::CallInst *, &CopyOutNode::callInst>>>>>
    CopyOutNodeSet;

// Value-out node
class ValueOutNode
{
public:
    NodeId id;
    Context ctx;
    llvm::CallInst *callInst;

    ValueOutNode(NodeId id, Context ctx, llvm::CallInst *callInst);
};

typedef boost::multi_index::multi_index_container<
    ValueOutNode,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            boost::multi_index::member<ValueOutNode, NodeId, &ValueOutNode::id>>,
        boost::multi_index::ordered_unique<
            boost::multi_index::composite_key<
                ValueOutNode,
                boost::multi_index::member<ValueOutNode, Context, &ValueOutNode::ctx>,
                boost::multi_index::member<ValueOutNode, llvm::CallInst *, &ValueOutNode::callInst>>>>>
    ValueOutNodeSet;

// Edge of Data Dependence Graph
// Def-use edge
class DefUse
{
public:
    NodeId srcId;
    NodeId dstId;

    DefUse(NodeId srcId, NodeId dstId);
};

typedef boost::multi_index::multi_index_container<
    DefUse,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_non_unique<
            boost::multi_index::member<DefUse, NodeId, &DefUse::srcId>>>>
    DefUseSet;

class LoadRelation
{
public:
    NodeId loadInstId;
    NodeId srcId;
    NodeId valId;
    uint64_t loadSize;

    // tags
    struct LoadInstId {};
    struct SrcId {};
    struct ValId {};

    LoadRelation(NodeId loadInstId, NodeId srcId, NodeId valId, uint64_t loadSize);
};

typedef boost::multi_index::multi_index_container<
    LoadRelation,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            boost::multi_index::tag<LoadRelation::LoadInstId>,
            boost::multi_index::member<LoadRelation, NodeId, &LoadRelation::loadInstId>>,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<LoadRelation::SrcId>,
            boost::multi_index::member<LoadRelation, NodeId, &LoadRelation::srcId>>,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<LoadRelation::ValId>,
            boost::multi_index::member<LoadRelation, NodeId, &LoadRelation::valId>>>>
    LoadRelations;

class LoadPtrPto
{
public:
    NodeId srcId;
    ObjId objId;
    offset_t offset;

    LoadPtrPto(NodeId srcId, ObjId objId, offset_t offset);
};

typedef boost::multi_index::multi_index_container<
    LoadPtrPto,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_non_unique<
            boost::multi_index::member<LoadPtrPto, NodeId, &LoadPtrPto::srcId>>,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::member<LoadPtrPto, ObjId, &LoadPtrPto::objId>>,
        boost::multi_index::ordered_unique<
            boost::multi_index::composite_key<
                LoadPtrPto,
                boost::multi_index::member<LoadPtrPto, NodeId, &LoadPtrPto::srcId>,
                boost::multi_index::member<LoadPtrPto, ObjId, &LoadPtrPto::objId>,
                boost::multi_index::member<LoadPtrPto, offset_t, &LoadPtrPto::offset>>>>>
    LoadPtrPtos;

class StoreRelation
{
public:
    NodeId storeInstId;
    NodeId valId;
    NodeId dstId;
    uint64_t storeSize;

    // tags
    struct StoreInstId {};
    struct ValId {};
    struct DstId {};

    StoreRelation(NodeId storeInstId, NodeId valId, NodeId dstId, uint64_t storeSize);
};

typedef boost::multi_index::multi_index_container<
    StoreRelation,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            boost::multi_index::tag<StoreRelation::StoreInstId>,
            boost::multi_index::member<StoreRelation, NodeId, &StoreRelation::storeInstId>>,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<StoreRelation::ValId>,
            boost::multi_index::member<StoreRelation, NodeId, &StoreRelation::valId>>,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<StoreRelation::DstId>,
            boost::multi_index::member<StoreRelation, NodeId, &StoreRelation::dstId>>>>
    StoreRelations;

class StorePtrPto
{
public:
    NodeId dstId;
    ObjId objId;
    offset_t offset;

    StorePtrPto(NodeId dstId, ObjId objId, offset_t offset);
};

typedef boost::multi_index::multi_index_container<
    StorePtrPto,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_non_unique<
            boost::multi_index::member<StorePtrPto, NodeId, &StorePtrPto::dstId>>,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::member<StorePtrPto, ObjId, &StorePtrPto::objId>>,
        boost::multi_index::ordered_unique<
            boost::multi_index::composite_key<
                StorePtrPto,
                boost::multi_index::member<StorePtrPto, NodeId, &StorePtrPto::dstId>,
                boost::multi_index::member<StorePtrPto, ObjId, &StorePtrPto::objId>,
                boost::multi_index::member<StorePtrPto, offset_t, &StorePtrPto::offset>>>>>
    StorePtrPtos;

class MemCpyRelation
{
public:
    NodeId memCpyInstId;
    NodeId srcId;
    NodeId dstId;
    NodeId nId;

    // tags
    struct SrcId {};
    struct DstId {};
    struct NId {};

    MemCpyRelation(NodeId memCpyInstId, NodeId srcId, NodeId dstId, NodeId nId);
};

typedef boost::multi_index::multi_index_container<
    MemCpyRelation,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<MemCpyRelation::SrcId>,
            boost::multi_index::member<MemCpyRelation, NodeId, &MemCpyRelation::srcId>>,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<MemCpyRelation::DstId>,
            boost::multi_index::member<MemCpyRelation, NodeId, &MemCpyRelation::dstId>>,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<MemCpyRelation::NId>,
            boost::multi_index::member<MemCpyRelation, NodeId, &MemCpyRelation::nId>>>>
    MemCpyRelations;

class CopyOutFromRel
{
public:
    NodeId copyOutInstId;
    NodeId fromId;

    // tags
    struct CopyOutInstId {};
    struct FromId {};

    CopyOutFromRel(NodeId copyOutInstId, NodeId fromId);
};

typedef boost::multi_index::multi_index_container<
    CopyOutFromRel,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<CopyOutFromRel::CopyOutInstId>,
            boost::multi_index::member<CopyOutFromRel, NodeId, &CopyOutFromRel::copyOutInstId>>,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<CopyOutFromRel::FromId>,
            boost::multi_index::member<CopyOutFromRel, NodeId, &CopyOutFromRel::fromId>>>>
    CopyOutFromRels;

class CopyOutNRel
{
public:
    NodeId copyOutInstId;
    NodeId nId;

    // tags
    struct CopyOutInstId {};
    struct NId {};

    CopyOutNRel(NodeId copyOutInstId, NodeId nId);
};

typedef boost::multi_index::multi_index_container<
    CopyOutNRel,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<CopyOutNRel::CopyOutInstId>,
            boost::multi_index::member<CopyOutNRel, NodeId, &CopyOutNRel::copyOutInstId>>,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<CopyOutNRel::NId>,
            boost::multi_index::member<CopyOutNRel, NodeId, &CopyOutNRel::nId>>>>
    CopyOutNRels;

typedef boost::adjacency_list<> Graph;

// Data Dependence Graph
class DDG
{
public:
    DDG();

    NodeId getOrCreateValNode(Context context, llvm::Value *val);
    NodeId getOrCreateLoadNode(Context context, llvm::LoadInst *loadInst);
    NodeId getOrCreateStoreNode(Context context, llvm::StoreInst *storeInst);
    NodeId getOrCreateMemCpyNode(Context context, llvm::Instruction *memCpyInst);
    NodeId getOrCreateCopyOutNode(Context context, llvm::CallInst *callInst);
    NodeId getOrCreateValOutNode(Context context, llvm::CallInst *callInst);
    NodeKind getNodeKind(NodeId nodeId) const;
    std::string nodeId2String(NodeId nodeId, bool hasSrcLoc = true);

    void addDefUseEdge(NodeId src, NodeId dst);
    void addLoadRelation(NodeId loadInstId, NodeId srcId, NodeId valId, std::set<ObjLoc> srcPtees, uint64_t loadSize);
    void addStoreRelation(NodeId storeInstId, NodeId valId, NodeId dstId, std::set<ObjLoc> dstPtees, uint64_t storeSize);
    void addMemCpy(NodeId memCpyInstId, NodeId srcId, NodeId dstId, NodeId nId, std::set<ObjLoc> srcPtees, std::set<ObjLoc> dstPtees);
    void addCopyOutRelation(NodeId copyOutInstId, NodeId fromId, NodeId nId, std::set<ObjLoc> fromPtees);
    std::set<NodeId> getStoreLikeNodesOnRange(ObjId loadObjId, offset_t loadStart, offset_t loadEnd);
    std::set<NodeId> getStoreLikeNodesOnObj(ObjId objId);
    std::set<NodeId> getLoadLikeNodesOnObj(ObjId objId);
    void dealWithStore(NodeId nodeId);

    // should be called after finishing building the graph
    void addAlias(ObjId objId1, ObjId objId2);

    void getAffectedNodes(NodeId nodeId, std::set<NodeId> &loadNodes, std::set<NodeId> &storeNodes);

    uint64_t getNumNodes(); // the number of nodes in the graph
    uint64_t getNumEdges(); // the number of edges in the graph
    std::string toString();

    void bfs(NodeId startId, std::vector<NodeId> loadLayers);

private:
    // next node id to be used
    NodeId currNodeId;

    // nodes
    std::map<NodeId, NodeKind> nodeKindMap;
    ValNodeSet valNodeSet;
    LoadNodeSet loadNodeSet;
    StoreNodeSet storeNodeSet;
    MemCpyNodeSet memCpyNodeSet;
    CopyOutNodeSet copyOutNodeSet;
    ValueOutNodeSet valueOutNodeSet;

    // edges
    DefUseSet defUseSet; // info edges (among values)
    LoadRelations loadRelations; // info + ptr edges
    StoreRelations storeRelations; // info + ptr edges
    MemCpyRelations memCpyRelations; // ptr edges
    CopyOutFromRels copyOutFromRels; // ptr edges
    CopyOutNRels copyOutNRels; // ptr edges

    // alias info
    LoadPtrPtos loadPtrPtos;
    StorePtrPtos storePtrPtos;

    // graph
    Graph graph; // info edges (among values + loads + stores + ...)

    NodeId genNodeId();
};

class my_visitor : public boost::default_bfs_visitor
{
public:
    DDG &ddg;
    std::set<NodeId> &reachedCopyOutNodes;
    std::set<NodeId> &unconnectedStores;
    std::set<NodeId> &affectedLoads;
    std::set<NodeId> &affectedStores;

    my_visitor(DDG &ddg, std::set<NodeId> &reachedCopyOutNodes, std::set<NodeId> &unconnectedStores, std::set<NodeId> &affectedLoads, std::set<NodeId> &affectedStores) : ddg(ddg), reachedCopyOutNodes(reachedCopyOutNodes), unconnectedStores(unconnectedStores), affectedLoads(affectedLoads), affectedStores(affectedStores){};
    void discover_vertex(Graph::vertex_descriptor v, const Graph &g);
};

void processLoad(DDG &ddg, Context context, std::set<ObjLoc> srcPtees, llvm::LoadInst *loadInst, llvm::Value *src, uint64_t loadSize);
void processStore(DDG &ddg, Context context, llvm::Value *val, std::set<ObjLoc> dstPtees, llvm::StoreInst *storeInst, llvm::Value *dst, uint64_t storeSize);
void processMemCpy(DDG &ddg, Context context, llvm::Instruction *memCpyInst, llvm::Value *src, std::set<ObjLoc> srcPtees, llvm::Value *dst, std::set<ObjLoc> dstPtees, llvm::Value *n);
void processGEP(DDG &ddg, Context context, llvm::Value *ptr, llvm::GetElementPtrInst *gepInst);
void processCast(DDG &ddg, Context context, llvm::Value *src, llvm::CastInst *castInst);
void processCopyOut(DDG &ddg, Context context, llvm::CallInst *callInst, llvm::Value *from, std::set<ObjLoc> fromPtees, llvm::Value *n);
void processPrintk(DDG &ddg, Context context, std::map<llvm::Value *, std::set<ObjLoc>> argPteesMap, llvm::CallInst *callInst);
void processValueOut(DDG &ddg, Context context, llvm::Value *val, llvm::CallInst *callInst);
void processCall(DDG &ddg, Context callerCtx, llvm::Value *arg, Context calleeCtx, llvm::Argument *parm);
void processRet(DDG &ddg, Context calleeCtx, llvm::Value *retVal, Context callerCtx, llvm::Instruction *callInst);
void processPHINode(DDG &ddg, Context context, std::set<llvm::Value *> incomingValues, llvm::PHINode *phiNode);
void processBiOp(DDG &ddg, Context context, llvm::Value *opd0, llvm::Value *opd1, llvm::BinaryOperator *biOp);
