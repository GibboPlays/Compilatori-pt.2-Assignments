//=============================================================================
// FILE:
//    LoopFusion.cpp
//
// DESCRIPTION:
//    Visits all functions in a module and prints their names. Strictly speaking, 
//    this is an analysis pass (i.e. //    the functions are not modified). However, 
//    in order to keep things simple there's no 'print' method here (every analysis 
//    pass should implement it).
//
// USAGE:
//    New PM
//      opt -load-pass-plugin=<path-to>libTestPass.so -passes="test-pass" `\`
//        -disable-output <input-llvm-file>

//
//
// License: MIT
//=============================================================================
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/CFG.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include <set>

using namespace llvm;

//-----------------------------------------------------------------------------
// TestPass implementation
//-----------------------------------------------------------------------------
// No need to expose the internals of the pass to the outside world - keep
// everything in an anonymous namespace.
namespace {


// New PM implementation
struct TestPass: PassInfoMixin<TestPass> {



  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
//<<<<<<< HEAD

    LoopInfo &LI = AM.getResult<LoopAnalysis>(F);

    SmallVector<Loop *, 8> Worklist;
    for (Loop *TopLevelLoop : LI)
    {
      for (Loop *L : depth_first(TopLevelLoop))
      {
        // We only handle inner-most loops
        if (L->isInnermost())
          Worklist.push_back(L);
      }
    }
    
    //Checking conditions

    //Adjacent
    SmallVector<std::pair<Loop*,Loop*>> adj;

    for (int i=0; i<7; i++)
    {

      //Ultimo basic block del primo loop
      Loop *l1 = Worklist[i];
      BasicBlock *b1 = l1->getExitBlock();
      //Primo basic block del secondo loop
      Loop *l2 = Worklist[i+1];
      BasicBlock *b2 = l2->getHeader();

      if (b1->getSingleSuccessor() == b2->getSinglePredecessor())
      {
        adj.push_back(std::make_pair(l1,l2));
      }
    }

    for(int i=0; i<8; i++)
    {
      bool found=false;
      Loop *l = Worklist[i];

      for (std::pair p : adj)
      {
        if(p.first==l || p.second==l)
        {
          found=true;
        }
      }

      if (!found)
      {
        Worklist.erase(Worklist.begin() + i); 
      }
    }

    //Control Flow Equivalent
    DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F); 
    PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);

    //Vettore che indica se un loop domina ed è post-dominato dal loop successivo
    SmallVector<bool> CFE;

    for (std::pair p : adj)
    {
      bool d = false;
      //Ultima istruzione del primo loop
      Loop *l1 = p.first;
      BasicBlock *b1 = l1->getExitBlock();
      const Instruction &i1 = b1->back();

      //Prima istruzione del secondo loop
      Loop *l2 = p.second;
      BasicBlock *b2 = l2->getHeader();
      const Instruction &i2 = b2->front();

      if (DT.dominates(b1,b2) && PDT.dominates(&i2,&i1))
      {
        d = true;
      }

      CFE.push_back(d);

    }

    SmallVector<Loop *, 8> Updated;
    //Elimino loops che non possono più soddisfare i requisiti
    for (int i=1; i<adj.size(); i++)
    {
      if(CFE[i])
      {
        bool found = false;
        for (Loop* comp : Updated)
        {
          if (adj[i].first != comp)
            found = true;
        }
        if(!found)
          Updated.push_back(adj[i].first);

        found = false;
        for (Loop* comp : Updated)
        {
          if (adj[i].second != comp)
            found = true;
        }
        if(!found)
          Updated.push_back(adj[i].second);
      }
    }

    //Loop Trip Count
    ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);

    SmallVector<unsigned, 8> LTC;
    
    for (Loop* l : Updated)
    {
      LTC.push_back(SE.getSmallConstantTripCount(l));
    }

  	return PreservedAnalyses::all();
  }

  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  static bool isRequired() { return true; }
};
} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getTestPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "TestPass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "loop_fusion") {
                    FPM.addPass(TestPass());
                    return true;
                  }
                  return false;
                });
          }};
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize TestPass when added to the pass pipeline on the
// command line, i.e. via '-passes=test-pass'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getTestPassPluginInfo();
}
