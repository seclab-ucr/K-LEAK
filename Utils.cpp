#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "Utils.h"

using namespace llvm;
using namespace std;

string getName(Value *value, bool shortName)
{
    string tmp;
    raw_string_ostream ss(tmp);
    ss << *value;
    string longName = ss.str();

    if (!shortName)
    {
        return longName;
    }

    // Some Instructions contain '='
    size_t idx = longName.find(" = ");
    if (idx != string::npos)
    {
        string shortName = longName.substr(0, idx);
        return shortName;
    }
    else
    {
        return longName;
    }
}

bool overlap(int64_t start1, int64_t end1, int64_t start2, int64_t end2)
{
    if (end1 == -1)
    {
        end1 = start1 + 10000; // TODO
    }
    if (end2 == -1)
    {
        end2 = start2 + 10000; // TODO
    }
    return start1 < end2 && start2 < end1;
}

string trimName(string name)
{
    size_t nd = name.rfind("."), t = 0;
    if (nd != std::string::npos)
    {
        std::string suffix = name.substr(nd + 1);
        try
        {
            std::stoi(suffix, &t);
        }
        catch (...)
        {
            t = 0;
        }
        if (t >= suffix.size())
        {
            name.erase(nd);
        }
    }
    return name;
}

vector<Instruction *> getAllInstsAtSrcLine(llvm::Function *F, string filename, uint32_t line)
{
    vector<Instruction *> insts;
    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
        llvm::DebugLoc dbgloc = (*I).getDebugLoc();

        // The instruction has DebugLoc
        if(dbgloc)
        {
            string fn = dbgloc->getFilename().str();
            uint32_t ln = dbgloc->getLine();

            // match file name and line
            if(fn == filename && ln == line)
            {
                insts.push_back(&(*I));
            }
        }
    }
    return insts;
}

tuple<string, uint32_t> getInstFileAndLine(llvm::Instruction *I)
{
    llvm::DebugLoc dbgloc = I->getDebugLoc();
    if (dbgloc)
    {
        string fn = dbgloc->getFilename().str();
        uint32_t ln = dbgloc->getLine();
        return make_tuple(fn, ln);
    }
    return make_tuple(string(), 0);
}

vector<string> tokenize(string s, string del)
{
    vector<string> splits;

    int start = 0;
    int end = s.find(del);
    while (end != -1)
    {
        splits.push_back(s.substr(start, end - start));
        start = end + del.size();
        end = s.find(del, start);
    }
    splits.push_back(s.substr(start, end - start));

    return splits;
}

vector<unsigned> findAllOccurrences(string str, char c)
{
    vector<unsigned> occurrences;
    for (unsigned i = 0; i < str.size(); i++)
    {
        if (str[i] == c)
        {
            occurrences.push_back(i);
        }
    }
    return occurrences;
}

bool storesSensinfo(llvm::StoreInst *storeInst)
{
    Value *value = storeInst->getValueOperand();
    if (isa<ConstantPointerNull>(value)) // nullptr is not sensitive info
    {
        return false;
    }
    return value->getType()->isPointerTy();
}

Function *getCalledFunction(CallInst *CI)
{
    Function *calledFunc = CI->getCalledFunction();
    if(!calledFunc)
    {
        Value *calledVal = CI->getCalledOperand();
        calledFunc = dyn_cast<Function>(calledVal->stripPointerCasts());
    }
    return calledFunc;
}
