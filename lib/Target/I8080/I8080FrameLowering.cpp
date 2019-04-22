//===-- I8080FrameLowering.cpp - I8080 Frame Information ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the z80 implementation of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#include "I8080FrameLowering.h"
#include "I8080.h"
#include "I8080InstrInfo.h"
#include "I8080MachineFunctionInfo.h"
#include "I8080Subtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
using namespace llvm;

I8080FrameLowering::I8080FrameLowering(const I8080Subtarget &STI)
  : TargetFrameLowering(StackGrowsDown, 1, -2),
    STI(STI), TII(*STI.getInstrInfo()), TRI(STI.getRegisterInfo()),
    SlotSize(2) {
}

/// hasFP - Return true if the specified function should have a dedicated frame
/// pointer register.  This is true if the function has variable sized allocas
/// or if frame pointer elimination is disabled.
bool I8080FrameLowering::hasFP(const MachineFunction &MF) const {
  return MF.getTarget().Options.DisableFramePointerElim(MF) ||
         MF.getFrameInfo().hasStackObjects();
}

void I8080FrameLowering::BuildStackAdjustment(MachineFunction &MF,
                                            MachineBasicBlock &MBB,
                                            MachineBasicBlock::iterator MI,
                                            DebugLoc DL, unsigned ScratchReg,
                                            int Offset, int FPOffset,
                                            bool UnknownOffset) const {
  if (!Offset) {
    return;
  }

  // Optimal if we are trying to set SP = FP
  //   LD SP, FP
  if (UnknownOffset || (FPOffset >= 0 && FPOffset == Offset)) {
    assert(hasFP(MF) && "This function doesn't have a frame pointer");
    BuildMI(MBB, MI, DL, TII.get(I8080::LD16SP))
    .addReg(TRI->getFrameRegister(MF));
    return;
  }

  bool OptSize = MF.getFunction().getAttributes()
                 .hasAttribute(AttributeList::FunctionIndex, Attribute::OptimizeForSize);

  bool Is24Bit = false;

  // Optimal for small offsets
  //   POP/PUSH HL for every SlotSize bytes
  unsigned SmallCost = OptSize ? 1 : Is24Bit ? 4 : Offset >= 0 ? 10 : 11;
  uint32_t PopPushCount = std::abs(Offset) / SlotSize;
  SmallCost *= PopPushCount;
  //   INC/DEC SP for remaining bytes
  uint32_t IncDecCount = std::abs(Offset) % SlotSize;
  SmallCost += (OptSize || Is24Bit ? 1 : 6) * IncDecCount;

  // Optimal for large offsets
  //   LD HL, Offset
  unsigned LargeCost = OptSize || Is24Bit ? 1 + SlotSize : 10;
  //   ADD HL, SP
  LargeCost += OptSize || Is24Bit ? 1 : 11;
  //   LD SP, HL
  LargeCost += OptSize || Is24Bit ? 1 : 6;

  // Optimal for medium offsets
  //   LEA HL, FP - Offset - FPOffset
  //   LD SP, HL
  bool CanUseLEA = false; // STI.hasEI8080Ops() && FPOffset >= 0 &&
  // isInt<8>(Offset - FPOffset) && hasFP(MF);
  unsigned LEACost = CanUseLEA ? 4 : LargeCost;

  // Prefer smaller version
  if (SmallCost <= LargeCost && SmallCost <= LEACost) {
    while (PopPushCount--)
      BuildMI(MBB, MI, DL, TII.get(Offset >= 0 ? I8080::POP16r : I8080::PUSH16r))
      .addReg(ScratchReg, getDefRegState(Offset >= 0) |
              getDeadRegState(Offset >= 0) | getUndefRegState(Offset < 0));
    while (IncDecCount--) {
      BuildMI(MBB, MI, DL, TII.get(Offset >= 0 ? I8080::INC16SP : I8080::DEC16SP));
    }
    return;
  }

  if (LargeCost <= LEACost) {
    BuildMI(MBB, MI, DL, TII.get(I8080::LD16ri),
            ScratchReg).addImm(Offset);
    BuildMI(MBB, MI, DL, TII.get(I8080::ADD16SP),
            ScratchReg).addReg(ScratchReg);
  } else {
    assert(CanUseLEA && hasFP(MF) && "Can't use lea");
    //BuildMI(MBB, MI, DL, TII.get(I8080::LEA16ro),
    //        ScratchReg).addReg(TRI->getFrameRegister(MF))
    //  .addImm(Offset - FPOffset);
  }
  BuildMI(MBB, MI, DL, TII.get(I8080::LD16SP))
  .addReg(ScratchReg, RegState::Kill);
}

/// emitPrologue - Push callee-saved registers onto the stack, which
/// automatically adjust the stack pointer. Adjust the stack pointer to allocate
/// space for local variables.
void I8080FrameLowering::emitPrologue(MachineFunction &MF,
                                    MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator MI = MBB.begin();

  // Debug location must be unknown since the first debug location is used
  // to determine the end of the prologue.
  DebugLoc DL;

  MachineFrameInfo &MFI = MF.getFrameInfo();
  int StackSize = -int(MFI.getStackSize());
  unsigned ScratchReg = I8080::HL;

  // skip callee-saved saves
  while (MI != MBB.end() && MI->getFlag(MachineInstr::FrameSetup)) {
    ++MI;
  }

  int FPOffset = -1;
  if (hasFP(MF)) {
    if (MF.getFunction().getAttributes().hasAttribute(
          AttributeList::FunctionIndex, Attribute::OptimizeForSize)) {
      if (StackSize) {
        BuildMI(MBB, MI, DL, TII.get(I8080::LD16ri),
                ScratchReg).addImm(StackSize);
        BuildMI(MBB, MI, DL, TII.get(I8080::CALL16i))
        .addExternalSymbol("_frameset").addReg(ScratchReg,
                                               RegState::ImplicitKill);
        return;
      }
      BuildMI(MBB, MI, DL, TII.get(I8080::CALL16i))
      .addExternalSymbol("_frameset0");
      return;
    }
    unsigned FrameReg = TRI->getFrameRegister(MF);
    BuildMI(MBB, MI, DL, TII.get(I8080::PUSH16r))
    .addReg(FrameReg);
    BuildMI(MBB, MI, DL, TII.get(I8080::LD16ri),
            FrameReg)
    .addImm(0);
    BuildMI(MBB, MI, DL, TII.get(I8080::ADD16SP),
            FrameReg).addReg(FrameReg);
    FPOffset = 0;
  }
  BuildStackAdjustment(MF, MBB, MI, DL, ScratchReg, StackSize, FPOffset);
}

void I8080FrameLowering::emitEpilogue(MachineFunction &MF,
                                    MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator MI = MBB.getFirstTerminator();
  DebugLoc DL = MBB.findDebugLoc(MI);

  MachineFrameInfo &MFI = MF.getFrameInfo();
  int StackSize = int(MFI.getStackSize());

  const TargetRegisterClass *ScratchRC = &I8080::AIR16RegClass;
  TargetRegisterClass::iterator ScratchReg = ScratchRC->begin();
  for (; MI->readsRegister(TRI->getSubReg(*ScratchReg, I8080::sub_low), TRI);
       ++ScratchReg)
    assert(ScratchReg != ScratchRC->end() &&
           "Could not allocate a scratch register!");
  assert((hasFP(MF) || *ScratchReg != TRI->getFrameRegister(MF)) &&
         "Cannot allocate csr as scratch register!");

  // skip callee-saved restores
  while (MI != MBB.begin())
    if (!(--MI)->getFlag(MachineInstr::FrameDestroy)) {
      ++MI;
      break;
    }

  // consume stack adjustment
  while (MI != MBB.begin()) {
    MachineBasicBlock::iterator PI = std::prev(MI);
    unsigned Opc = PI->getOpcode();
    if (Opc == I8080::POP16r &&
        PI->getOperand(0).isDead()) {
      StackSize += SlotSize;
    } else if (Opc == I8080::LD16SP) {
      unsigned Reg = PI->getOperand(0).getReg();
      if (PI == MBB.begin()) {
        break;
      }
      MachineBasicBlock::iterator AI = std::prev(PI);
      Opc = AI->getOpcode();
      if (AI == MBB.begin() || Opc != I8080::ADD16SP ||
          AI->getOperand(0).getReg() != Reg ||
          AI->getOperand(1).getReg() != Reg) {
        break;
      }
      MachineBasicBlock::iterator LI = std::prev(AI);
      Opc = LI->getOpcode();
      if (Opc != I8080::LD16ri ||
          LI->getOperand(0).getReg() != Reg) {
        break;
      }
      StackSize += LI->getOperand(1).getImm();
      LI->removeFromParent();
      AI->removeFromParent();
    } else {
      break;
    }
    PI->removeFromParent();
  }

  bool HasFP = hasFP(MF);
  BuildStackAdjustment(MF, MBB, MI, DL, *ScratchReg, StackSize,
                       HasFP ? StackSize : -1, MFI.hasVarSizedObjects());
  if (HasFP)
    BuildMI(MBB, MI, DL, TII.get(I8080::POP16r),
            TRI->getFrameRegister(MF));
}

// Only non-nested non-nmi interrupts can use shadow registers.
static bool shouldUseShadow(const MachineFunction &MF) {
  const Function &F = MF.getFunction();
  return F.getFnAttribute("interrupt").getValueAsString() == "Generic";
}

void I8080FrameLowering::shadowCalleeSavedRegisters(
  MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, DebugLoc DL,
  MachineInstr::MIFlag Flag, const std::vector<CalleeSavedInfo> &CSI) const {
  assert(shouldUseShadow(*MBB.getParent()) &&
         "Can't use shadow registers in this function.");
  bool SaveAF = false, SaveG = false;
  for (unsigned i = 0, e = CSI.size(); i != e; ++i) {
    unsigned Reg = CSI[i].getReg();
    if (Reg == I8080::AF) {
      SaveAF = true;
    } else if (I8080::GR16RegClass.contains(Reg)) {
      SaveG = true;
    }
  }
  if (SaveAF)
    BuildMI(MBB, MI, DL, TII.get(I8080::EXAF))
    .setMIFlag(Flag);
  if (SaveG)
    BuildMI(MBB, MI, DL, TII.get(I8080::EXX))
    .setMIFlag(Flag);
}

bool I8080FrameLowering::assignCalleeSavedSpillSlots(
  MachineFunction &MF, const TargetRegisterInfo *TRI,
  std::vector<CalleeSavedInfo> &CSI) const {
  MF.getInfo<I8080MachineFunctionInfo>()
  ->setCalleeSavedFrameSize((CSI.size() + hasFP(MF)) * SlotSize);
  return true;
}

bool I8080FrameLowering::spillCalleeSavedRegisters(
  MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
  const std::vector<CalleeSavedInfo> &CSI,
  const TargetRegisterInfo *TRI) const {
  const MachineFunction &MF = *MBB.getParent();
  const MachineRegisterInfo &MRI = MF.getRegInfo();
  bool UseShadow = shouldUseShadow(MF);
  DebugLoc DL = MBB.findDebugLoc(MI);
  if (UseShadow) {
    shadowCalleeSavedRegisters(MBB, MI, DL, MachineInstr::FrameSetup, CSI);
  }
  for (unsigned i = CSI.size(); i != 0; --i) {
    unsigned Reg = CSI[i - 1].getReg();

    // Non-index registers can be spilled to shadow registers.
    if (UseShadow && !I8080::IR16RegClass.contains(Reg)) {
      continue;
    }

    bool isLiveIn = MRI.isLiveIn(Reg);
    if (!isLiveIn) {
      MBB.addLiveIn(Reg);
    }

    // Decide whether we can add a kill flag to the use.
    bool CanKill = !isLiveIn;
    // Check if any subregister is live-in
    if (CanKill) {
      for (MCRegAliasIterator AReg(Reg, TRI, false); AReg.isValid(); ++AReg) {
        if (MRI.isLiveIn(*AReg)) {
          CanKill = false;
          break;
        }
      }
    }

    // Do not set a kill flag on values that are also marked as live-in. This
    // happens with the @llvm-returnaddress intrinsic and with arguments
    // passed in callee saved registers.
    // Omitting the kill flags is conservatively correct even if the live-in
    // is not used after all.
    MachineInstrBuilder MIB;
    if (Reg == I8080::AF) {
      MIB = BuildMI(MBB, MI, DL, TII.get(I8080::PUSH16AF));
    } else
      MIB = BuildMI(MBB, MI, DL, TII.get(I8080::PUSH16r))
            .addReg(Reg, getKillRegState(CanKill));
    MIB.setMIFlag(MachineInstr::FrameSetup);
  }
  return true;
}
bool I8080FrameLowering::restoreCalleeSavedRegisters(
  MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
  std::vector<CalleeSavedInfo> &CSI,
  const TargetRegisterInfo *TRI) const {
  const MachineFunction &MF = *MBB.getParent();
  bool UseShadow = shouldUseShadow(MF);
  DebugLoc DL = MBB.findDebugLoc(MI);
  for (unsigned i = 0, e = CSI.size(); i != e; ++i) {
    unsigned Reg = CSI[i].getReg();

    // Non-index registers can be spilled to shadow registers.
    if (UseShadow && !I8080::IR16RegClass.contains(Reg)) {
      continue;
    }

    MachineInstrBuilder MIB;
    if (Reg == I8080::AF) {
      MIB = BuildMI(MBB, MI, DL, TII.get(I8080::POP16AF));
    } else
      MIB = BuildMI(MBB, MI, DL, TII.get(I8080::POP16r),
                    Reg);
    MIB.setMIFlag(MachineInstr::FrameDestroy);
  }
  if (UseShadow) {
    shadowCalleeSavedRegisters(MBB, MI, DL, MachineInstr::FrameDestroy, CSI);
  }
  return true;
}

void I8080FrameLowering::processFunctionBeforeFrameFinalized(
  MachineFunction &MF, RegScavenger *RS) const {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MFI.setMaxCallFrameSize(0); // call frames are not implemented atm
  if (MFI.estimateStackSize(MF) > 0x80) {
    RS->addScavengingFrameIndex(MFI.CreateStackObject(SlotSize, 1, false));
  }
}

MachineBasicBlock::iterator I8080FrameLowering::
eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator I) const {
  //if (!hasReservedCallFrame(MF)) {
  unsigned Amount = TII.getFrameSize(*I);
  unsigned ScratchReg = I->getOperand(I->getNumOperands() - 1).getReg();

  {
    bool isLive;
    isLive = MF.getRegInfo().isLiveIn(I8080::HL);
    isLive = MF.getRegInfo().isLiveIn(I8080::BC);
    isLive = MF.getRegInfo().isLiveIn(I8080::DE);
    isLive = MF.getRegInfo().isLiveIn(I8080::IX);
    isLive = MF.getRegInfo().isLiveIn(I8080::IY);
    isLive=false;
  }


  assert(I8080::AIR16RegClass.contains(ScratchReg) &&
         "Expected last operand to be the scratch reg.");
  if (I->getOpcode() == TII.getCallFrameDestroyOpcode()) {
    Amount -= TII.getFramePoppedByCallee(*I);
    assert(TargetRegisterInfo::isPhysicalRegister(ScratchReg) &&
           "Reg alloc should have already happened.");
    BuildStackAdjustment(MF, MBB, I, I->getDebugLoc(), ScratchReg, Amount);
  }
  //}

  return MBB.erase(I);
}
