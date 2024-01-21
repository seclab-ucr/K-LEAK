#pragma once

#include "llvm/IR/Instructions.h"

#include <vector>

class InstLoc;

class Context
{
public:
    llvm::Function *entry;
    std::vector<std::pair<llvm::CallInst *, llvm::Function *>> pairs;

    Context(llvm::Function *entry); // construct Context from entry
    bool operator<(const Context &rhs) const; // for std::map
    llvm::Function *getCurrentFunction() const;
    InstLoc getCallerInstLoc() const;
    uint32_t getDepth() const;
    Context add(llvm::CallInst *callInst, llvm::Function *calledFunc) const;
    std::string toString() const;
};

class InstLoc
{
public:
    Context context;
    llvm::Instruction *inst;

    InstLoc(Context context, llvm::Instruction *inst);
    std::string toString() const;
};

bool isReachable(InstLoc fromLoc, InstLoc toLoc);
