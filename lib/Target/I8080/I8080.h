//===-- I8080.h - Top-level interface for I8080 representation ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in
// the LLVM I8080 back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_I8080_I8080_H
#define LLVM_LIB_TARGET_I8080_I8080_H

//#include "I8080Config.h"
#include "MCTargetDesc/I8080MCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class I8080TargetMachine;
class FunctionPass;

/// This pass converts a legalized DAG into a I8080-specific DAG, ready for
/// instruction scheduling.
FunctionPass *createI8080ISelDag(I8080TargetMachine &TM,
                               CodeGenOpt::Level OptLevel);

/// Return a pass that optimizes i8080 call sequences.
FunctionPass *createI8080CallFrameOptimization();

/// Return a Machine IR pass that expands I8080-specific pseudo
/// instructions into a sequence of actual instructions. This pass
/// must run after prologue/epilogue insertion and before lowering
/// the MachineInstr to MC.
FunctionPass *createI8080ExpandPseudoPass();

/// Return a pass that optimizes instructions after register selection.
FunctionPass *createI8080MachineLateOptimization();
} // end namespace llvm;

#endif

