#pragma once

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include "Config.h"
#include "GlobalState.h"
#include "InstLoc.h"

#include <map>
#include <set>
#include <string>
#include <vector>

// Simple call graph that lacks callsite information
typedef std::map<std::string, std::set<std::string>> DSimpleCGMap;
typedef std::pair<std::string, std::string> DSimpleCGEdge;
typedef std::set<DSimpleCGEdge> DSimpleCGSet;

// Context-insensitive call graph
typedef std::pair<llvm::Function *, llvm::CallInst *> Caller;
typedef std::map<Caller, std::set<llvm::Function *>> CGMap;
typedef std::pair<Caller, llvm::Function *> CGEdge;
typedef std::set<CGEdge> CGSet;

DSimpleCGSet readDSimpleCGSet(std::string filename);
DSimpleCGMap DSimpleCGSet2DSimpleCGMap(DSimpleCGSet dSimpleCGSet);
CGMap DSimpleCGMap2CGMap(GlobalState &glbState, DSimpleCGMap dSimpleCGMap);

Context callTrace2Context(GlobalState &glbState, std::vector<CallTraceItem> callTrace, CGMap &cgMap);
