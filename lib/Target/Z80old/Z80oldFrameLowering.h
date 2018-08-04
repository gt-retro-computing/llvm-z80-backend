//===-- Z80oldTargetFrameLowering.h - Define frame lowering for Z80old -*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class implements z80old-specific bits of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Z80old_Z80oldFRAMELOWERING_H
#define LLVM_LIB_TARGET_Z80old_Z80oldFRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {
class Z80oldSubtarget;
class Z80oldInstrInfo;
class Z80oldRegisterInfo;

class Z80oldFrameLowering : public TargetFrameLowering {
  const Z80oldSubtarget &STI;
  const Z80oldInstrInfo &TII;
  const Z80oldRegisterInfo *TRI;

  bool Is24Bit;
  unsigned SlotSize;

public:
  explicit Z80oldFrameLowering(const Z80oldSubtarget &STI);

  /// emitProlog/emitEpilog - These methods insert prolog and epilog code into
  /// the function.
  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  bool assignCalleeSavedSpillSlots(
    MachineFunction &MF, const TargetRegisterInfo *TRI,
    std::vector<CalleeSavedInfo> &CSI) const override;
  bool spillCalleeSavedRegisters(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MI,
                                 const std::vector<CalleeSavedInfo> &CSI,
                                 const TargetRegisterInfo *TRI) const override;
  bool restoreCalleeSavedRegisters(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator MI,
                                   std::vector<CalleeSavedInfo> &CSI,
                                   const TargetRegisterInfo *TRI) const override;

  void processFunctionBeforeFrameFinalized(
    MachineFunction &MF, RegScavenger *RS = nullptr) const override;

  MachineBasicBlock::iterator eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MI) const override;

  bool hasFP(const MachineFunction &MF) const override;

private:
  void BuildStackAdjustment(MachineFunction &MF, MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MBBI, DebugLoc DL,
                            /*unsigned ScratchReg,*/ int Offset,
                            int FPOffset = -1,
                            bool UnknownOffset = false) const;

  void shadowCalleeSavedRegisters(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, DebugLoc DL,
    MachineInstr::MIFlag Flag, const std::vector<CalleeSavedInfo> &CSI) const;
};
} // End llvm namespace

#endif
