#pragma once

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/SourceMgr.h"

#include <string>
#include <vector>

std::vector<llvm::Module *> parseIRFiles(std::vector<std::string> inputFilenames, std::ostream &out);
std::vector<llvm::Module *> parseIRFilesMultithread(std::vector<std::string> inputFilenames, std::ostream &out);
