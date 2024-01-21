#include "llvm/IR/TypeFinder.h"

#include "GlobalState.h"

using namespace llvm;
using namespace std;

GlobalState::GlobalState(vector<Module *> MList, ostream &out) : out(out), outFunctions{"out_functions"}, outTypes{"out_types"}
{
    for (Module *M : MList)
    {
        if (M)
        {
            this->moduleList.push_back(M);
            this->genModuleFacts(*M); // generate facts for each module
        }
    }

    // resolve weak function definitioin
    for (auto &it : this->name2WeakGlbFuncDef)
    {
        if (this->name2GlbFuncDef.find(it.first) == this->name2GlbFuncDef.end())
        {
            this->name2GlbFuncDef[it.first] = it.second;
        }
    }
    this->name2WeakGlbFuncDef.clear();
}

std::vector<llvm::Module *> &GlobalState::getModuleList()
{
    return this->moduleList;
}

void GlobalState::genModuleFacts(Module &M)
{
    // functions
    for (Function &F : M)
    {
        if (!F.isDeclaration())
        {
            string name = F.getName().str();
            auto linkage = F.getLinkage();
            if (F.hasExternalLinkage())
            {
                auto it = this->name2GlbFuncDef.find(name);
                if (it != this->name2GlbFuncDef.end())
                {
                    this->outFunctions << "[-] Function " << name << " already defined in " << it->second->getParent()->getModuleIdentifier() << ".\n";
                    this->outFunctions << "[-] Now comes a second definition in " << F.getParent()->getModuleIdentifier() << ".\n";
                }
                this->name2GlbFuncDef[name] = &F;
            }
            else if (F.hasInternalLinkage())
            {
                this->name2InternalFuncDef.insert(std::make_pair(name, &F));
            }
            else if (F.hasWeakLinkage())
            {
                auto it = this->name2WeakGlbFuncDef.find(name);
                if (it != this->name2WeakGlbFuncDef.end())
                {
                    this->outFunctions << "[-] Weak Function " << name << " already defined in " << it->second->getParent()->getModuleIdentifier() << ".\n";
                    this->outFunctions << "[-] Now comes a second definition in " << F.getParent()->getModuleIdentifier() << ".\n";
                }
                this->name2WeakGlbFuncDef[name] = &F;
            }
        }
    }

    // types
    TypeFinder typeFinder;
    typeFinder.run(M, false);
    for (TypeFinder::iterator itr = typeFinder.begin(), ite = typeFinder.end(); itr != ite; ++itr)
    {
        StructType *st = *itr;
        if (st->isLiteral())
        {
            // TODO: don't know what is literal
            continue;
        }
        if (!st->isOpaque())
        {
            string structName = st->getStructName().str();
            this->name2StructType.insert(std::make_pair(structName, std::make_pair(&M, st)));
        }
    }
}

Function *GlobalState::getFuncDef(Function *F)
{
    if (F->isDeclaration())
    {
        string name = F->getName().str();
        auto it = this->name2GlbFuncDef.find(name);
        if (it != this->name2GlbFuncDef.end())
        {
            return it->second;
        }
        else
        {
            // Cannot find definition
            return nullptr;
        }
    }
    else // already definition
    {
        return F;
    }
}

set<Function *> GlobalState::getFuncDefs(string name)
{
    set<Function *> defs;

    auto itGlb = this->name2GlbFuncDef.find(name);
    if (itGlb != this->name2GlbFuncDef.end())
    {
        defs.insert(itGlb->second);
    }

    auto itInternal = this->name2InternalFuncDef.equal_range(name);
    for (auto it = itInternal.first; it != itInternal.second; ++it)
    {
        defs.insert(it->second);
    }

    return defs;
}

Function *GlobalState::getSingleFuncDef(string name)
{
    set<Function *> defs = this->getFuncDefs(name);
    if (defs.size() == 1)
    {
        return *defs.begin();
    }
    else
    {
        throw exception();
    }
}

void GlobalState::printTypes()
{
    for (auto it = this->name2StructType.begin(); it != this->name2StructType.end(); ++it)
    {
        this->outTypes << "[type] " << it->first << " [id] " << (uint64_t)it->second.second << " [module] " << it->second.first->getModuleIdentifier() << endl;
    }
}

void GlobalState::searchTypes(const char *objName)
{
    auto range = this->name2StructType.equal_range(objName);
    int cnt = 0;
    for (auto it = range.first; it != range.second; ++it)
    {
        this->outTypes << "[type] " << it->first << " [id] " << (uint64_t)it->second.second << " [module] " << it->second.first->getModuleIdentifier() << endl;
        cnt++;
    }
    cout << objName << ": number is " << cnt << endl;
}
