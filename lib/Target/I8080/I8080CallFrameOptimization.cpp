//===----- I8080CallFrameOptimization.cpp - Optimize z80 call sequences -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines a pass that optimizes call sequences on z80.
//
//===----------------------------------------------------------------------===//

#include "I8080.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

using namespace llvm;

#define DEBUG_TYPE "z80-cf-opt"

static cl::opt<bool>
NoI8080CFOpt("no-i8080-call-frame-opt",
           cl::desc("Avoid optimizing i8080 call frames"),
           cl::init(false), cl::Hidden);

namespace {
class I8080CallFrameOptimization : public MachineFunctionPass {
public:
  I8080CallFrameOptimization() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

private:
  static char ID;
};

char I8080CallFrameOptimization::ID = 0;
} // end anonymous namespace

FunctionPass *llvm::createI8080CallFrameOptimization() {
  return new I8080CallFrameOptimization();
}

bool I8080CallFrameOptimization::runOnMachineFunction(MachineFunction &MF) {
  if (skipFunction(MF.getFunction()) || NoI8080CFOpt.getValue()) {
    return false;
  }
  return false;
}
