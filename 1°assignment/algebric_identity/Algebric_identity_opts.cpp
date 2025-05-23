//=============================================================================
// FILE:
//    algebric_identity_opts.cpp
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
#include <cmath>

using namespace llvm;

//-----------------------------------------------------------------------------
// TestPass implementation
//-----------------------------------------------------------------------------
// No need to expose the internals of the pass to the outside world - keep
// everything in an anonymous namespace.
namespace {


// New PM implementation
struct TestPass: PassInfoMixin<TestPass> {

  void replaceOperand(Instruction &Inst, int idxreg)
  {
    //itero sugli usi per trovare dove uso il registro che voglio sostituire
    for (auto Iter = Inst.user_begin(); Iter != Inst.user_end(); ++Iter) {
      Instruction &InstAdd = *(dyn_cast<Instruction>(*Iter));

      //se è il primo operando è lo stesso registro del risultato dell'operazione da sostituire
      if (&Inst == InstAdd.getOperand(0))
      {
        //sostituisco l'operando dell'istruzione con quello dell'operazione scorsa (avendo lo stesso valore)
        InstAdd.setOperand(0, Inst.getOperand(idxreg));
      } else if (&Inst == InstAdd.getOperand(1)) //se è il secondo operando faccio lo stesso
      {
        //sostituisco l'operando dell'istruzione con quello dell'operazione scorsa (avendo lo stesso valore)
        InstAdd.setOperand(1, Inst.getOperand(idxreg));
      }
    }
  }

  bool runOnBasicBlock(BasicBlock &B) {

    std::vector<Instruction*> toErase;

    //itero le istruzioni
    for (auto It = B.begin(); It != B.end(); ++It) {
      Instruction &Inst = *It;
      //se è una add 
      if (Inst.getOpcode() == Instruction::Add) {
        // Converto a int il secondo operatore
        if (ConstantInt *C = dyn_cast<ConstantInt>(Inst.getOperand(1))) {
          //guardo se è zero
          if (C->getSExtValue() == 0) {
            //Aggiungo l'istruzione a quelle da rimuovere
            toErase.push_back(&Inst);
            //il registro è da sostituire con il suo operando
            replaceOperand(Inst,0);
          }
        }
        // Converto a int il primo operatore
        else if (ConstantInt *C = dyn_cast<ConstantInt>(Inst.getOperand(0))) {
          //guardo se è zero
          if (C->getSExtValue() == 0) {
            //Aggiungo l'istruzione a quelle da rimuovere
            toErase.push_back(&Inst);
            //il registro è da sostituire con il suo operando
            replaceOperand(Inst,1);
          }
        }
      }
      //se è una mull
      else if (Inst.getOpcode() == Instruction::Mul) {
        // Converto a int il secondo operatore
        if (ConstantInt *C = dyn_cast<ConstantInt>(Inst.getOperand(1))) {
          //guardo se è uno
          if (C->getSExtValue() == 1) {
            //Aggiungo l'istruzione a quelle da rimuovere
            toErase.push_back(&Inst);
            //il registro è da sostituire con il suo operando
            replaceOperand(Inst,0);
          }
        }
        // Converto a int il primo operatore
        else if (ConstantInt *C = dyn_cast<ConstantInt>(Inst.getOperand(0))) {
          //guardo se è uno
          if (C->getSExtValue() == 1) {
            //Aggiungo l'istruzione a quelle da rimuovere
            toErase.push_back(&Inst);
            //il registro è da sostituire con il suo operando
            replaceOperand(Inst,1);
          }
        }
      }
    }

    for (Instruction *I : toErase){
      errs() << "Erasing instruction: " << *I << "\n";
      if (I->use_empty()) 
        I->eraseFromParent();
      else
        errs() << "Error erasing instruction: " << *I << "\n";
    }

    return true;
  }


bool runOnFunction(Function &F) {
  bool Transformed = false;

  for (auto Iter = F.begin(); Iter != F.end(); ++Iter) {
    if (runOnBasicBlock(*Iter)) {
      Transformed = true;
    }
  }

  return Transformed;
}


  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    outs() << runOnFunction(F) << "\n";
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
                  if (Name == "algebric_identity_opts") {
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
