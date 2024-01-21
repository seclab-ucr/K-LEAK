#include "llvm/IR/InstIterator.h"

#include "CallGraph.h"
#include "Utils.h"

#include <fstream>

using namespace llvm;
using namespace std;

DSimpleCGSet readDSimpleCGSet(string filename)
{
    ifstream s(filename);
    DSimpleCGSet res;
    string line;
    while (getline(s, line))
    {
        vector<string> splits = tokenize(line, "->");
        res.insert(std::make_pair(splits[0], splits[1]));
    }
    return res;
}

DSimpleCGMap DSimpleCGSet2DSimpleCGMap(DSimpleCGSet dSimpleCGSet)
{
    DSimpleCGMap res;
    for (auto edge : dSimpleCGSet)
    {
        string src = edge.first;
        string dst = edge.second;

        // update adj
        if (res.find(src) == res.end())
        {
            res[src] = set<string>();
        }
        res[src].insert(dst);
    }
    return res;
}

CGMap DSimpleCGMap2CGMap(GlobalState &glbState, DSimpleCGMap dSimpleCGMap)
{
    CGMap res;
    for (auto &pair : dSimpleCGMap)
    {
        // find caller function
        string callerName = pair.first;
        Function *callerFunc = glbState.getSingleFuncDef(callerName);
        if (!callerFunc)
        {
            continue;
        }

        for (string calleeName : pair.second)
        {
            // find callee function
            Function *calleeFunc = glbState.getSingleFuncDef(calleeName);
            if(!calleeFunc)
            {
                continue;
            }

            // locate callInst within the caller function
            for (inst_iterator I = inst_begin(callerFunc), E = inst_end(callerFunc); I != E; ++I)
            {
                if (CallInst *CI = dyn_cast<CallInst>(&(*I)))
                {
                    Function *calledFunc = getCalledFunction(CI);
                    if (!calledFunc)
                    {
                        if (CI->arg_size() == calleeFunc->arg_size()) // TODO: further check function signature
                        {
                            res[std::pair<Function *, CallInst *>(callerFunc, CI)].insert(calleeFunc);
                        }
                    }
                }
            }
        }
    }
    return res;
}

Context callTrace2Context(GlobalState &glbState, vector<CallTraceItem> callTrace, CGMap &cgMap)
{
    assert(!callTrace.empty());
    Function *entry = glbState.getSingleFuncDef(callTrace.front().func);
    Context res(entry);

    uint32_t i = 0;
    Function *currFunc = entry;
    for (uint32_t i = 0; i < callTrace.size() - 1; ++i)
    {
        std::vector<Instruction *> insts = getAllInstsAtSrcLine(currFunc, callTrace[i].file, callTrace[i].line);
        for (Instruction *inst : insts)
        {
            if (CallInst *CI = dyn_cast<CallInst>(inst))
            {
                Function *calledFunc = getCalledFunction(CI);
                if (calledFunc) // direct call
                {
                    if (calledFunc->isIntrinsic())
                    {
                    }
                    else
                    {
                        calledFunc = glbState.getFuncDef(calledFunc);
                        assert(calledFunc);
                        if (calledFunc->getName().str() == callTrace[i + 1].func)
                        {
                            res = res.add(CI, calledFunc);
                            currFunc = calledFunc;
                            break;
                        }
                    }
                }
                else // indirect call
                {
                    Function *expectedFunc = glbState.getSingleFuncDef(callTrace[i + 1].func);
                    if (CI->arg_size() == expectedFunc->arg_size()) // TODO: further check function signature
                    {
                        res = res.add(CI, expectedFunc);
                        cgMap[std::make_pair(currFunc, CI)].insert(expectedFunc); // update indirect call graph
                        currFunc = expectedFunc;
                        break; // pick the first match
                    }
                }
            }
        }
    }

    return res;
}
