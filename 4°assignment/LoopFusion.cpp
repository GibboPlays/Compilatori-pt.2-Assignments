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
    errs() << "Worklist ha size " << Worklist.size() << "\n";
    
    //Checking conditions

    //Adjacent
    SmallVector<std::pair<Loop*,Loop*>> adj;

    for (size_t i = 0; i + 1 < Worklist.size(); i++) {
      //Ultimo basic block del primo loop
      errs() << "Cerco adiacenze tra: "
         << Worklist[i]->getHeader()->getName() << " e "
         << Worklist[i+1]->getHeader()->getName() << "\n";
      Loop *l1 = Worklist[i];
      BasicBlock *b1 = l1->getExitBlock();
      //Primo basic block del secondo loop
      Loop *l2 = Worklist[i+1];
      BasicBlock *b2 = l2->getHeader();
      if (!b1 || !b2) continue;
      if (b1->getSingleSuccessor() == b2->getSinglePredecessor())
      {
        adj.push_back(std::make_pair(l1,l2));
      }
    }

    errs() << "adj ha size " << adj.size() << "\n";

    SmallVector<Loop*, 8> Filtered;
    for (Loop* l : Worklist){
      bool found = false;
      for (const auto &p : adj) {
        if (p.first == l || p.second == l)
          found = true;
      }
      if (found)
        //Worklist.erase(Worklist.begin() + i);
        Filtered.push_back(l);
    }
    Worklist = Filtered;
    errs() << "Worklist (aggiornato dopo adiacenze) ha size " << Worklist.size() << "\n";

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

    errs() << "Updated (dopo CFE) ha size " << Updated.size() << "\n";

    //Loop Trip Count
    ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
    std::map<Loop*, unsigned> TripCount; // uso mappa per facilitare la dependence analysis
    //SmallVector<unsigned, 8> LTC;
    
    for (Loop* l : Updated)
    {
      unsigned TC = SE.getSmallConstantTripCount(l);
      if (TC > 0) TripCount[l] = TC;
    }

    errs() << "Ho fatto TripCount\n";

    //Dependence Analysis
    SmallVector<Loop *, 8> Fusable;
    DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);
    for (int i = 0; i < Updated.size()-1; ++i) {
      Loop *l1 = Updated[i];
      Loop *l2 = Updated[i+1];
      if (TripCount.count(l1) == 0 || TripCount.count(l2) == 0) continue; // è necessario o posso toglierlo?
      bool hasNegativeDependence = false;
      for (auto *bb1 : l1->blocks()){
        for (auto &i1 : *bb1){
          for (auto *bb2 : l2->blocks()){
            for (auto &i2 : *bb2){
              if(auto dep = DI.depends(&I1, &I2, true))
                if (dep->getDistance() && dep->getDistance()->isNegative())
                {
                  hasNegativeDependence = True;
                  break;
                }
            }
            if (hasNegativeDependence) break;
          }
          if (hasNegativeDependence) break;
        }
        if (hasNegativeDependence) break;
      }
      if (!hasNegativeDependence) {
        // Fusable potrebbe contenere dei duplicati... è un problema?
        Fusable.push_back({l1, l2});
      }
    }

    errs() << "Fusable ha size " << Fusable.size() << "\n";
    
    //Trasformazione del codice
    for (auto &p : Fusable){
      Loop *l1 = p.first;
      Loop *l2 = p.second;

      //Controlli sulla struttura dei loop
      SmallVector<BasicBlock *, 2> latches1, latches2;
      l1->getLoopLatches(latches1);
      l2->getLoopLatches(latches2);
      if (latches1.size() != 1 || latches2.size() != 1) continue;
      BasicBlock *latch1 = latches1[0];
      BasicBlock *header2 = l2->getHeader();
      BasicBlock *preheader2 = l2->getLoopPreheader();
      if (!latch1 || !header2 || !preheader2) continue;

      //Modificare gli usi
      PHINode *IV1 = l1->getInductionVariable(SE);
      PHINode *IV2 = l2->getInductionVariable(SE);
      if (!IV1 || !IV2) continue;
      IV2->replaceAllUsesWith(IV1);

      //Modificare il CFG
      Instruction *Term = latch1->getTerminator();
      for (unsigned i = 0; i < Term->getNumSuccessors(); ++i) {
        if (Term->getSuccessor(i) == l1->getHeader()) { // se il salto torna all'inizio del loop
          Term->setSuccessor(i, header2);
          break;
        }
      }
      preheader2->eraseFromParent(); // rimuovo il preheader che non serve più
      LI.erase(l2); // rimuovo l2 che non serve più
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
