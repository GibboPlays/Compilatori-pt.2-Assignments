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

    // 1 ASSIGNMENT

    for (auto It = B.begin(); It != B.end(); ++It) {
      Instruction &Inst = *It; 
      if (Inst.getOpcode() == Instruction::Add) {
          if (ConstantInt *C = dyn_cast<ConstantInt>(Inst.getOperand(1))) {
            // Converto a int e guardo se è zero
            if (C->getSExtValue() == 0) {
              
              for (auto Iter = Inst1st.user_begin(); Iter != Inst1st.user_end(); ++Iter) { //itero sugli usi per trovare dove uso il registro che voglio sostituire
                Instruction InstAdd = dyn_cast<Instruction>(*Iter);

                if (&Inst == InstAdd.getOperand(0))
                {
                  InstAdd->setOperand(0, Inst.getOperand(0)); //sostituisco l'operando dell'istruzione con quello dell'operazione scorsa (avendo lo stesso valore)
                } else if (&Inst == InstAdd.getOperand(1))
                {
                  InstAdd->setOperand(1, Inst.getOperand(0));
                }
              }
            }
          }
  
          else if (ConstantInt *C = dyn_cast<ConstantInt>(Inst.getOperand(0))) {
            if (C->getSExtValue() == 0) {
              for (auto Iter = Inst1st.user_begin(); Iter != Inst1st.user_end(); ++Iter) { //itero sugli usi per trovare dove uso il registro che voglio sostituire
                Instruction InstAdd = dyn_cast<Instruction>(*Iter);

                if (&Inst == InstAdd.getOperand(0))
                {
                  InstAdd->setOperand(0, Inst.getOperand(0)); //sostituisco l'operando dell'istruzione con quello dell'operazione scorsa (avendo lo stesso valore)
                } else if (&Inst == InstAdd.getOperand(1))
                {
                  InstAdd->setOperand(1, Inst.getOperand(0));
                }
              }
            }
        }
      }
      else if (Inst.getOpcode() == Instruction::Mul) {
        if (ConstantInt *C = dyn_cast<ConstantInt>(Inst.getOperand(1))) {
          // Converto a int e guardo se è uno
          if (C->getSExtValue() == 1) {
            for (auto Iter = Inst1st.user_begin(); Iter != Inst1st.user_end(); ++Iter) { //itero sugli usi per trovare dove uso il registro che voglio sostituire
              Instruction InstAdd = dyn_cast<Instruction>(*Iter);

              if (&Inst == InstAdd.getOperand(0))
              {
                InstAdd->setOperand(0, Inst.getOperand(0)); //sostituisco l'operando dell'istruzione con quello dell'operazione scorsa (avendo lo stesso valore)
              } else if (&Inst == InstAdd.getOperand(1))
              {
                InstAdd->setOperand(1, Inst.getOperand(0));
              }
            }
          }
        }

        else if (ConstantInt *C = dyn_cast<ConstantInt>(Inst.getOperand(0))) {
          if (C->getSExtValue() == 1) {
            for (auto Iter = Inst1st.user_begin(); Iter != Inst1st.user_end(); ++Iter) { //itero sugli usi per trovare dove uso il registro che voglio sostituire
              Instruction InstAdd = dyn_cast<Instruction>(*Iter);

              if (&Inst == InstAdd.getOperand(0))
              {
                InstAdd->setOperand(0, Inst.getOperand(0)); //sostituisco l'operando dell'istruzione con quello dell'operazione scorsa (avendo lo stesso valore)
              } else if (&Inst == InstAdd.getOperand(1))
              {
                InstAdd->setOperand(1, Inst.getOperand(0));
              }
            }
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
