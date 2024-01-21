#include "llvm/Support/raw_ostream.h"

#include "DDG.h"
#include "Utils.h"

#include <boost/range.hpp>

#include <iostream>

using namespace llvm;
using namespace std;

ValNode::ValNode(NodeId id, Context ctx, Value *val) : id(id), ctx(ctx), val(val)
{
}

LoadNode::LoadNode(NodeId id, Context ctx, LoadInst *loadInst) : id(id), ctx(ctx), loadInst(loadInst)
{
}

StoreNode::StoreNode(NodeId id, Context ctx, StoreInst *storeInst) : id(id), ctx(ctx), storeInst(storeInst)
{
}

MemCpyNode::MemCpyNode(NodeId id, Context ctx, llvm::Instruction *memCpyInst) : id(id), ctx(ctx), memCpyInst(memCpyInst)
{
}

CopyOutNode::CopyOutNode(NodeId id, Context ctx, llvm::CallInst *callInst) : id(id), ctx(ctx), callInst(callInst)
{
}

ValueOutNode::ValueOutNode(NodeId id, Context ctx, llvm::CallInst *callInst) : id(id), ctx(ctx), callInst(callInst)
{
}

DefUse::DefUse(NodeId srcId, NodeId dstId) : srcId(srcId), dstId(dstId)
{
}

LoadRelation::LoadRelation(NodeId loadInstId, NodeId srcId, NodeId valId, uint64_t loadSize) : loadInstId(loadInstId), srcId(srcId), valId(valId), loadSize(loadSize)
{
}

LoadPtrPto::LoadPtrPto(NodeId srcId, ObjId objId, offset_t offset) : srcId(srcId), objId(objId), offset(offset)
{
}

StoreRelation::StoreRelation(NodeId storeInstId, NodeId valId, NodeId dstId, uint64_t storeSize) : storeInstId(storeInstId), valId(valId), dstId(dstId), storeSize(storeSize)
{
}

StorePtrPto::StorePtrPto(NodeId dstId, ObjId objId, offset_t offset) : dstId(dstId), objId(objId), offset(offset)
{
}

MemCpyRelation::MemCpyRelation(NodeId memCpyInstId, NodeId srcId, NodeId dstId, NodeId nId) : memCpyInstId(memCpyInstId), srcId(srcId), dstId(dstId), nId(nId)
{
}

CopyOutFromRel::CopyOutFromRel(NodeId copyOutInstId, NodeId fromId) : copyOutInstId(copyOutInstId), fromId(fromId)
{
}

CopyOutNRel::CopyOutNRel(NodeId copyOutInstId, NodeId nId) : copyOutInstId(copyOutInstId), nId(nId)
{
}

DDG::DDG() : currNodeId(0)
{
}

NodeId DDG::getOrCreateValNode(Context context, Value *val)
{
    auto &ctxValView = this->valNodeSet.get<1>();
    auto it = ctxValView.find(boost::make_tuple(context, val));
    if (it != ctxValView.end()) // get
    {
        return it->id;
    }
    else // create
    {
        NodeId id = this->genNodeId();
        this->nodeKindMap[id] = NodeKind::value;
        this->valNodeSet.insert(ValNode(id, context, val));
        return id;
    }
}

NodeId DDG::getOrCreateLoadNode(Context context, LoadInst *loadInst)
{
    auto &ctxInstView = this->loadNodeSet.get<1>();
    auto it = ctxInstView.find(boost::make_tuple(context, loadInst));
    if (it != ctxInstView.end()) // get
    {
        return it->id;
    }
    else // create
    {
        NodeId id = this->genNodeId();
        this->nodeKindMap[id] = NodeKind::load;
        this->loadNodeSet.insert(LoadNode(id, context, loadInst));
        return id;
    }
}

NodeId DDG::getOrCreateStoreNode(Context context, StoreInst *storeInst)
{
    auto &ctxInstView = this->storeNodeSet.get<1>();
    auto it = ctxInstView.find(boost::make_tuple(context, storeInst));
    if (it != ctxInstView.end()) // get
    {
        return it->id;
    }
    else // create
    {
        NodeId id = this->genNodeId();
        this->nodeKindMap[id] = NodeKind::store;
        this->storeNodeSet.insert(StoreNode(id, context, storeInst));
        return id;
    }
}

NodeId DDG::getOrCreateMemCpyNode(Context context, llvm::Instruction *memCpyInst)
{
    auto &ctxInstView = this->memCpyNodeSet.get<1>();
    auto it = ctxInstView.find(boost::make_tuple(context, memCpyInst));
    if (it != ctxInstView.end()) // get
    {
        return it->id;
    }
    else // create
    {
        NodeId id = this->genNodeId();
        this->nodeKindMap[id] = NodeKind::mcpy;
        this->memCpyNodeSet.insert(MemCpyNode(id, context, memCpyInst));
        return id;
    }
}

NodeId DDG::getOrCreateCopyOutNode(Context context, CallInst *callInst)
{
    auto &ctxInstView = this->copyOutNodeSet.get<1>();
    auto it = ctxInstView.find(boost::make_tuple(context, callInst));
    if (it != ctxInstView.end()) // get
    {
        return it->id;
    }
    else // create
    {
        NodeId id = this->genNodeId();
        this->nodeKindMap[id] = NodeKind::copyout;
        this->copyOutNodeSet.insert(CopyOutNode(id, context, callInst));
        return id;
    }
}

NodeId DDG::getOrCreateValOutNode(Context context, llvm::CallInst *callInst)
{
    auto &ctxInstView = this->valueOutNodeSet.get<1>();
    auto it = ctxInstView.find(boost::make_tuple(context, callInst));
    if (it != ctxInstView.end()) // get
    {
        return it->id;
    }
    else // create
    {
        NodeId id = this->genNodeId();
        this->nodeKindMap[id] = NodeKind::valout;
        this->valueOutNodeSet.insert(ValueOutNode(id, context, callInst));
        return id;
    }
}

inline NodeKind DDG::getNodeKind(NodeId nodeId) const
{
    return this->nodeKindMap.at(nodeId);
}

string DDG::nodeId2String(NodeId nodeId, bool hasSrcLoc)
{
    string fn; // filename
    uint32_t ln; // line number
    string ctxStr;
    string name;
    switch (this->getNodeKind(nodeId))
    {
    case NodeKind::value:
    {
        auto itVal = this->valNodeSet.find(nodeId);
        Value *val = itVal->val;
        if (Instruction *I = dyn_cast<Instruction>(val))
        {
            std::tie(fn, ln) = getInstFileAndLine(I);
        }
        ctxStr = itVal->ctx.toString();
        name = getName(val);
        break;
    }
    case NodeKind::load:
    {
        auto itLoad = this->loadNodeSet.find(nodeId);
        LoadInst *loadInst = itLoad->loadInst;
        std::tie(fn, ln) = getInstFileAndLine(loadInst);
        ctxStr = itLoad->ctx.toString();
        name = getName(loadInst, false);
        break;
    }
    case NodeKind::store:
    {
        auto itStore = this->storeNodeSet.find(nodeId);
        StoreInst *storeInst = itStore->storeInst;
        std::tie(fn, ln) = getInstFileAndLine(storeInst);
        ctxStr = itStore->ctx.toString();
        name = getName(storeInst, false);
        break;
    }
    case NodeKind::mcpy:
    {
        auto itMemCpy = this->memCpyNodeSet.find(nodeId);
        Instruction *memCpyInst = itMemCpy->memCpyInst;
        std::tie(fn, ln) = getInstFileAndLine(memCpyInst);
        ctxStr = itMemCpy->ctx.toString();
        name = getName(memCpyInst, false);
        break;
    }
    case NodeKind::copyout:
    {
        auto itCopyOut = this->copyOutNodeSet.find(nodeId);
        CallInst *callInst = itCopyOut->callInst;
        std::tie(fn, ln) = getInstFileAndLine(callInst);
        ctxStr = itCopyOut->ctx.toString();
        name = getName(callInst, false);
        break;
    }
    case NodeKind::valout:
    {
        auto itValueOut = this->valueOutNodeSet.find(nodeId);
        CallInst *callInst = itValueOut->callInst;
        std::tie(fn, ln) = getInstFileAndLine(callInst);
        ctxStr = itValueOut->ctx.toString();
        name = getName(callInst, false);
        break;
    }
    default:
        return "error";
    }
    string res;
    if (hasSrcLoc && !fn.empty())
    {
        res += "(" + fn + ":" + to_string(ln) + ") ";
    }
    res += "<" + ctxStr + ">";
    res += name;
    return res;
}

void DDG::addDefUseEdge(NodeId src, NodeId dst)
{
    // TODO: deduplication
    this->defUseSet.insert(DefUse(src, dst)); // TODO: redundant

    // info edge
    boost::add_edge(src, dst, this->graph);
}

void DDG::addLoadRelation(NodeId loadInstId, NodeId srcId, NodeId valId, set<ObjLoc> srcPtees, uint64_t loadSize)
{
    // TODO: deduplication
    this->loadRelations.insert(LoadRelation(loadInstId, srcId, valId, loadSize)); // TODO: redundant

    // info edge
    boost::add_edge(loadInstId, valId, this->graph);

    // alias info
    for (ObjLoc srcPtee : srcPtees)
    {
        this->loadPtrPtos.insert(LoadPtrPto(srcId, srcPtee.objId, srcPtee.offset));
    }

    // connect store-like nodes to load
    for (ObjLoc srcPtee : srcPtees)
    {
        ObjId objId = srcPtee.objId;
        offset_t start = srcPtee.offset;
        offset_t end = start + loadSize;

        set<NodeId> storeLikeNodes = this->getStoreLikeNodesOnRange(objId, start, end);
        for (NodeId storeLikeNode : storeLikeNodes)
        {
            boost::add_edge(storeLikeNode, loadInstId, this->graph);
        }
    }
}

void DDG::addStoreRelation(NodeId storeInstId, NodeId valId, NodeId dstId, set<ObjLoc> dstPtees, uint64_t storeSize)
{
    // TODO: deduplication
    this->storeRelations.insert(StoreRelation(storeInstId, valId, dstId, storeSize)); // TODO: redundant

    // info edge
    boost::add_edge(valId, storeInstId, this->graph);

    // alias info
    for (ObjLoc dstPtee : dstPtees)
    {
        this->storePtrPtos.insert(StorePtrPto(dstId, dstPtee.objId, dstPtee.offset));
    }
}

void DDG::addMemCpy(NodeId memCpyInstId, NodeId srcId, NodeId dstId, NodeId nId, std::set<ObjLoc> srcPtees, std::set<ObjLoc> dstPtees)
{
    this->memCpyRelations.insert(MemCpyRelation(memCpyInstId, srcId, dstId, nId));

    // alias info
    for (ObjLoc srcPtee : srcPtees)
    {
        this->loadPtrPtos.insert(LoadPtrPto(srcId, srcPtee.objId, srcPtee.offset));
    }
    for (ObjLoc dstPtee : dstPtees)
    {
        this->storePtrPtos.insert(StorePtrPto(dstId, dstPtee.objId, dstPtee.offset));
    }

    // connect store-like nodes to memCpy
    for (ObjLoc srcPtee : srcPtees)
    {
        ObjId objId = srcPtee.objId;
        offset_t start = srcPtee.offset;
        offset_t end;
        auto it = this->valNodeSet.find(nId);
        Value *n = it->val;
        if (ConstantInt *constantInt = dyn_cast<ConstantInt>(n))
        {
            int64_t copySize = constantInt->getSExtValue();
            end = start + copySize;
        }
        else
        {
            end = -1;
        }

        set<NodeId> storeLikeNodes = this->getStoreLikeNodesOnRange(objId, start, end);
        for (NodeId storeLikeNode : storeLikeNodes)
        {
            boost::add_edge(storeLikeNode, memCpyInstId, this->graph);
        }
    }
}

void DDG::addCopyOutRelation(NodeId copyOutInstId, NodeId fromId, NodeId nId, set<ObjLoc> fromPtees)
{
    // TODO: deduplication
    this->copyOutFromRels.insert(CopyOutFromRel(copyOutInstId, fromId));
    if (nId != -1)
    {
        this->copyOutNRels.insert(CopyOutNRel(copyOutInstId, nId));
    }

    // alias info
    for (ObjLoc fromPtee : fromPtees)
    {
        this->loadPtrPtos.insert(LoadPtrPto(fromId, fromPtee.objId, fromPtee.offset));
    }

    // connect store-like nodes to copyOut
    int64_t n; // copy out length
    if (nId != -1)
    {
        Value *nVal = this->valNodeSet.find(nId)->val;
        if (ConstantInt *constantInt = dyn_cast<ConstantInt>(nVal))
        {
            n = constantInt->getSExtValue();
        }
        else
        {
            n = -1;
        }
    }
    else
    {
        n = -1;
    }

    for (ObjLoc fromPtee : fromPtees)
    {
        ObjId objId = fromPtee.objId;
        offset_t start = fromPtee.offset;
        offset_t end = start + (n >= 0) ? n : -1;

        set<NodeId> storeLikeNodes = this->getStoreLikeNodesOnRange(objId, start, end);
        for (NodeId storeLikeNode : storeLikeNodes)
        {
            boost::add_edge(storeLikeNode, copyOutInstId, this->graph);
        }
    }
}

set<NodeId> DDG::getStoreLikeNodesOnRange(ObjId objId, offset_t start, offset_t end)
{
    set<NodeId> storeLikeNodes;
    for (auto &storePtrPto : boost::make_iterator_range(this->storePtrPtos.get<1>().equal_range(objId)))
    {
        // store nodes
        for (auto &storeRelation : boost::make_iterator_range(this->storeRelations.get<StoreRelation::DstId>().equal_range(storePtrPto.dstId)))
        {
            if (overlap(start, end, storePtrPto.offset, storePtrPto.offset + storeRelation.storeSize))
            {
                storeLikeNodes.insert(storeRelation.storeInstId);
            }
        }

        // memcpy nodes
        for (auto &memCpyRelation : boost::make_iterator_range(this->memCpyRelations.get<MemCpyRelation::DstId>().equal_range(storePtrPto.dstId)))
        {
            NodeId nId = memCpyRelation.nId;
            auto it = this->valNodeSet.find(nId);
            Value *n = it->val;
            offset_t memCpyStart = storePtrPto.offset;
            offset_t memCpyEnd;
            if (ConstantInt *constantInt = dyn_cast<ConstantInt>(n))
            {
                int64_t copySize = constantInt->getSExtValue();
                memCpyEnd = memCpyStart + copySize;
            }
            else
            {
                memCpyEnd = -1;
            }
            if (overlap(start, end, memCpyStart, memCpyEnd))
            {
                storeLikeNodes.insert(memCpyRelation.memCpyInstId);
            }
        }
    }
    return storeLikeNodes;
}

set<NodeId> DDG::getStoreLikeNodesOnObj(ObjId objId)
{
    set<NodeId> storeLikeNodes;
    for (auto &storePtrPto : boost::make_iterator_range(this->storePtrPtos.get<1>().equal_range(objId)))
    {
        // store nodes
        for (auto &storeRelation : boost::make_iterator_range(this->storeRelations.get<StoreRelation::DstId>().equal_range(storePtrPto.dstId)))
        {
            storeLikeNodes.insert(storeRelation.storeInstId);
        }

        // memCpy nodes
        for (auto &memCpyRelation : boost::make_iterator_range(this->memCpyRelations.get<MemCpyRelation::DstId>().equal_range(storePtrPto.dstId)))
        {
            storeLikeNodes.insert(memCpyRelation.memCpyInstId);
        }
    }
    return storeLikeNodes;
}

set<NodeId> DDG::getLoadLikeNodesOnObj(ObjId objId)
{
    set<NodeId> res;
    for (auto &loadPtrPto : boost::make_iterator_range(this->loadPtrPtos.get<1>().equal_range(objId)))
    {
        // load nodes
        for (auto &loadRelation : boost::make_iterator_range(this->loadRelations.get<LoadRelation::SrcId>().equal_range(loadPtrPto.srcId)))
        {
            res.insert(loadRelation.loadInstId);
        }

        // memCpy nodes
        for (auto &memCpyRel : boost::make_iterator_range(this->memCpyRelations.get<MemCpyRelation::SrcId>().equal_range(loadPtrPto.srcId)))
        {
            res.insert(memCpyRel.memCpyInstId);
        }

        // copy-out nodes
        for (auto &copyOutFromRel : boost::make_iterator_range(this->copyOutFromRels.get<CopyOutFromRel::FromId>().equal_range(loadPtrPto.srcId)))
        {
            res.insert(copyOutFromRel.copyOutInstId);
        }
    }
    return res;
}

void DDG::dealWithStore(NodeId nodeId)
{
    auto it = this->storeNodeSet.find(nodeId);
    if(it != this->storeNodeSet.end())
    {
        StoreInst *storeInst = it->storeInst;
        if (storesSensinfo(storeInst))
        {
            outs() << "[++] Ws";
        }
        else
        {
            outs() << "[++] Wn";
        }
        outs() << ": ";
        outs() << this->nodeId2String(nodeId) << "\n";
    }
}

void DDG::addAlias(ObjId objId1, ObjId objId2)
{
    // store obj1, load obj2
    // TODO: connect others
    for (auto &storePtrPto : boost::make_iterator_range(this->storePtrPtos.get<1>().equal_range(objId1)))
    {
        for (auto &loadPtrPto : boost::make_iterator_range(this->loadPtrPtos.get<1>().equal_range(objId2)))
        {
            for (auto &storeRelation : boost::make_iterator_range(this->storeRelations.get<StoreRelation::DstId>().equal_range(storePtrPto.dstId)))
            {
                for (auto &loadRelation : boost::make_iterator_range(this->loadRelations.get<LoadRelation::SrcId>().equal_range(loadPtrPto.srcId)))
                {
                    if (overlap(storePtrPto.offset, storePtrPto.offset + storeRelation.storeSize, loadPtrPto.offset, loadPtrPto.offset + loadRelation.loadSize))
                    {
                        boost::add_edge(storeRelation.storeInstId, loadRelation.loadInstId, this->graph);
                    }
                }
            }
        }
    }
}

void DDG::getAffectedNodes(NodeId nodeId, set<NodeId> &loadNodes, set<NodeId> &storeNodes)
{
    // load
    for (auto &loadRelation : boost::make_iterator_range(this->loadRelations.get<LoadRelation::SrcId>().equal_range(nodeId)))
    {
        loadNodes.insert(loadRelation.loadInstId);
    }

    // store
    for (auto &storeRelation : boost::make_iterator_range(this->storeRelations.get<StoreRelation::DstId>().equal_range(nodeId)))
    {
        storeNodes.insert(storeRelation.storeInstId);
    }

    // memcpy
    for (auto &memCpyRelation : boost::make_iterator_range(this->memCpyRelations.get<MemCpyRelation::SrcId>().equal_range(nodeId)))
    {
        loadNodes.insert(memCpyRelation.memCpyInstId);
    }
    for (auto &memCpyRelation : boost::make_iterator_range(this->memCpyRelations.get<MemCpyRelation::DstId>().equal_range(nodeId)))
    {
        storeNodes.insert(memCpyRelation.memCpyInstId);
    }
    for (auto &memCpyRelation : boost::make_iterator_range(this->memCpyRelations.get<MemCpyRelation::NId>().equal_range(nodeId)))
    {
        loadNodes.insert(memCpyRelation.memCpyInstId);
        storeNodes.insert(memCpyRelation.memCpyInstId);
    }

    // copy out
    for (auto &copyOutFromRel : boost::make_iterator_range(this->copyOutFromRels.get<CopyOutFromRel::FromId>().equal_range(nodeId)))
    {
        loadNodes.insert(copyOutFromRel.copyOutInstId);
    }
    for (auto &copyOutNRel : boost::make_iterator_range(this->copyOutNRels.get<CopyOutNRel::NId>().equal_range(nodeId)))
    {
        loadNodes.insert(copyOutNRel.copyOutInstId);
    }
}

uint64_t DDG::getNumNodes()
{
    return boost::num_vertices(this->graph);
}

uint64_t DDG::getNumEdges()
{
    return boost::num_edges(this->graph);
}

string DDG::toString()
{
    string res;
    res += "DDG(\n";

    // def-use
    res += "Def-use(\n";
    for (DefUse defUse : this->defUseSet)
    {
        res += this->nodeId2String(defUse.srcId);
        res += " -> ";
        res += this->nodeId2String(defUse.dstId) + "\n";
    }
    res += ")\n";

    // load
    res += "Load(\n";
    for (LoadRelation loadRelation : this->loadRelations)
    {
        res += "{";
        for (auto &loadPtrPto : boost::make_iterator_range(this->loadPtrPtos.equal_range(loadRelation.srcId)))
        {
            res += "(";
            res += ObjectManager::toString(loadPtrPto.objId) + ", ";
            res += "[" + to_string(loadPtrPto.offset) + ", " + to_string(loadPtrPto.offset + loadRelation.loadSize) + ")";
            res += "), ";
        }
        res += "} -> ";
        res += this->nodeId2String(loadRelation.loadInstId);
        res += " -> ";
        res += this->nodeId2String(loadRelation.valId) + "\n";
    }
    res += ")\n";

    // store
    res += "Store(\n";
    for (StoreRelation storeRelation : this->storeRelations)
    {
        res += this->nodeId2String(storeRelation.valId);
        res += " -> ";
        res += this->nodeId2String(storeRelation.storeInstId);
        res += " -> {";
        for (auto &storePtrPto : boost::make_iterator_range(this->storePtrPtos.equal_range(storeRelation.dstId)))
        {
            res += "(";
            res += ObjectManager::toString(storePtrPto.objId) + ", ";
            res += "[" + to_string(storePtrPto.offset) + ", " + to_string(storePtrPto.offset + storeRelation.storeSize) + ")";
            res += "), ";
        }
        res += "}\n";
    }
    res += ")\n";

    // memCpy
    res += "MemCpy(\n";
    for (MemCpyRelation memCpyRelation : this->memCpyRelations)
    {
        res += "{";
        for (auto &loadPtrPto : boost::make_iterator_range(this->loadPtrPtos.equal_range(memCpyRelation.srcId)))
        {
            res += "(";
            res += ObjectManager::toString(loadPtrPto.objId) + ", ";
            res += "[" + to_string(loadPtrPto.offset) + ", )"; // TODO: end offset
            res += "), ";
        }
        res += "} -> ";
        res += this->nodeId2String(memCpyRelation.memCpyInstId);
        res += " -> {";
        for (auto &storePtrPto : boost::make_iterator_range(this->storePtrPtos.equal_range(memCpyRelation.dstId)))
        {
            res += "(";
            res += ObjectManager::toString(storePtrPto.objId) + ", ";
            res += "[" + to_string(storePtrPto.offset) + ", )"; // TODO: end offset
            res += "), ";
        }
        res += "}\n";
    }
    res += ")\n";

    // copy out
    res += "CopyOut(\n";
    for (CopyOutFromRel copyOutFromRel : this->copyOutFromRels)
    {
        res += "{";
        for (auto &loadPtrPto : boost::make_iterator_range(this->loadPtrPtos.equal_range(copyOutFromRel.fromId)))
        {
            res += "(";
            res += ObjectManager::toString(loadPtrPto.objId) + ", ";
            res += "[" + to_string(loadPtrPto.offset) + ", )"; // TODO: end offset
            res += "), ";
        }
        res += "} -> ";
        res += this->nodeId2String(copyOutFromRel.copyOutInstId);
        res += " -> user space\n";
    }
    res += ")\n";

    res += ")\n";

    res += "Graph(\n";
    for (auto e : boost::make_iterator_range(boost::edges(this->graph)))
    {
        res += this->nodeId2String(boost::source(e, this->graph));
        res += " -> ";
        res += this->nodeId2String(boost::target(e, this->graph));
        res += "\n";
    }
    res += ")\n";

    return res;
}

void DDG::bfs(NodeId startId, vector<NodeId> loadLayers)
{
    if (loadLayers.size() == 3)
    {
        return;
    }

    if (std::find(loadLayers.begin(), loadLayers.end(), startId) == loadLayers.end())
    {
        loadLayers.push_back(startId);
    }
    else
    {
        return;
    }
    outs() << "[*] BFS from NodeId " << startId << ": " << this->nodeId2String(startId) << "\n";

    set<NodeId> reachedCopyOutNodes;
    set<NodeId> unconnectedStores;
    set<NodeId> affectedLoads;
    set<NodeId> affectedStores;
    my_visitor vis(*this, reachedCopyOutNodes, unconnectedStores, affectedLoads, affectedStores);
    boost::breadth_first_search(this->graph, startId, visitor(vis));

    // info leak
    if (!reachedCopyOutNodes.empty())
    {
        outs() << "[+] Reached CopyOutNodes:\n";
        for (NodeId nodeId : reachedCopyOutNodes)
        {
            outs() << "[++] " << string(loadLayers.size(), 'R') << ": ";
            outs() << this->nodeId2String(nodeId) << "\n";
        }
    }
    else
    {
        outs() << "[-] No reached CopyOutNodes\n";
    }

    // unconnected load nodes
    if (!unconnectedStores.empty())
    {
        outs() << "[+] Unconnected store nodes:\n";
        for (NodeId nodeId : unconnectedStores)
        {
            outs() << this->nodeId2String(nodeId) << "\n";
        }
    }

    // new read error
    if (!affectedLoads.empty())
    {
        outs() << "[+] Affected loads:\n";
        for (NodeId nodeId : affectedLoads)
        {
            outs() << this->nodeId2String(nodeId) << "\n";
            this->bfs(nodeId, loadLayers);
        }
    }
    else
    {
        outs() << "[-] No affected loads\n";
    }

    // new write error
    if (!affectedStores.empty())
    {
        outs() << "[+] Affected stores:\n";
        for (NodeId nodeId : affectedStores)
        {
            auto it = this->storeNodeSet.find(nodeId);
            if(it != this->storeNodeSet.end())
            {
                StoreInst *storeInst = it->storeInst;
                outs() << "[++] " << string(loadLayers.size(), 'R');
                if (storesSensinfo(storeInst))
                {
                    outs() << "Ws";
                }
                else
                {
                    outs() << "Wn";
                }
                outs() << ": ";
                outs() << this->nodeId2String(nodeId) << "\n";
            }
            // TODO: memcpy
        }
    }
    else
    {
        outs() << "[-] No affected stores\n";
    }
}

NodeId DDG::genNodeId()
{
    return this->currNodeId++;
}

void my_visitor::discover_vertex(Graph::vertex_descriptor v, const Graph &g)
{
    outs() << "Discovers: " << this->ddg.nodeId2String(v) << "\n";
    switch (this->ddg.getNodeKind(v))
    {
    case NodeKind::copyout:
    case NodeKind::valout:
    {
        this->reachedCopyOutNodes.insert(v);
        break;
    }
    case NodeKind::value:
    {
        this->ddg.getAffectedNodes(v, this->affectedLoads, this->affectedStores);
        break;
    }
    case NodeKind::store:
    case NodeKind::mcpy:
    {
        if (boost::out_degree(v, g) == 0)
        {
            this->unconnectedStores.insert(v);
        }
        break;
    }
    default:
        break;
    }
}

void processLoad(DDG &ddg, Context context, set<ObjLoc> srcPtees, LoadInst *loadInst, Value *src, uint64_t loadSize)
{
    // TODO: value can be other than stack variable
    NodeId loadInstId = ddg.getOrCreateLoadNode(context, loadInst);
    NodeId srcId = ddg.getOrCreateValNode(context, src);
    NodeId valId = ddg.getOrCreateValNode(context, loadInst);
    ddg.addLoadRelation(loadInstId, srcId, valId, srcPtees, loadSize);
}

void processStore(DDG &ddg, Context context, Value *val, set<ObjLoc> dstPtees, StoreInst *storeInst, Value *dst, uint64_t storeSize)
{
    // TODO: value can be other than stack variable
    NodeId valId = ddg.getOrCreateValNode(context, val);
    NodeId dstId = ddg.getOrCreateValNode(context, dst);
    NodeId storeInstId = ddg.getOrCreateStoreNode(context, storeInst);
    ddg.addStoreRelation(storeInstId, valId, dstId, dstPtees, storeSize);
}

void processMemCpy(DDG &ddg, Context context, llvm::Instruction *memCpyInst, Value *src, set<ObjLoc> srcPtees, Value *dst, std::set<ObjLoc> dstPtees, Value *n)
{
    // TODO: value can be other than stack variable
    NodeId memCpyInstId = ddg.getOrCreateMemCpyNode(context, memCpyInst);
    NodeId srcId = ddg.getOrCreateValNode(context, src);
    NodeId dstId = ddg.getOrCreateValNode(context, dst);
    NodeId nId = ddg.getOrCreateValNode(context, n);
    ddg.addMemCpy(memCpyInstId, srcId, dstId, nId, srcPtees, dstPtees);
}

void processGEP(DDG &ddg, Context context, Value *ptr, GetElementPtrInst *gepInst)
{
    // TODO: value can be other than stack variable
    NodeId ptrId = ddg.getOrCreateValNode(context, ptr);
    NodeId gepId = ddg.getOrCreateValNode(context, gepInst);
    ddg.addDefUseEdge(ptrId, gepId);
}

void processCast(DDG &ddg, Context context, Value *src, CastInst *castInst)
{
    // TODO: value can be other than stack variable
    NodeId srcId = ddg.getOrCreateValNode(context, src);
    NodeId castId = ddg.getOrCreateValNode(context, castInst);
    ddg.addDefUseEdge(srcId, castId);
}

void processCopyOut(DDG &ddg, Context context, CallInst *callInst, Value *from, set<ObjLoc> fromPtees, Value *n)
{
    NodeId copyOutInstId = ddg.getOrCreateCopyOutNode(context, callInst);
    NodeId fromId = ddg.getOrCreateValNode(context, from);
    NodeId nId = ddg.getOrCreateValNode(context, n);
    ddg.addCopyOutRelation(copyOutInstId, fromId, nId, fromPtees);
}

void processPrintk(DDG &ddg, Context context, map<Value *, set<ObjLoc>> argPteesMap, CallInst *callInst)
{
    NodeId copyOutInstId = ddg.getOrCreateCopyOutNode(context, callInst);
    for (auto &it : argPteesMap)
    {
        NodeId argId = ddg.getOrCreateValNode(context, it.first);
        ddg.addCopyOutRelation(copyOutInstId, argId, -1, it.second);
    }
}

void processValueOut(DDG &ddg, Context context, llvm::Value *val, llvm::CallInst *callInst)
{
    NodeId valId = ddg.getOrCreateValNode(context, val);
    NodeId valueOutId = ddg.getOrCreateValOutNode(context, callInst);
    ddg.addDefUseEdge(valId, valueOutId);
}

void processCall(DDG &ddg, Context callerCtx, llvm::Value *arg, Context calleeCtx, llvm::Argument *parm)
{
    // TODO: value can be other than stack variable
    NodeId argId = ddg.getOrCreateValNode(callerCtx, arg);
    NodeId parmId = ddg.getOrCreateValNode(calleeCtx, parm);
    ddg.addDefUseEdge(argId, parmId);
}

void processRet(DDG &ddg, Context calleeCtx, Value *retVal, Context callerCtx, Instruction *callInst)
{
    NodeId retValId = ddg.getOrCreateValNode(calleeCtx, retVal);
    NodeId callInstId = ddg.getOrCreateValNode(callerCtx, callInst);
    ddg.addDefUseEdge(retValId, callInstId);
}

void processPHINode(DDG &ddg, Context context, set<Value *> incomingValues, PHINode *phiNode)
{
    // TODO: value can be other than stack variable
    NodeId phiNodeId = ddg.getOrCreateValNode(context, phiNode);
    for (Value *incomingVal : incomingValues)
    {
        NodeId incomingValId = ddg.getOrCreateValNode(context, incomingVal);
        ddg.addDefUseEdge(incomingValId, phiNodeId);
    }
}

void processBiOp(DDG &ddg, Context context, Value *opd0, Value *opd1, BinaryOperator *biOp)
{
    // TODO: value can be other than stack variable
    NodeId opd0Id = ddg.getOrCreateValNode(context, opd0);
    NodeId opd1Id = ddg.getOrCreateValNode(context, opd1);
    NodeId biOpId = ddg.getOrCreateValNode(context, biOp);
    ddg.addDefUseEdge(opd0Id, biOpId);
    ddg.addDefUseEdge(opd1Id, biOpId);
}
