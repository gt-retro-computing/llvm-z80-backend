//===-- X86InstrBuilder.h - Functions to aid building x86 insts -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file exposes functions that may be used with BuildMI from the
// MachineInstrBuilder.h file to handle X86'isms in a clean way.
//
// The BuildMem function may be used with the BuildMI function to add entire
// memory references in a single, typed, function call.  X86 memory references
// can be very complex expressions (described in the README), so wrapping them
// up behind an easier to use interface makes sense.  Descriptions of the
// functions are included below.
//
// For reference, the order of operands for memory references is:
// (Operand), Base, Scale, Index, Displacement.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_X86_X86INSTRBUILDER_H
#define LLVM_LIB_TARGET_X86_X86INSTRBUILDER_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/MC/MCInstrDesc.h"
#include <cassert>

namespace llvm {

static inline const MachineInstrBuilder &
addOffset(const MachineInstrBuilder &MIB, int Offset) {
  return MIB.addImm(1).addReg(0).addImm(Offset).addReg(0);
}

/// addFrameReference - This function is used to add a reference to the base of
/// an abstract object on the stack frame of the current function.  This
/// reference has base register as the FrameIndex offset until it is resolved.
/// This allows a constant offset to be specified as well...
///
static inline const MachineInstrBuilder &
addFrameReference(const MachineInstrBuilder &MIB, int FI, int Offset = 0) {
  MachineInstr *MI = MIB;
  MachineFunction &MF = *MI->getParent()->getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const MCInstrDesc &MCID = MI->getDesc();
  auto Flags = MachineMemOperand::MONone;
  if (MCID.mayLoad()) {
    Flags |= MachineMemOperand::MOLoad;
  }
  if (MCID.mayStore()) {
    Flags |= MachineMemOperand::MOStore;
  }
  MachineMemOperand *MMO = MF.getMachineMemOperand(
                             MachinePointerInfo::getFixedStack(MF, FI, Offset), Flags,
                             MFI.getObjectSize(FI), MFI.getObjectAlignment(FI));
  return addOffset(MIB.addFrameIndex(FI), Offset)
         .addMemOperand(MMO);
}

} // end namespace llvm

#endif // LLVM_LIB_TARGET_X86_X86INSTRBUILDER_H 