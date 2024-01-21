#pragma once

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"

#include <set>
#include <string>
#include <tuple>
#include <vector>

// sometimes Value::getName returns an empty string, so we need to use raw_string_ostream to get its name
std::string getName(llvm::Value *value, bool shortName = true);
bool overlap(int64_t start1, int64_t end1, int64_t start2, int64_t end2);
std::string trimName(std::string name);
std::vector<llvm::Instruction *> getAllInstsAtSrcLine(llvm::Function *F, std::string filename, uint32_t line);

// get the filename and the line number of an instruction
std::tuple<std::string, uint32_t> getInstFileAndLine(llvm::Instruction *I);

std::vector<std::string> tokenize(std::string s, std::string del);
std::vector<unsigned> findAllOccurrences(std::string str, char c);

// check if the store instruction stores sensitive info
bool storesSensinfo(llvm::StoreInst *storeInst);

// get called function from the callInst
llvm::Function *getCalledFunction(llvm::CallInst *CI);
