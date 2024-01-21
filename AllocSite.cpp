#include "AllocSite.h"
#include "Utils.h"

#include <map>

using namespace llvm;
using namespace std;

static string isGeneralAllocFunc(string funcName)
{
    static const set<string> generalAllocFuncs{
        "__kmalloc", "__kmalloc_node", "kmalloc", "kmalloc_node", "kmalloc_array", "kzalloc", "kmalloc_array_node", "kzalloc_node", "kcalloc_node", "kcalloc"};

    string trimmedFuncName = trimName(funcName); // to solve renaming problem
    if (generalAllocFuncs.find(trimmedFuncName) != generalAllocFuncs.end())
    {
        return funcName;
    }
    else
    {
        return "";
    }
}

static string isSpecialAllocFunc(string funcName)
{
    static const set<string> dedicatedAllocFuncs{
        // special
        "kmem_cache_alloc", "kmem_cache_alloc_node", "kmem_cache_alloc_trace", "kmem_cache_zalloc",

        // corner
        "sock_kmalloc"};

    string trimmedFuncName = trimName(funcName); // to solve renaming problem
    if (dedicatedAllocFuncs.find(trimmedFuncName) != dedicatedAllocFuncs.end())
    {
        return funcName;
    }
    else
    {
        return "";
    }
}

// may be cast to multiple types (i.e. a struct and its nested structs)
static void retIsCastTo(CallInst &callInst, vector<StructType *> &castDstTypes)
{
    for (Value::use_iterator ui = callInst.use_begin(); ui != callInst.use_end(); ui++)
    {
        if (CastInst *inst = dyn_cast<CastInst>(ui->getUser()))
        {
            if (PointerType *T = dyn_cast<PointerType>(inst->getDestTy()))
            {
                Type *elementType = T->getElementType();
                if (StructType *structType = dyn_cast<StructType>(elementType)) // elementType->getTypeID() == Type::StructTyID
                {
                    // TODO: may be an opaque type
                    castDstTypes.push_back(structType);
                }
            }
        }
    }
}

// Intraprocedural Analysis
void getAllocSites(GlobalState &glbState, multimap<StructType *, CallInst *> &general, multimap<StructType *, CallInst *> &special)
{
    for (Module *M : glbState.getModuleList())
    {
        for (Function &F : *M)
        {
            for (BasicBlock &BB : F)
            {
                for (Instruction &I : BB)
                {
                    if (CallInst *callInst = dyn_cast<CallInst>(&I))
                    {
                        Function *callee = callInst->getCalledFunction();
                        if (callee)
                        {
                            string calleeName = callee->getName().str();
                            string strGeneral = isGeneralAllocFunc(calleeName);
                            string strSpecial = isSpecialAllocFunc(calleeName);
                            bool isGeneral = strGeneral != "";
                            bool isSpecial = strSpecial != "";
                            if (isGeneral || isSpecial)
                            {
                                vector<StructType *> castDstTypes;
                                retIsCastTo(*callInst, castDstTypes);
                                if (castDstTypes.size() >= 1)
                                {
                                    // tip: in most cases, the last type is the outmost struct type
                                    StructType *structType = castDstTypes.back();
                                    if (isGeneral)
                                    {
                                        general.insert(std::make_pair(structType, callInst));
                                    }
                                    else
                                    {
                                        special.insert(std::make_pair(structType, callInst));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
