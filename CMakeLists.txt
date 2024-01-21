cmake_minimum_required(VERSION 3.13.4)
project(MyProject)

find_package(LLVM REQUIRED CONFIG)
find_package(nlohmann_json 3.11.2 REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-fno-rtti -fPIC")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Now build our tools
set(SOURCES_PAT
    MainPAT.cpp
    CallGraph.cpp Config.cpp DDG.cpp GlobalState.cpp InstLoc.cpp ParseIR.cpp PointsTo.cpp Object.cpp SCC.cpp Utils.cpp Visitor.cpp
    CallGraph.h Config.h DDG.h GlobalState.h InstLoc.h ParseIR.h PointsTo.h Object.h SCC.h Utils.h Visitor.h
)
add_executable(PAT ${SOURCES_PAT})

set(SOURCES_AllocSite
    MainAllocSite.cpp
    AllocSite.cpp Config.cpp GlobalState.cpp ParseIR.cpp Utils.cpp
    AllocSite.h Config.h GlobalState.h ParseIR.h Utils.h
)
add_executable(AllocSite ${SOURCES_AllocSite})

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support core irreader analysis)

# Link against LLVM libraries
target_link_libraries(PAT ${llvm_libs} nlohmann_json::nlohmann_json)
target_link_libraries(AllocSite ${llvm_libs})
