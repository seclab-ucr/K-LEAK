// The code is copied from https://github.com/seclab-ucr/SUTURE

#include "llvm/ADT/SCCIterator.h"
#include "llvm/IR/CFG.h"

#include "SCC.h"

using namespace llvm;

// NOTE: this "TopoSorter" class is adpated from online:
// https://github.com/eliben/llvm-clang-samples/blob/master/src_llvm/bb_toposort_sccs.cpp
// Runs a topological sort on the basic blocks of the given function. Uses
// the simple recursive DFS from "Introduction to algorithms", with 3-coloring
// of vertices. The coloring enables detecting cycles in the graph with a simple
// test.
class TopoSorter {
public:
    TopoSorter() {
        //
    }
    // Given a set of BBs (e.g., BBs belonging to a SCC), we try to sort them in the topological order.
    // The most important thing is we will try to identify back edges if any and act as if they don't exist, which is useful
    // if we want to get a topological sort of BBs in a loop as if the loop is only executed once (e.g., most topo sort
    // yield wrong results in the presence of back edges).
    // The passed-in BBs will be sorted in-place.
    void runToposort(std::vector<BasicBlock*> *BBs) {
        if (!BBs || BBs->size() <= 1) {
            return;
        }
        this->BBs = BBs;
        // Initialize the color map by marking all the vertices white.
        for (BasicBlock *bb : *BBs) {
            ColorMap[bb] = TopoSorter::WHITE;
        }
        // We will always start DFS from the first BB in the vector (e.g., assuming it's the loop head).
        bool success = recursiveDFSToposort((*BBs)[0]);
        if (success) {
            // Now we have all the BBs inside SortedBBs in reverse topological order.
            // Put the reversed sorted results in BBs.
            BBs->clear();
            for (unsigned i = 0; i < SortedBBs.size(); ++i) {
                BBs->insert(BBs->begin(),SortedBBs[i]);
            }
        }
    }

private:
    // "BBs": The BB set to be sorted.
    // "SortedBBs": Collects vertices (BBs) in "finish" order. The first finished vertex is first, and so on.
    std::vector<BasicBlock*> *BBs, SortedBBs;
    enum Color { WHITE, GREY, BLACK };
    // Color marks per vertex (BB).
    DenseMap<BasicBlock *, Color> ColorMap;

    // Helper function to recursively run topological sort from a given BB.
    // This will ignore the back edge when encountered.
    bool recursiveDFSToposort(BasicBlock *BB) {
        if (!BB || std::find(this->BBs->begin(),this->BBs->end(),BB) == this->BBs->end()) {
            return true;
        }
        ColorMap[BB] = TopoSorter::GREY;
        for (llvm::succ_iterator sit = llvm::succ_begin(BB), set = llvm::succ_end(BB); sit != set; ++sit) {
            BasicBlock *Succ = *sit;
            Color SuccColor = ColorMap[Succ];
            if (SuccColor == TopoSorter::WHITE) {
                if (!recursiveDFSToposort(Succ))
                    return false;
            } else if (SuccColor == TopoSorter::GREY) {
                // This detects a cycle because grey vertices are all ancestors of the
                // currently explored vertex (in other words, they're "on the stack").
                // But we just ignore it and act as if current "Succ" has been explored.
            }
        }
        // This BB is finished (fully explored), so we can add it to the vector.
        ColorMap[BB] = TopoSorter::BLACK;
        SortedBBs.push_back(BB);
        return true;
    }
};

// This code is copied from an online source.
std::vector<std::vector<BasicBlock*>> getSCCTraversalOrder(Function &currF) {
    std::vector<std::vector<BasicBlock *>> bbTraversalList;
    const Function *F = &currF;
    //NOTE: both the SCCs and the BBs within each SCC are sorted in the reverse topological order with scc_iterator,
    //so we need to re-revert the order.  
    for (scc_iterator<const Function *> I = scc_begin(F), IE = scc_end(F); I != IE; ++I) {
        // Obtain the vector of BBs in this SCC and print it out.
        const std::vector<const BasicBlock *> &SCCBBs = *I;
        std::vector<BasicBlock *> newCurrSCC;
        for (unsigned int i = 0; i < SCCBBs.size(); ++i) {
            for (Function::iterator b = currF.begin(), be = currF.end(); b != be; ++b) {
                BasicBlock *BB = &(*b);
                if (BB == SCCBBs[i]) {
                    newCurrSCC.insert(newCurrSCC.begin(), BB);
                    break;
                }
            }
        }
        //HZ: NOTE: "succ_iterator" maintains the reverse topological order between SCCs, but inside a single SCC that
        //has multiple BBs, the output order is "reverse DFS visit order" instead of "reverse topological order",
        //e.g., assume the SCC: A->B, A->C, B->C, C->A, the DFS visit order might be A->C->B (if A's child C is visited before B),
        //however, the topo order should be A->B->C as B has an edge to C.
        //To solve this problem, we need to run an extra sorting for SCCs which have more than 1 nodes, forcing it to be in the topo order.
        if (newCurrSCC.size() > 1) {
            TopoSorter tps;
            tps.runToposort(&newCurrSCC);
        }
        bbTraversalList.insert(bbTraversalList.begin(), newCurrSCC);
    }
    return bbTraversalList;
}
