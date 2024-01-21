#pragma once

#include <string>
#include <vector>

class CallTraceItem
{
public:
    std::string func;
    std::string file;
    uint32_t line;

    CallTraceItem(std::string func, std::string file, uint32_t line);
};

class InitMemErr
{
public:
    std::vector<CallTraceItem> callTrace;
    bool isRead;
    uint32_t memAccessSize;

    InitMemErr();
};

std::vector<std::string> parseInput(std::string input);
void parseConfigFile(std::string filename, std::vector<std::string> &entryFunctionNames, std::vector<InitMemErr> &initMemErrs, std::vector<std::string> &inputFilenames, uint32_t &maxCallDepth, std::string &callGraph, bool &doPrint);
