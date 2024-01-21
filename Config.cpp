#include "Config.h"

#include <fstream>
#include <vector>

#include <nlohmann/json.hpp>

using namespace std;

using json = nlohmann::json;

CallTraceItem::CallTraceItem(string func, string file, uint32_t line) : func(func), file(file), line(line)
{
}

InitMemErr::InitMemErr()
{
}

vector<string> parseInput(string input)
{
    vector<string> inputFiles;
    ifstream s(input);
    string line;
    while (getline(s, line))
    {
        inputFiles.push_back(line);
    }
    return inputFiles;
}

void parseConfigFile(
    string filename,
    vector<string> &entryFunctionNames,
    vector<InitMemErr> &initMemErrs,
    vector<string> &inputFilenames,
    uint32_t &maxCallDepth,
    string &callGraph,
    bool &doPrint)
{
    ifstream configFile(filename);
    assert(configFile.is_open());

    json j = nlohmann::json::parse(configFile, nullptr, true, true); // allow json comments

    // entry functions
    for (json &entry : j["entries"])
    {
        entryFunctionNames.push_back(entry);
    }

    // init memory errors
    for (json &jInitMemErr : j["initMemErrs"])
    {
        InitMemErr initMemErr;

        // call trace
        for (json &item : jInitMemErr["callTrace"])
        {
            initMemErr.callTrace.push_back(CallTraceItem(item["func"], item["file"], item["line"]));
        }

        // bug type
        initMemErr.isRead = jInitMemErr["isRead"];

        // memory access size
        initMemErr.memAccessSize = jInitMemErr["memAccessSize"];

        initMemErrs.push_back(initMemErr);
    }

    // file that contains all input files
    string input;
    if (j.contains("input"))
    {
        input = j["input"];
    }
    else
    {
        input = "input";
    }
    inputFilenames = parseInput(input); // all input files

    // max call depth
    maxCallDepth = j["maxCallDepth"];

    // call graph file
    if (j.contains("callGraph"))
    {
        callGraph = j["callGraph"];
    }
    else
    {
        callGraph = "cg";
    }

    // whether to print the graph and related information
    if (j.contains("doPrint"))
    {
        doPrint = j["doPrint"];
    }
    else
    {
        doPrint = false;
    }
}
