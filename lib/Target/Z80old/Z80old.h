//===-- Z80old.h - Top-level interface for Z80old representation ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in
// the LLVM Z80old back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Z80old_Z80old_H
#define LLVM_LIB_TARGET_Z80old_Z80old_H

//#include "Z80oldConfig.h"
#include "MCTargetDesc/Z80oldMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class Z80oldTargetMachine;
class FunctionPass;

/// This pass converts a legalized DAG into a Z80old-specific DAG, ready for
/// instruction scheduling.
FunctionPass *createZ80oldISelDag(Z80oldTargetMachine &TM,
                               CodeGenOpt::Level OptLevel);

/// Return a pass that optimizes z80old call sequences.
FunctionPass *createZ80oldCallFrameOptimization();

/// Return a Machine IR pass that expands Z80old-specific pseudo
/// instructions into a sequence of actual instructions. This pass
/// must run after prologue/epilogue insertion and before lowering
/// the MachineInstr to MC.
FunctionPass *createZ80oldExpandPseudoPass();

/// Return a pass that optimizes instructions after register selection.
FunctionPass *createZ80oldMachineLateOptimization();
} // end namespace llvm;

#endif

