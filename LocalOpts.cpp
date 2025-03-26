//=============================================================================
// FILE:
//    LocalOpts.cpp
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

// in /build:
// export LLVM_DIR=/usr/lib/llvm-19/bin
// cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR ..
// make
// export PATH=/usr/lib/llvm-19/bin:$PATH
// opt -load-pass-plugin ./libLocalOpts.so -passes=local-opts ../test/Foo.ll -o ../test/Foo.optimized.bc
// llvm-dis ../test/Foo.optimized.bc -o ../test/Foo.optimized.ll
// 

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

  bool runOnBasicBlock(BasicBlock &B) {
    
    // Preleviamo le prime due istruzioni del BB
    Instruction &Inst1st = *B.begin(), &Inst2nd = *(++B.begin());

    // L'indirizzo della prima istruzione deve essere uguale a quello del 
    // primo operando della seconda istruzione (per costruzione dell'esempio)
    assert(&Inst1st == Inst2nd.getOperand(0));

    // Stampa la prima istruzione
    outs() << "PRIMA ISTRUZIONE: " << Inst1st << "\n";
    // Stampa la prima istruzione come operando
    outs() << "COME OPERANDO: ";
    Inst1st.printAsOperand(outs(), false);
    outs() << "\n";

    // User-->Use-->Value
    outs() << "I MIEI OPERANDI SONO:\n";
    for (auto *Iter = Inst1st.op_begin(); Iter != Inst1st.op_end(); ++Iter) {
      Value *Operand = *Iter;

      if (Argument *Arg = dyn_cast<Argument>(Operand)) {
        outs() << "\t" << *Arg << ": SONO L'ARGOMENTO N. " << Arg->getArgNo() 
	       <<" DELLA FUNZIONE " << Arg->getParent()->getName()
               << "\n";
      }
      if (ConstantInt *C = dyn_cast<ConstantInt>(Operand)) {
        outs() << "\t" << *C << ": SONO UNA COSTANTE INTERA DI VALORE " << C->getValue()
               << "\n";
      }
    }

    outs() << "LA LISTA DEI MIEI USERS:\n";
    for (auto Iter = Inst1st.user_begin(); Iter != Inst1st.user_end(); ++Iter) {
      outs() << "\t" << *(dyn_cast<Instruction>(*Iter)) << "\n";
    }

    outs() << "E DEI MIEI USI (CHE E' LA STESSA):\n";
    for (auto Iter = Inst1st.use_begin(); Iter != Inst1st.use_end(); ++Iter) {
      outs() << "\t" << *(dyn_cast<Instruction>(Iter->getUser())) << "\n";
    }

    // ESERCIZIO 1
/*
    // Manipolazione delle istruzioni
    Instruction *NewInst = BinaryOperator::Create(
        Instruction::Add, Inst1st.getOperand(0), Inst1st.getOperand(0)); // getOperand(0) = %1 cioe il primo operando dell'istruzione 1

    NewInst->insertAfter(&Inst1st);
    // Si possono aggiornare le singole references separatamente?
    // Controlla la documentazione e prova a rispondere.
    // Sì, la classe Use contiene metodo set, e puo' usare il metodo addUse di Value
    Inst1st.replaceAllUsesWith(NewInst);
*/
    

    // ESERCIZIO 2
    for (auto It = B.begin(); It != B.end(); ++It) {
      Instruction &Inst = *It; 
      if (Inst.getOpcode() == Instruction::Mul) {
          if (ConstantInt *C = dyn_cast<ConstantInt>(Inst.getOperand(1))) {
            // Se è logaritmo esatto in base 2 allora inserisco shift
            if (C->getValue().exactLogBase2() != -1) {
              Instruction *NewInst = BinaryOperator::Create(Instruction::Shl, Inst.getOperand(0), 
                ConstantInt::get(Type::getInt32Ty(Inst.getContext()), C->getValue().logBase2()));
              NewInst->insertAfter(&Inst);
              Inst.replaceAllUsesWith(NewInst);
              }
          }
  
          else if (ConstantInt *C = dyn_cast<ConstantInt>(Inst.getOperand(0))) {
            if (C->getValue().exactLogBase2() != -1) {
              Instruction *NewInst = BinaryOperator::Create(Instruction::Shl, Inst.getOperand(1), 
                ConstantInt::get(Type::getInt32Ty(Inst.getContext()), C->getValue().logBase2()));
              NewInst->insertAfter(&Inst);
              Inst.replaceAllUsesWith(NewInst);
            }
        }
      }
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
                  if (Name == "local-opts") {
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
