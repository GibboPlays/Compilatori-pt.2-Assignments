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
#include "llvm/Analysis/DependenceAnalysis.h"
#include <set>
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



  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {

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
      Loop *l1 = Worklist[i+1];
      BasicBlock *b1header = l1->getHeader();
      errs() << "b1header: " << b1header->getName() << "\n";
      BasicBlock *b1exitblock = l1->getExitBlock();
      errs() << "b1exitblock: " << b1exitblock->getName() << "\n";
      //Primo basic block del secondo loop
      Loop *l2 = Worklist[i];
      BasicBlock *b2header = l2->getHeader();
      errs() << "b2header: " << b2header->getName() << "\n";
      BasicBlock *b2preheader = nullptr;
      for (BasicBlock *Pred : predecessors(b2header)) {
        if (!l2->contains(Pred)) {
          b2preheader = Pred;
        }
      }
      errs() << "b2preheader: " << b2preheader->getName() << "\n";
      if (!b1header || !b1exitblock || !b2header || !b2preheader) continue;
      //Guarded
      BasicBlock *b1headersuccessor = nullptr;
      if (BranchInst *br = dyn_cast<BranchInst>(b1header->getTerminator()))
      {
        if (br->isConditional()) {
          BasicBlock *succ0 = br->getSuccessor(0);
          BasicBlock *succ1 = br->getSuccessor(1);
          // Supponiamo che il corpo sia il ramo 'true' (condizione vera)
          if (l1->contains(succ0))
            b1headersuccessor = succ1;
          else if (l1->contains(succ1))
            b1headersuccessor = succ0;
        }
      }
      if (b1headersuccessor == b2header)
      {
        adj.push_back(std::make_pair(l1,l2));
      } //Not guarded
      else if (b1exitblock == b2preheader)
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
      Loop *L0 = p.first;
      Loop *L1 = p.second;

      BasicBlock *H0 = L0->getHeader();
      BasicBlock *H1 = L1->getHeader();

      bool equiv = false;

      if (DT.dominates(H0, H1) && PDT.dominates(H1, H0) || DT.dominates(H1, H0) && PDT.dominates(H0, H1)) { // ordine dei loop in Worklist può essere diverso
          equiv = true;
      }

    CFE.push_back(equiv);

    }

    SmallVector<Loop *, 8> Updated;
    //Elimino loops che non possono più soddisfare i requisiti
    for (int i=0; i<adj.size(); i++)
    {
      if (CFE[i]) 
      {
        if (std::find(Updated.begin(), Updated.end(), adj[i].first) == Updated.end())
            Updated.push_back(adj[i].first);

        if (std::find(Updated.begin(), Updated.end(), adj[i].second) == Updated.end())
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
    SmallVector<std::pair<Loop*, Loop*>, 8> Fusable;
    DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);
    for (size_t i = 0; i + 1 < Updated.size(); ++i) {
      Loop *l1 = Updated[i];
      Loop *l2 = Updated[i+1];
      if (TripCount.count(l1) == 0 || TripCount.count(l2) == 0) continue; // è necessario o posso toglierlo?
      bool hasDependence = false;
      for (auto *bb1 : l1->blocks()){
        for (auto &i1 : *bb1){
          for (auto *bb2 : l2->blocks()){
            for (auto &i2 : *bb2){
              if (auto dep = DI.depends(&i1, &i2, true))
                hasDependence = true;
            }
            if (hasDependence) break;
          }
          if (hasDependence) break;
        }
        if (hasDependence) break;
      }
      if (!hasDependence) {
        // Fusable potrebbe contenere dei duplicati... è un problema?
        Fusable.push_back({l1, l2});
      }
    }

    errs() << "Fusable ha size " << Fusable.size() << "\n";
    
    //Trasformazione del codice
    for (auto &p : Fusable) {
      Loop *l1 = p.second;
      Loop *l2 = p.first;
      errs() << "   " << l1->getName() << ", " << l2->getName() << "\n";

      // Prendo latch e header
      SmallVector<BasicBlock *, 2> latches1, latches2;
      l1->getLoopLatches(latches1);
      l2->getLoopLatches(latches2);
      if (latches1.size() != 1 || latches2.size() != 1) {
        errs() << "---Errore size latches\n";
        continue;
      }
      BasicBlock *latch1 = latches1[0];
      BasicBlock *header1 = l1->getHeader();
      BasicBlock *header2 = l2->getHeader();
      //BasicBlock *preheader2 = l2->getLoopPreheader();
      if (!latch1) {
        errs() << "---Errore latch1\n";
        continue;
      } else if (!header2) {
        errs() << "---Errore header2\n";
        continue;
      } else if (!header1) {
        errs() << "---Errore header1\n";
        continue;
      /*} else if (!preheader2) {
        errs() << "---Errore preheader2\n";
        continue;*/
      }

      errs() << "   Controlli finiti\n";

      // Recupero IV1 e IV2
      PHINode *IV1 = dyn_cast<PHINode>(header1->begin());
      PHINode *IV2 = dyn_cast<PHINode>(header2->begin());
      if (!IV1 || !IV2) continue;

      errs() << "   PHI1: " << *IV1 << "\n";
      errs() << "   PHI2: " << *IV2 << "\n";

      /* Modificare gli usi della induction variable nel body del 
        loop 2 con quelli della induction variable del loop 1*/
      for (BasicBlock *BB : l2->blocks()) {
        if (BB == header2) continue;
        for (Instruction &I : *BB) {
          for (unsigned i = 0; i < I.getNumOperands(); ++i) {
            if (I.getOperand(i) == IV2) {
              I.setOperand(i, IV1);
            }
          }
        }
      }

      errs() << "   Modificato usi IV2\n";

      /* Modificare il CFG perché il body del loop 2 sia 
        agganciato a seguito del body del loop 1 nel loop 1*/

      BasicBlock *body2 = nullptr;
      BasicBlock *body1 = nullptr;
      if (BranchInst *br = dyn_cast<BranchInst>(header1->getTerminator())) { // ricavo body1
        if (br->isConditional()) {
          BasicBlock *succ0 = br->getSuccessor(0);
          BasicBlock *succ1 = br->getSuccessor(1);
          // Supponiamo che il corpo sia il ramo 'true' (condizione vera)
          if (l1->contains(succ0))
            body1 = succ0;
          else if (l1->contains(succ1))
            body1 = succ1;
        }
      }
      if (BranchInst *br = dyn_cast<BranchInst>(header2->getTerminator())) { // ricavo body2
        if (br->isConditional()) {
          BasicBlock *succ0 = br->getSuccessor(0);
          BasicBlock *succ1 = br->getSuccessor(1);
          if (l2->contains(succ0))
            body2 = succ0;
          else if (l2->contains(succ1))
            body2 = succ1;
        }
      }

            
      Instruction *term1 = body1->getTerminator();
      Instruction *term2 = body2->getTerminator();

      errs() << "body1: " << body1->getName() << ". body2: " << body2->getName() << "\n";
      

      errs() << "Blocchi in l2:\n";
      for (BasicBlock *BB : l2->blocks()) {
          errs() << "  " << BB->getName() << "\n";
          Instruction *term = BB->getTerminator();
          if (term)
              errs() << "    terminator: " << *term << "\n";
      }


      // header2 salta a latch2
      Instruction *termHeader2 = header2->getTerminator();
      Instruction *Br = nullptr;
      BasicBlock *latch2 = nullptr;
      for (BasicBlock *BB : l2->blocks()) {
        if (BB != header2 && BB != body2) latch2 = BB;
      }
      if (termHeader2) termHeader2->eraseFromParent();
      if (latch2){
        Br = BranchInst::Create(latch2, header2);
        errs() << "   header2 salta a latch2 " << *Br << "\n";
      } else {
        errs() << "---ERRORE: latch2 non trovato!\n";
      }
      

      // body1 salta a body2
      if (term1){
        errs() << "   Prima: body1->term = " << *term1 << "\n";
        term1->eraseFromParent(); 
      }
      IRBuilder<> builder(body1);
      Br = builder.CreateBr(body2);
      errs() << "   body1 salta a body2: " << *Br << "\n";

      // body2 salta a latch1
      term2->eraseFromParent();
      Br = BranchInst::Create(latch1, body2);
      errs() << "   body2 salta a latch1 " << *Br << "\n";

      // header1 salta a body1 o end
      //Instruction *termHeader1 = header1->getTerminator();
      //if (termHeader1) termHeader1->eraseFromParent();

      BasicBlock *endBlock = nullptr; // recupero blocco end (che prima era dopo header2 con condizione falsa)
      for (BasicBlock &BB : *header1->getParent()) {
        Instruction *term = BB.getTerminator();
        if (!term) continue; // salta blocchi malformati
        if (isa<ReturnInst>(term)) {
          endBlock = &BB;
          errs() << "   Blocco end: " << endBlock->getName() << "\n";
          break;
        }
      }
      if (!endBlock) errs() << "   Non ho trovato end!!! ERRORE\n";

      IRBuilder<> builder2(header1);
      builder2.SetInsertPoint(header1);
      Instruction *term = header1->getTerminator();
      if (term) {
        if (auto *br = dyn_cast<BranchInst>(term)) {
          Value *cond = nullptr;
          BasicBlock *trueDest = nullptr;
          BasicBlock *falseDest = nullptr;

          if (br->isConditional()) {
            cond = br->getCondition();
            trueDest = br->getSuccessor(0);
            falseDest = br->getSuccessor(1);
          } else {
            trueDest = br->getSuccessor(0);
          }

          br->eraseFromParent();

          if (cond && trueDest && endBlock) {
            Instruction *newBr = BranchInst::Create(trueDest, endBlock, cond, header1);
            errs() << "   header1 salta a body1 / end (condizione originale mantenuta): " << *newBr << "\n";
          } else if (trueDest) {
            Instruction *newBr = BranchInst::Create(trueDest, header1);
            errs() << "   header1 salta a " << trueDest->getName() << " (branch non condizionale): " << *newBr << "\n";
          } else {
            errs() << "---Errore: impossibile creare terminator valido per header1\n";
          }
        }
      }
      errs() << "   header1 salta a body1 / end\n";

      // modificare PHI node in header2
      SmallVector<std::pair<Value *, BasicBlock *>, 4> newIncomingIV2;
      for (unsigned i = 0; i < IV2->getNumIncomingValues(); ++i) {
        BasicBlock *BB = IV2->getIncomingBlock(i);
        Value *V = IV2->getIncomingValue(i);
        if (llvm::is_contained(predecessors(header2), BB)) {
          newIncomingIV2.push_back({V, BB});
        }
      }
      while (IV2->getNumIncomingValues() > 0)
        IV2->removeIncomingValue((unsigned)0, false);
      for (auto &pair : newIncomingIV2)
        IV2->addIncoming(pair.first, pair.second);

      errs() << "Fusione completata\n";


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
