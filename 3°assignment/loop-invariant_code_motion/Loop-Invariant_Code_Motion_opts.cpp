//=============================================================================
// FILE:
//    Loop-invariant_Code_Motion_opts.cpp
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
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/BitVector.h"
#include <cmath>
#include <vector>
#include <map>

using namespace llvm;

//-----------------------------------------------------------------------------
// TestPass implementation
//-----------------------------------------------------------------------------
// No need to expose the internals of the pass to the outside world - keep
// everything in an anonymous namespace.
namespace {


// New PM implementation
struct TestPass: PassInfoMixin<TestPass> {

  std::map<BasicBlock*,std::pair<BitVector,BitVector>> getReachingDefinitions(Function &F)
  {
    std::map<BasicBlock*,std::pair<BitVector,BitVector>> rd;
    unsigned int numInst = F.getInstructionCount();
    BitVector in(numInst), out(numInst);
    std::pair<BitVector,BitVector> p(in,out);
    std::vector<BasicBlock*> changedNodes;

    //Initialize
    for (BasicBlock &bb : F)
    {
      rd[&bb]=p;
      changedNodes.append(&bb);
    }

    //Iterate
    while(!changedNodes.empty())
    {
      
    }


    return rd;

  }

  std::vector<std::pair<Instruction*,Loop*>> getLoopInvariants(LoopInfo &LI)
  {
    //Vettore di coppie istruzioni Loop-Invariant e loop a cui appartiene
    std::vector<std::pair<Instruction*,Loop*>> LII;
    //Per capire se l'istruzione usata è loop invariant
    bool li1;
    //Per capire se l'istruzione che usa è loop invariant
    bool li2;

    //Vettore dei definiti non Loop-Invariant
    std::vector<Instruction*> defined;
    for (Loop *L: LI)
    {
      for (BasicBlock *BB : L->blocks()) 
      {
          
        for (auto It = BB->begin(); It != BB->end(); ++It) 
        {
          Instruction &Inst = *It;

          li1 = true;

          for (auto Iter = Inst.user_begin(); Iter != Inst.user_end(); ++Iter)
          {
            Instruction &InstAdd = *(dyn_cast<Instruction>(*Iter));

            li2 = true;

            //Se l'istruzione che usa è già in defined è inutile continuare
            for (Instruction *i: defined)
            {
              if (&InstAdd == i)
              {
                li2 = false;
                break;
              }
            }
            
            if (li2)
            {
              //Guardo se l'istruzione di cui sto cercando gli usi è in defined
              for (Instruction *i: defined)
              {
                if (&Inst == i)
                {
                  li1 = false;
                  li2 = false;
                  defined.push_back(&InstAdd);
                  break;
                }
              }
            }
            
            //Se ha passato non è in defined può essere candidata a essere loop invariant
            if (li2 && L->contains(InstAdd.getParent()))
            {
              std::pair<Instruction*,Loop*> p1(&InstAdd,L);
              LII.push_back(p1);
            }

            
          }

          if (li1)
          {
            std::pair <Instruction*,Loop*> p(&Inst,L);
            LII.push_back(p);
          } 
        }
    
      }

      defined.clear();
    }

    return LII;
  }

  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {

/* Lab_3

     //Esercitazione 1 

     LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
    
     if (LI.empty())
     {
       outs() << "Nessun loop trovato nella funzione " << F.getName() << "\n";
       return PreservedAnalyses::all();
     }
 
     for (BasicBlock &BB : F) {
       if (LI.isLoopHeader(&BB)) {
         outs() << "\nBB '" << BB << "' è header di loop";
       }
     }
     
     for (Loop *L : LI) {
       bool norm = true;
       
       if (!L->getLoopPreheader()) {
         norm = false;
       }
       if (L->getNumBackEdges() != 1) {
         norm = false;
       }
       if (!L->hasDedicatedExits()) {
         norm = false;
       }
            
       if (norm)
       {
         outs() << "Loop in forma normale" << "\n";
       } else {
         outs() << "Loop NON in forma normale" << "\n";
       }
 
       Function *Fadd = L->getHeader()->getParent();
 
       for (BasicBlock &Bt: *Fadd)
       {
         outs() << "  BB '" << Bt << "' -> ";
 
         for (BasicBlock *Succ : successors(&Bt)) 
         {
           outs() << Succ<< " ";
         }
         outs() << "\n";
       } 
             
 
       outs() << "- Blocchi del loop:\n";
          
       for (BasicBlock *BB : L->blocks()) {
           
         outs() << "  - " << BB << "\n";
     
       }
 
 
     }
 
     //Esercitazione 2
 
     DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
     
     outs() << "Stampa Dominator Tree:\n";
     for (auto *DTN : depth_first(DT.getRootNode())) 
     {
       BasicBlock &B = *DTN->getBlock();
       outs() << B << "\n";
     }
    
*/

    LoopInfo &LI = AM.getResult<LoopAnalysis>(F);

    DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);

    //Debug
    std::map<BasicBlock*,std::pair<BitVector,BitVector>> rd = getReachingDefinitions(F);
    for (auto [bb,p]: rd)
    {
      outs() << bb->getName() << ": ";
      for (unsigned i = 0; i < p.first.size(); ++i) {
        outs() << (p.first.test(i) ? "1" : "0");
      }
      outs() << " ";
      for (unsigned i = 0; i < p.second.size(); ++i) {
        outs() << (p.second.test(i) ? "1" : "0");
      }
      outs() << "\n";
    }

    
    //Istruzioni Loop-Invariant
    std::vector<std::pair<Instruction*,Loop*>> LII = getLoopInvariants(LI);

    //Scorro le coppie dei loop invariant
    for (std::pair<Instruction*,Loop*> *p : LII)
    {
      Instruction *i = p->first;
      Loop *l = p->second;
      
      SmallVector <BasicBlock*> ebs = l->getExitingBlocks();

      bool dom = true;

      for (BasicBlock *eb : ebs)
      {
        dom = DT.dominates(i, eb);
        if (!dom) break;
      }

      if (dom)
      {
        
      }
      

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
                  if (Name == "loop-invariant_code_motion_opts") {
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
