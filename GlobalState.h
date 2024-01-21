#pragma once

#include "llvm/IR/Module.h"

#include <fstream>
#include <iostream>
#include <set>
#include <vector>

class GlobalState
{
public:
    std::ostream& out; // use this ostream to output info
    std::ofstream outFunctions; // output function info
    std::ofstream outTypes; // output type info
    std::vector<llvm::Module *> moduleList;
    std::map<std::string, llvm::Function *> name2GlbFuncDef;
    std::multimap<std::string, llvm::Function *> name2InternalFuncDef;
    std::map<std::string, llvm::Function *> name2WeakGlbFuncDef;
    std::multimap<std::string, std::pair<llvm::Module *, llvm::StructType *>> name2StructType;

    GlobalState(std::vector<llvm::Module *> MList, std::ostream& out);
    std::vector<llvm::Module *> &getModuleList();
    llvm::Function *getFuncDef(llvm::Function *);
    std::set<llvm::Function *> getFuncDefs(std::string name); // TODO: use caller's filename to filter
    llvm::Function *getSingleFuncDef(std::string name); // TODO: use caller's filename to filter

    // debug
    void printTypes(); // print types
    void searchTypes(const char *objName); // print types

private:
    void genModuleFacts(llvm::Module &M); // called in constructor
};
