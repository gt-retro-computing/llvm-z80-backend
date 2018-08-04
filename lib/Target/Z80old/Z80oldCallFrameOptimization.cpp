//===----- Z80oldCallFrameOptimization.cpp - Optimize z80old call sequences -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines a pass that optimizes call sequences on z80old.
//
//===----------------------------------------------------------------------===//

#include "Z80old.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

using namespace llvm;

#define DEBUG_TYPE "z80old-cf-opt"

static cl::opt<bool>
NoZ80oldCFOpt("no-z80old-call-frame-opt",
           cl::desc("Avoid optimizing z80old call frames"),
           cl::init(false), cl::Hidden);

namespace {
class Z80oldCallFrameOptimization : public MachineFunctionPass {
public:
  Z80oldCallFrameOptimization() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

private:
  static char ID;
};

char Z80oldCallFrameOptimization::ID = 0;
} // end anonymous namespace

FunctionPass *llvm::createZ80oldCallFrameOptimization() {
  return new Z80oldCallFrameOptimization();
}

bool Z80oldCallFrameOptimization::runOnMachineFunction(MachineFunction &MF) {
  if (skipFunction(MF.getFunction()) || NoZ80oldCFOpt.getValue()) {
    return false;
  }
  return false;
}
