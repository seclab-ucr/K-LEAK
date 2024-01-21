#pragma once

#include "llvm/IR/BasicBlock.h"

#include <vector>

std::vector<std::vector<llvm::BasicBlock *>> getSCCTraversalOrder(llvm::Function &currF);
