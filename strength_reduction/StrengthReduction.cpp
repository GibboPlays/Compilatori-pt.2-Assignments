//=============================================================================
// FILE:
//    StrengthReduction.cpp
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
  void applyStrengthReduction(std::vector<Instruction*> &toErase, Instruction *Inst, Instruction::BinaryOps ShiftOp, ConstantInt *C, int opNumber){
    // toErase = vecotr di istruzioni vecchie (da eliminare successivamente);
    // Inst = istruzione corrente;
    // ShiftOp = codice operazione (Shl o AShr);
    // C = operando ConstantInt;
    // opNumber = numero dell'operando di tipo registro (non ConstantInt).

    // Se l'operando è logaritmo esatto
    if (C->getValue().exactLogBase2() != -1) {
      Instruction *ShInst = BinaryOperator::Create(ShiftOp, Inst->getOperand(opNumber), 
        ConstantInt::get(Type::getInt32Ty(Inst->getContext()), C->getValue().logBase2()));
      ShInst->insertAfter(Inst);
      Inst->replaceAllUsesWith(ShInst);
      toErase.push_back(Inst);
    }
    // L'operando non è logaritmo esatto
    // Se sottraendo 1 all'operando è logaritmo esatto
    else if ((C->getValue() - 1).exactLogBase2() != -1) {
      Instruction *ShInst = BinaryOperator::Create(ShiftOp, Inst->getOperand(opNumber), 
        ConstantInt::get(Type::getInt32Ty(Inst->getContext()), (C->getValue() - 1).logBase2()));
      ShInst->insertAfter(Inst);
      // Se è Mul
      if (ShiftOp == Instruction::Shl){
        Instruction *AddInst = BinaryOperator::Create(Instruction::Add, ShInst, Inst->getOperand(opNumber));       
        AddInst->insertAfter(ShInst);
        Inst->replaceAllUsesWith(AddInst);
      }
      // Se è Sdiv
      else if (ShiftOp == Instruction::AShr){
        Instruction *SubInst = BinaryOperator::Create(Instruction::Sub, ShInst, Inst->getOperand(opNumber));
        SubInst->insertAfter(ShInst);
        Inst->replaceAllUsesWith(SubInst);
      }
      toErase.push_back(Inst);
    }
    // Se aggiungendo 1 all'operando è logaritmo esatto
    else if ((C->getValue() + 1).exactLogBase2() != -1) {
      Instruction *ShInst = BinaryOperator::Create(ShiftOp, Inst->getOperand(opNumber), 
        ConstantInt::get(Type::getInt32Ty(Inst->getContext()), (C->getValue() + 1).logBase2()));
      ShInst->insertAfter(Inst);
      // Se è Mul
      if (ShiftOp == Instruction::Shl){
        Instruction *SubInst = BinaryOperator::Create(Instruction::Sub, ShInst, Inst->getOperand(opNumber));
        SubInst->insertAfter(ShInst);
        Inst->replaceAllUsesWith(SubInst);
      }
      // Se è Sdiv
      else if (ShiftOp == Instruction::AShr){
        Instruction *AddInst = BinaryOperator::Create(Instruction::Add, ShInst, Inst->getOperand(opNumber));
        AddInst->insertAfter(ShInst);
        Inst->replaceAllUsesWith(AddInst);
      }
      toErase.push_back(Inst);
    }
  }

  bool runOnBasicBlock(BasicBlock &B) {
    std::vector<Instruction*> toErase;
    for (auto It = B.begin(); It != B.end(); ++It) {
      Instruction &Inst = *It; 
      if (Inst.getOpcode() == Instruction::Mul) {
        if (ConstantInt *C = dyn_cast<ConstantInt>(Inst.getOperand(1))) {
          // Se il valore (signed) dell'operando C è positivo (altrimenti errori nei casi limite)
          if (C->getValue().getSExtValue() > 0)
            applyStrengthReduction(toErase, &Inst, Instruction::Shl, C, 0);              
        }
        else if (ConstantInt *C = dyn_cast<ConstantInt>(Inst.getOperand(0))) {
          if (C->getValue().getSExtValue() > 0)
            applyStrengthReduction(toErase, &Inst, Instruction::Shl, C, 1);
        }
      }
      else if (Inst.getOpcode() == Instruction::SDiv){
        if (ConstantInt *C = dyn_cast<ConstantInt>(Inst.getOperand(1))) {
          if (C->getValue().getSExtValue() > 0)
            applyStrengthReduction(toErase, &Inst, Instruction::AShr, C, 0);   
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
                  if (Name == "strength-reduction") {
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
