#include "llvm/Analysis/CFG.h"
#include "llvm/Support/raw_ostream.h"

#include "InstLoc.h"

#include <algorithm>

using namespace llvm;
using namespace std;

Context::Context(Function *entry) : entry(entry)
{
}

bool Context::operator<(const Context &rhs) const
{
    if (this->entry == rhs.entry)
    {
        return this->pairs < rhs.pairs;
    }
    else
    {
        return this->entry < rhs.entry;
    }
}

Function *Context::getCurrentFunction() const
{
    if (this->pairs.size() == 0)
    {
        return entry;
    }
    else
    {
        return this->pairs.back().second;
    }
}

InstLoc Context::getCallerInstLoc() const
{
    assert(this->pairs.size() >= 1);
    CallInst *inst = this->pairs.back().first;

    Context res(*this);
    res.pairs.pop_back();
    return InstLoc(res, inst);
}

uint32_t Context::getDepth() const
{
    return this->pairs.size() + 1;
}

Context Context::add(CallInst *callInst, Function *calledFunc) const
{
    Context res(*this);
    res.pairs.push_back(std::pair<CallInst *, Function *>(callInst, calledFunc));
    return res;
}

string Context::toString() const
{
    string res;
    res += entry->getName().str();
    for (auto &pair : this->pairs)
    {
        // for simplicity, only print called function
        res += ":";
        res += pair.second->getName().str();
    }
    return res;
}

InstLoc::InstLoc(Context context, Instruction *inst) : context(context), inst(inst)
{
}

string InstLoc::toString() const
{
    string res;
    res += this->context.toString();
    res += ":";

    string tmp;
    raw_string_ostream ss(tmp);
    ss << *this->inst;
    res += ss.str();

    return res;
}

bool isReachable(InstLoc fromLoc, InstLoc toLoc)
{
    // Different entries, unreachable
    if (fromLoc.context.entry != toLoc.context.entry)
    {
        return false;
    }

    size_t minPairsSize = min(fromLoc.context.pairs.size(), toLoc.context.pairs.size());
    for (size_t i = 0; i < minPairsSize; ++i)
    {
        if (fromLoc.context.pairs[i].first != toLoc.context.pairs[i].first)
        {
            return isPotentiallyReachable(fromLoc.context.pairs[i].first, toLoc.context.pairs[i].first);
        }
        else
        {
            if (fromLoc.context.pairs[i].second != toLoc.context.pairs[i].second)
            {
                return false; // same caller but different called functions, unreachable
            }
        }
    }

    Instruction *tmpFrom = (fromLoc.context.pairs.size() <= minPairsSize) ? fromLoc.inst : fromLoc.context.pairs[minPairsSize].first;
    Instruction *tmpTo = (toLoc.context.pairs.size() <= minPairsSize) ? toLoc.inst : toLoc.context.pairs[minPairsSize].first;
    return isPotentiallyReachable(tmpFrom, tmpTo);
}
