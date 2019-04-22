//===-- I8080TargetFrameLowering.h - Define frame lowering for I8080 -*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class implements z80-specific bits of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_I8080_I8080FRAMELOWERING_H
#define LLVM_LIB_TARGET_I8080_I8080FRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {
class I8080Subtarget;
class I8080InstrInfo;
class I8080RegisterInfo;

class I8080FrameLowering : public TargetFrameLowering {
  const I8080Subtarget &STI;
  const I8080InstrInfo &TII;
  const I8080RegisterInfo *TRI;

  bool Is24Bit;
  unsigned SlotSize;

public:
  explicit I8080FrameLowering(const I8080Subtarget &STI);

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
                            unsigned ScratchReg, int Offset,
                            int FPOffset = -1,
                            bool UnknownOffset = false) const;

  void shadowCalleeSavedRegisters(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, DebugLoc DL,
    MachineInstr::MIFlag Flag, const std::vector<CalleeSavedInfo> &CSI) const;
};
} // End llvm namespace

#endif
