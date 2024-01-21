#pragma once

#include "GlobalState.h"

void getAllocSites(GlobalState &glbState, std::multimap<llvm::StructType *, llvm::CallInst *> &general, std::multimap<llvm::StructType *, llvm::CallInst *> &special);
