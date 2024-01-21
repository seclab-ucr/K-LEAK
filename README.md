# K-LEAK

This framework consists of static analysis and dynamic analysis.

We use [SyzScope](https://github.com/seclab-ucr/SyzScope) as our dynamic analysis.

The followings are tested on **Ubuntu 18.04**.

# Setup

## Install boost and nlohmann json

```sh
sudo apt install libboost-all-dev
wget https://github.com/nlohmann/json/archive/refs/tags/v3.11.2.tar.gz            
tar -xf v3.11.2.tar.gz 
cd json-3.11.2/
mkdir build
cd build/
cmake ..
make
sudo make install
```

## Build

```sh
rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j15
cd ..
```

# Usage

## Building Initial M-DFG

The initial M-DFG is built by static analysis tool.
It takes as input a list of kernel entry functions to be analyzed and builds an initial DFG of them.

The DFG is stored using C++ data structures:
[Boost Graph Library](https://www.boost.org/doc/libs/1_84_0/libs/graph/doc/index.html) and [Boost Multi-index Containers Library](https://www.boost.org/doc/libs/1_84_0/libs/multi_index/doc/index.html).
The data structures should be understood thoroughly.
A boost graph stores the skeleton of M-DFG;
Multiple multi-index containers store additional important information.
Please see the codebase carefully.

A workspace folder should be created to run the tool.
It stores the input/output of this tool:

`config.json` is the configuration file to run this tool.
It is a JSON file.
Two options are worth mentioning:
`entries` specifies the kernel entries.
`maxCallDepth` specifies the inter-procedural depth.

`input` contains a list of kernel bitcode files to analyze, with each line representing a filepath.
It is recommended to compile the kernel into a list of bitcode files instead of a single file, which is timesaving during the analysis.

`out` contains the output of the tool (e.g., the infoleak paths; new memory errors).
You can also print the whole M-DFG.

`out_parse` produces additional diagnosis information.

## Iterative Algorithm

After the initial DFG is built,
we can perform the iterative algorithm.
The algorithm is implemented using both static and dynamic analysis steps.

First, we needs to use [SyzScope](https://github.com/seclab-ucr/SyzScope) to reproduce a bug and obtain the capability of the initial memory error.

Second, we augment the DFG with the initial memory error by adding an unintended RAW edge (You should understand the codebase, especially `DDG.h` and `DDG.cpp`, which provides APIs for doing so).

Third, we start searching the BFS search in M-DFG.
The search will output the new memory error and infoleaks (See `MainPAT.cpp`).
For each new memory error, we should (recursively/iteratively) use the symbolic execution of SyzScope to verify the path feasibility and determine the real capability of the memory error.
For each infoleak path, we also run SyzScope to verify it.
