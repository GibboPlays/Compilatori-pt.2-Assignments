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
#include "llvm/IR/CFG.h"
#include "llvm/ADT/SmallVector.h"
#include <cmath>
#include <vector>
#include <map>
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

  std::map<BasicBlock*,std::pair<std::set<Instruction*>,std::set<Instruction*>>> getReachingDefinitions(Function &F)
  {
    std::map<BasicBlock*,std::pair<std::set<Instruction*>,std::set<Instruction*>>> rd;
    unsigned int numInst = F.getInstructionCount();
    std::set<Instruction*> in, out;
    std::pair<std::set<Instruction*>,std::set<Instruction*>> p(in,out);
    std::vector<BasicBlock*> changedNodes;

    //Initialize
    for (BasicBlock &bb : F)
    {
      rd[&bb]=p;
      changedNodes.push_back(&bb);
    }

    // Condizione al contorno: out[entry] = ∅
    BasicBlock *entry = &F.getEntryBlock();
    rd[entry].second.clear(); // out[entry] = ∅

    //Iterate
    while(!changedNodes.empty())
    {
      BasicBlock &bi = *(changedNodes.front());
      changedNodes.pop_back();

      std::set<Instruction*> ini; //in[i]
      //Scorro i predecessori
      for (BasicBlock *pred :  predecessors(&bi)) 
      {
        std::set outp = (rd.at(pred)).second; //out[p]
        ini.insert(outp.begin(), outp.end()); //in[i] = U out[p]
      }
      rd[&bi].first = ini;

      //Genero gen[i] e kill[i]
      std::set<Instruction*> geni, killi;
      for (Instruction &inst : bi)
      {
        if (inst.isBinaryOp())
        {
          geni.insert(&inst);
        }
        for (Instruction *instPtr : ini)
        {
          Instruction &instAdd = *instPtr;
          if (&inst == &instAdd)
          {
            killi.insert(&instAdd);
          }
        }
      }

      //Calcolo out[i]=gen[i]U(in[i]-kill[i]) 
      std::set<Instruction*> in_minus_kill;
      std::set_difference(ini.begin(), ini.end(),
                          killi.begin(), killi.end(),
                          std::inserter(in_minus_kill, in_minus_kill.begin()));
        
      std::set<Instruction*> outi;
      outi.insert(geni.begin(), geni.end());
      outi.insert(in_minus_kill.begin(), in_minus_kill.end());

      //Controllo se out[i] è cambiato
      if (rd[&bi].second != outi)
      {
        rd[&bi].second = outi;
        for (BasicBlock *succ : successors(&bi))
        {
          if (std::find(changedNodes.begin(), changedNodes.end(), succ) == changedNodes.end()) 
          {
            changedNodes.push_back(succ);
          }
        }
      }
    }


    return rd;

  }

  std::vector<std::pair<Instruction*,Loop*>> getLoopInvariants(LoopInfo &LI, 
                                                              std::map<BasicBlock*,std::pair<std::set<Instruction*>,std::set<Instruction*>>> rd)
  {
    //Vettore di coppie istruzioni Loop-Invariant e loop a cui appartiene
    std::vector<std::pair<Instruction*,Loop*>> LII;

    for (Loop *L: LI)
    {
      for (BasicBlock *BB : L->blocks()) 
      {
        const auto BBrd = rd.at(BB).first; //in[BB]

        for (Instruction& Inst : *BB)
        {
          bool isLoopInvariant = true;

          for (auto *Iter = Inst.op_begin(); Iter != Inst.op_end(); ++Iter) {
            Value *Operand = *Iter;
            
            //Se l'operando è un'istruzione
            if (Instruction* opInst = dyn_cast<Instruction>(Operand))
            {
              if (L->contains(opInst->getParent())) 
              {
                //Verifico se è loop-invariant (deve essere già nella lista)
                bool found = false;
                for (const auto &pair : LII) 
                {
                    if (pair.first == opInst && pair.second == L) {
                        found = true;
                        break;
                    }
                }
                
                if (!found) 
                {
                    isLoopInvariant = false;
                    break;
                }
              } 
              //Se la definizione viene da fuori il loop, verifica che sia nelle reaching definitions
              else 
              {
                if (BBrd.find(opInst) == BBrd.end()) 
                {
                    isLoopInvariant = false;
                    break;
                }
              }
            } else { //Se non è una istruzione
              isLoopInvariant = false;
              break;
            }
          
          }

          if (isLoopInvariant) {
            LII.emplace_back(&Inst, L);
          }
        }
      }
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
    std::map<BasicBlock*,std::pair<std::set<Instruction*>,std::set<Instruction*>>> rd = getReachingDefinitions(F);
    //Istruzioni Loop-Invariant
    std::vector<std::pair<Instruction*,Loop*>> LII = getLoopInvariants(LI,rd);
    std::vector<Instruction *> Candidates
    
    std::vector<BasicBlock *> ExitBlocks;
    for (Loop *L : LI) {
      for (BasicBlock *BB : L->blocks()) {
        for (BasicBlock *Succ : successors(BB))
          if (!L->contains(Succ)) ExitBlocks.push_back(Succ);
      }
    }

    

    // Cercare istruzioni candidate

    for (auto &p : LII) {
      Instruction *Inst = p.first;
      Loop *L = p.second;
      // Si trovano in blocchi che dominano tutte le uscite del loop
      bool dom_uscite = true;
      for (BasicBlock *ExitBB : ExitBlocks) {
        if (!DT.dominates(Inst->getParent(), ExitBB)) {
          dom_uscite = false;
          break;
        }
      }
      // Oppure la variabile definita dall’istruzione è dead all’uscita del loop
      bool dead = true;
      for (User *U : Inst->users()) { // Scorro gli user dell'istruzione
        if (Instruction *UserInst = dyn_cast<Instruction>(U)) {
          if (L->contains(UserInst->getParent())) { // Se è nel loop
            dead = false;
            break;
          }
        }
      }
      // Assegnano un valore a variabili non assegnate altrove nel loop
      bool assegn_unico = true;
      for (BasicBlock *LoopBB : L->blocks()) {
        for (Instruction &OtherInst : *LoopBB) {
          if (&OtherInst != Inst && OtherInst.getName() == Inst->getName()) {
            assegn_unico = false;
            break;
          }
        }
        if (!assegn_unico) break;
      }
      // Si trovano in blocchi che dominano tutti i blocchi nel loop che usano la variabile a cui si sta assegnando un valore
      bool dom_all_b = true;
      for (BasicBlock *LoopBB : L->blocks()) {
        if (!DT.dominates(Inst->getParent(), LoopBB)) {
          dom_all_b = false;
          break;
        }
      }
      // Se tutte le condizioni sono soddisfatte, l'istruzione è candidata per essere spostata
      if (dom_uscite && (dead || assegn_unico) && dom_all_b) Candidates.push_back(Inst);
    }

    // Eseguire una ricerca depth-first dei blocch
    std::vector<BasicBlock*> BBOrdinatiDFS;
    for (BasicBlock *BB : llvm::depth_first(&F.getEntryBlock()))
      BBOrdinatiDFS.push_back(BB);

    // Spostare l’istruzione candidata nel preheader se tutte le istruzioni invarianti da cui questa dipende sono state spostate
    Loop *L = LI.getLoopFor(&F.getEntryBlock());
      if (L){
      BasicBlock *Preheader = L->getLoopPreheader();
    if (Preheader) {
      std::vector<Instruction*> moved; // per istruzioni già spostate
      for (Instruction *Inst : Candidates) {
        bool tutteSpostate = true;
        for (Use &U : Inst->operands())
          // Se trovo una che non è ancora stata spostata
          if (Instruction *OpInst = dyn_cast<Instruction>(U.get()))
            if (std::find(Candidates.begin(), Candidates.end(), OpInst) != Candidates.end() &&
                std::find(moved.begin(), moved.end(), OpInst) == moved.end()) {
                tutteSpostate = false;
                break;
            }     
        if (tutteSpostate) {
          Inst->moveBefore(Preheader->getTerminator());
          moved.push_back(Inst);
        }
      }
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
