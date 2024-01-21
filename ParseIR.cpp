#include "llvm/IRReader/IRReader.h"

#include "ParseIR.h"

#include <ostream>
#include <thread>

using namespace llvm;
using namespace std;

vector<Module *> parseIRFiles(vector<string> inputFilenames, ostream &out)
{
    vector<Module *> moduleList;
    const unsigned numInputFiles = inputFilenames.size();
    out << "[*] There are " << numInputFiles << " files to parse\n";
    SMDiagnostic Err;
    for (unsigned i = 0; i < numInputFiles; ++i)
    {
        out << "[*] Parsing (" << i + 1 << " / " << numInputFiles << ") " << inputFilenames[i] << "...\n";
        LLVMContext *LLVMCtx = new LLVMContext();
        unique_ptr<Module> MPtr = parseIRFile(inputFilenames[i], Err, *LLVMCtx);
        if (!MPtr)
        {
            out << "[-] Error in parsing " << inputFilenames[i] << '\n';
            continue;
        }
        Module *M = MPtr.release();
        out << "[+] Parsed\n";
        moduleList.push_back(M);
    }
    return moduleList;
}

void parseWork(vector<string> &inputFilenames, vector<Module *> &moduleList, uint32_t start, uint32_t end, ostream &out)
{
    SMDiagnostic Err;
    for (uint32_t i = start; i < end; ++i)
    {
        LLVMContext *LLVMCtx = new LLVMContext();
        unique_ptr<Module> MPtr = parseIRFile(inputFilenames[i], Err, *LLVMCtx);
        if (!MPtr)
        {
            out << "[-] Error in parsing " << inputFilenames[i] << '\n';
            continue;
        }
        Module *M = MPtr.release();
        out << "[+] Parsed (" << i + 1 << ") " << inputFilenames[i] << "\n";
        moduleList[i] = M;
    }
}

vector<Module *> parseIRFilesMultithread(vector<string> inputFilenames, ostream &out)
{
    const unsigned numInputFiles = inputFilenames.size();
    vector<Module *> moduleList(numInputFiles);
    out << "[*] There are " << numInputFiles << " files to parse\n";

    uint32_t numThreads = 10; // TODO: adjust
    uint32_t workPerThread = 1 + ((numInputFiles - 1) / numThreads);
    vector<thread> threads(numThreads);
    for (uint32_t t = 0; t < numThreads; ++t)
    {
        uint32_t start = workPerThread * t;
        uint32_t end = std::min(numInputFiles, start + workPerThread);
        threads[t] = thread(parseWork, std::ref(inputFilenames), std::ref(moduleList), start, end, std::ref(out));
    }
    for (uint32_t t = 0; t < numThreads; ++t)
    {
        threads[t].join();
    }
    return moduleList;
}
