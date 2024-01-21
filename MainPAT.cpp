#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"

#include "CallGraph.h"
#include "Config.h"
#include "DDG.h"
#include "GlobalState.h"
#include "ParseIR.h"
#include "Utils.h"
#include "Visitor.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>

using namespace llvm;
using namespace std;

cl::opt<string> config("c", cl::desc("Specify config file"), cl::value_desc("filename"), cl::init("config.json"));

uint32_t maxCallDepth;

vector<Function *> getEntryFuncs(GlobalState &glbState, vector<string> entryFuncNames)
{
    vector<Function *> entryFuncs;
    for (string name : entryFuncNames)
    {
        Function *def = glbState.getSingleFuncDef(name);
        entryFuncs.push_back(def);
    }
    return entryFuncs;
}

int main(int argc, char **argv)
{
    cl::ParseCommandLineOptions(argc, argv);

    // parse config file
    vector<string> entryFuncNames;
    vector<InitMemErr> initMemErrs;
    vector<string> inputFilenames;
    string callGraph;
    bool doPrint;
    parseConfigFile(config, entryFuncNames, initMemErrs, inputFilenames, maxCallDepth, callGraph, doPrint);

    // parse all input IR files
    ofstream parseOut("out_parse"); // output all parsing related info to this file
    assert(parseOut.is_open());
    vector<Module *> moduleList = parseIRFilesMultithread(inputFilenames, parseOut);
    GlobalState glbState(moduleList, parseOut);
    parseOut.close();

    // read call graph
    DSimpleCGSet dSimpleCGSet = readDSimpleCGSet(callGraph);
    DSimpleCGMap dSimpleCGMap = DSimpleCGSet2DSimpleCGMap(dSimpleCGSet);
    CGMap cgMap = DSimpleCGMap2CGMap(glbState, dSimpleCGMap);

    // entries
    vector<Function *> entryFuncs;
    vector<Context> initMemErrCtxs; // contexts of all init memory errors. Assume they have the same entry (TODO: fix the assumption)
    for (InitMemErr initMemErr : initMemErrs)
    {
        // context of init memory error. Also update indirect call graph
        Context initMemErrCtx = callTrace2Context(glbState, initMemErr.callTrace, cgMap);
        initMemErrCtxs.push_back(initMemErrCtx);

        // add init memory error entry
        if (std::find(entryFuncs.begin(), entryFuncs.end(), initMemErrCtx.entry) == entryFuncs.end())
        {
            entryFuncs.push_back(initMemErrCtx.entry);
        }

        maxCallDepth = std::max(maxCallDepth, initMemErrCtx.getDepth()); // should at least allow us to reach the bug site. TODO: use different call depths
    }
    vector<Function *> otherEntries = getEntryFuncs(glbState, entryFuncNames); // other entries
    entryFuncs.insert(entryFuncs.end(), otherEntries.begin(), otherEntries.end());

    // analyze each entry function
    PointsToRecords ptoRes; // pto records of all entry functions
    DDG ddgRes; // DDG of all entry functions
    vector<long long> timeRes; // time taken for all entry functions
#ifdef NUM_VISITED_BBS
    uint64_t numVisitedBB = 0;
#endif
    for (uint32_t i = 0; i < entryFuncs.size(); ++i)
    {
        Context context(entryFuncs[i]);
        FunctionVisitor vis(glbState, context, cgMap, ptoRes, ddgRes);

        chrono::steady_clock::time_point begin = chrono::steady_clock::now();
        vis.analyze();
        chrono::steady_clock::time_point end = chrono::steady_clock::now();
        timeRes.push_back(std::chrono::duration_cast<chrono::milliseconds>(end - begin).count());
#ifdef NUM_VISITED_BBS
        numVisitedBB += vis.getNumVisitedBBs();
#endif
    }
    for (uint32_t i = 0; i < entryFuncs.size(); ++i)
    {
        outs() << "It takes " << timeRes[i] << "ms to finish analyzing the entry function\n";
    }
#ifdef NUM_VISITED_BBS
    outs() << "Number of basic blocks visited: " << numVisitedBB << '\n';
#endif
    outs() << "DDG size: " << ddgRes.getNumNodes() << " nodes, " << ddgRes.getNumEdges() << " edges\n";

    // TODO: whatever way to provide alias info as input and consume it, before building the graph

    // print results
    if (doPrint)
    {
        outs() << ObjectManager::toString() << "\n";
        outs() << ptoRes.toString() << "\n";
        outs() << ddgRes.toString() << "\n";
    }

    // multi-syscall alias analysis (comparing access path)
    // TODO: provide initial aliasing objects among different entries, e.g. socket
    // TODO: aliasing objects under a single objects
    // TODO: alias set should contain offset, not just ObjId
    // TODO: many initial alias sets; use dominator tree to get all alias sets?
    set<ObjId> initialAliasSet; // {1, 395} for ax25 case
    vector<set<ObjId>> resAliasSets;
    vector<set<ObjId>> worklist;
    worklist.push_back(initialAliasSet);
    while (!worklist.empty())
    {
        set<ObjId> currWork = worklist.back();
        worklist.pop_back();
        resAliasSets.push_back(currWork);

        // get all records
        set<ObjPto> records;
        for (auto &objId : currWork)
        {
            set<ObjPto> eachRecords = ptoRes.getRecordsOfObj(objId);
            records.insert(eachRecords.begin(), eachRecords.end());
        }

        // get all offsets
        set<offset_t> offsets;
        for (auto &record : records)
        {
            offsets.insert(record.srcOffset);
        }

        for (offset_t offset : offsets)
        {
            // newly found alias set
            set<ObjId> foundAliasSet;
            for (auto &objPto : records)
            {
                if (objPto.srcOffset == offset)
                {
                    foundAliasSet.insert(objPto.dstObjId);
                }
            }
            worklist.push_back(foundAliasSet);
        }
    }
    for (auto &aliasSet : resAliasSets)
    {
        outs() << "Alias set: ";
        for (auto &objId : aliasSet)
        {
            outs() << objId << ", ";
        }
        outs() << "\n";

        if (aliasSet.size() >= 2)
        {
            ObjId objId1 = *aliasSet.begin();
            ObjId objId2 = *(++aliasSet.begin());
            outs() << objId1 << " " << objId2 << "\n";
            ddgRes.addAlias(objId1, objId2);
        }
    }

    // search DDG
    outs() << "[*] Searching DDG...\n";
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();
    for (uint32_t i = 0; i < initMemErrs.size(); ++i)
    {
        InitMemErr initMemErr = initMemErrs[i];
        Context bugCtx = initMemErrCtxs[i];

        assert(!initMemErr.callTrace.empty());
        CallTraceItem bugItem = initMemErr.callTrace.back();
        Function *initMemErrFunc = bugCtx.getCurrentFunction();
        vector<Instruction *> insts = getAllInstsAtSrcLine(initMemErrFunc, bugItem.file, bugItem.line);
        if (insts.empty())
        {
            outs() << "[-] Cannot find instructions for " << i << "th initial memory errors\n";
        }
        else
        {
            if (initMemErr.isRead) // initial read error
            {
                // get all load like instructions at the src line
                vector<LoadInst *> loadInsts;
                vector<MemCpyInst *> memCpyInsts;
                for (Instruction *I : insts)
                {
                    if (LoadInst *loadInst = dyn_cast<LoadInst>(I))
                    {
                        loadInsts.push_back(loadInst);
                    }
                    else if (MemCpyInst *memCpyInst = dyn_cast<MemCpyInst>(I))
                    {
                        memCpyInsts.push_back(memCpyInst);
                    }
                }

                // filter out irrelevant instructions
                const DataLayout *dataLayout = &bugCtx.entry->getParent()->getDataLayout();
                if (initMemErr.memAccessSize == 1 || initMemErr.memAccessSize == 2 || initMemErr.memAccessSize == 4 || initMemErr.memAccessSize == 8)
                {
                    vector<LoadInst *> copiedLoadInsts = loadInsts;
                    loadInsts.clear();
                    for (LoadInst *loadInst : copiedLoadInsts)
                    {
                        if (dataLayout->getTypeStoreSize(loadInst->getType()) == initMemErr.memAccessSize)
                        {
                            loadInsts.push_back(loadInst);
                        }
                    }
                }

                auto it = std::find(entryFuncs.begin(), entryFuncs.end(), bugCtx.entry);
                assert(it != entryFuncs.end());
                if (loadInsts.empty() && memCpyInsts.empty())
                {
                    outs() << "[-] No load or memcpy instructions for " << i << "th initial memory errors\n";
                }
                else
                {
                    for (LoadInst *loadInst : loadInsts)
                    {
                        outs() << "Load Instruction: " << *loadInst << "\n";

                        // get accessed obj(s)
                        set<ObjLoc> ptees = getPteesOfValPtr(dataLayout, ptoRes, bugCtx, loadInst->getPointerOperand(), false);
                        set<ObjId> objIds;
                        for (auto &ptee : ptees)
                        {
                            objIds.insert(ptee.objId);
                        }

                        // get memory accesses on the same obj
                        set<NodeId> loadNodes;
                        for (ObjId objId : objIds)
                        {
                            set<NodeId> tmp = ddgRes.getLoadLikeNodesOnObj(objId);
                            loadNodes.insert(tmp.begin(), tmp.end());
                        }

                        NodeId id = ddgRes.getOrCreateLoadNode(bugCtx, loadInst);
                        assert(loadNodes.find(id) != loadNodes.end());

                        outs() << std::string(20, '>') << " Initial read error " << std::string(20, '>') << "\n";
                        ddgRes.bfs(id, vector<NodeId>());
                    }

                    for (MemCpyInst *memCPyInst : memCpyInsts)
                    {
                        outs() << "MemCpy Instruction: " << *memCPyInst << "\n";

                        // get accessed obj(s)
                        set<ObjLoc> ptees = getPteesOfValPtr(dataLayout, ptoRes, bugCtx, memCPyInst->getSource(), false);
                        set<ObjId> objIds;
                        for (auto &ptee : ptees)
                        {
                            objIds.insert(ptee.objId);
                        }

                        // get memory accesses on the same obj
                        set<NodeId> loadNodes;
                        for (ObjId objId : objIds)
                        {
                            set<NodeId> tmp = ddgRes.getLoadLikeNodesOnObj(objId);
                            loadNodes.insert(tmp.begin(), tmp.end());
                        }

                        NodeId id = ddgRes.getOrCreateMemCpyNode(bugCtx, memCPyInst);
                        assert(loadNodes.find(id) != loadNodes.end());

                        outs() << std::string(20, '>') << " Initial read error " << std::string(20, '>') << "\n";
                        ddgRes.bfs(id, vector<NodeId>());
                    }
                }
            }
            else // initial write error
            {
                // TODO
            }
        }
    }
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    outs() << "It takes " << std::chrono::duration_cast<chrono::milliseconds>(end - begin).count() << "ms to finish the iterative algorithm\n";
}
