//===-- Z80oldRegisterInfo.cpp - Z80old Register Information --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Z80old implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#include "Z80oldRegisterInfo.h"
#include "Z80oldFrameLowering.h"
#include "Z80oldSubtarget.h"
#include "MCTargetDesc/Z80oldMCTargetDesc.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
using namespace llvm;

#define DEBUG_TYPE "z80oldreginfo"

#define GET_REGINFO_TARGET_DESC
#include "Z80oldGenRegisterInfo.inc"

Z80oldRegisterInfo::Z80oldRegisterInfo(const Triple &TT)
  : Z80oldGenRegisterInfo(Z80old::PC) {
  // Cache some information
}

const TargetRegisterClass *
Z80oldRegisterInfo::getPointerRegClass(const MachineFunction &MF,
                                    unsigned Kind) const {
  //const Z80oldSubtarget& Subtarget = MF.getSubtarget<Z80oldSubtarget>();
  switch (Kind) {
  default: llvm_unreachable("Unexpected Kind in getPointerRegClass!");
  case 0: return &Z80old::GR16RegClass;
  case 1: return &Z80old::AIR16RegClass;
  case 2: return &Z80old::IR16RegClass;
  }
}

const TargetRegisterClass *
Z80oldRegisterInfo::getLargestLegalSuperClass(const TargetRegisterClass *RC,
                                           const MachineFunction &) const {
  const TargetRegisterClass *Super = RC;
  TargetRegisterClass::sc_iterator I = RC->getSuperClasses();
  do {
    switch (Super->getID()) {
    case Z80old::RR8RegClassID:
    case Z80old::R16RegClassID:
      //case Z80old::R24RegClassID:
      return Super;
    }
    Super = *I++;
  } while (Super);
  return RC;
}

unsigned Z80oldRegisterInfo::getRegPressureLimit(const TargetRegisterClass *RC,
                                              MachineFunction &MF) const {
  return 3;
  const Z80oldFrameLowering *TFI = getFrameLowering(MF);

  switch (RC->getID()) {
  default:
    return 0;
  case Z80old::R16RegClassID:
    //case Z80old::R24RegClassID:
    return 2;
  }
}

const MCPhysReg *
Z80oldRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  switch (MF->getFunction().getCallingConv()) {
  default: llvm_unreachable("Unsupported calling convention");
  case CallingConv::C:
  case CallingConv::Fast:
    return /*Is24Bit ? CSR_EZ80old_C_SaveList :*/ CSR_Z80old_C_SaveList;
  case CallingConv::PreserveAll:
  case CallingConv::Z80_LibCall:
  case CallingConv::Z80_LibCall_AC:
  case CallingConv::Z80_LibCall_BC:
  case CallingConv::Z80_LibCall_C:
  case CallingConv::Z80_LibCall_L:
    return /*Is24Bit ? CSR_EZ80old_AllRegs_SaveList :*/ CSR_Z80old_AllRegs_SaveList;
  }
}

const uint32_t *
Z80oldRegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                      CallingConv::ID CC) const {
  switch (CC) {
  default: llvm_unreachable("Unsupported calling convention");
  case CallingConv::C:
  case CallingConv::Fast:
    return /*Is24Bit ? CSR_EZ80old_C_RegMask :*/ CSR_Z80old_C_RegMask;
  case CallingConv::PreserveAll:
  case CallingConv::Z80_LibCall:
  case CallingConv::Z80_LibCall_AC:
  case CallingConv::Z80_LibCall_BC:
  case CallingConv::Z80_LibCall_C:
  case CallingConv::Z80_LibCall_L:
    return /*Is24Bit ? CSR_EZ80old_AllRegs_RegMask :*/ CSR_Z80old_AllRegs_RegMask;
  }
}
const uint32_t *Z80oldRegisterInfo::getNoPreservedMask() const {
  return CSR_NoRegs_RegMask;
}

BitVector Z80oldRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  const Z80oldFrameLowering *TFI = getFrameLowering(MF);

  // Set the stack-pointer registers as reserved.
  Reserved.set(Z80old::SPS);

  // Set the program-counter register as reserved.
  Reserved.set(Z80old::PC);

  // Set the frame-pointer register and its aliases as reserved if needed.
  if (TFI->hasFP(MF)) {
    for (MCSubRegIterator I(Z80old::IX, this, /*IncludesSelf=*/true); I.isValid();
         ++I) {
      Reserved.set(*I);
    }
  }

  return Reserved;
}

bool Z80oldRegisterInfo::saveScavengerRegister(MachineBasicBlock &MBB,
                                            MachineBasicBlock::iterator MI,
                                            MachineBasicBlock::iterator &UseMI,
                                            const TargetRegisterClass *RC,
                                            unsigned Reg) const {
  return false;
  const Z80oldSubtarget &STI = MBB.getParent()->getSubtarget<Z80oldSubtarget>();
  const TargetInstrInfo &TII = *STI.getInstrInfo();
  const TargetRegisterInfo *TRI = STI.getRegisterInfo();
  DebugLoc DL;
  if (Reg == Z80old::AF) {
    BuildMI(MBB, MI, DL, TII.get(Z80old::PUSH16AF));
  } else
    BuildMI(MBB, MI, DL, TII.get(Z80old::PUSH16r))
    .addReg(Reg);
  for (MachineBasicBlock::iterator II = MI; II != UseMI ; ++II) {
    if (II->isDebugValue()) {
      continue;
    }
    if (II->modifiesRegister(Reg, TRI)) {
      UseMI = II;
    }
  }
  if (Reg == Z80old::AF) {
    BuildMI(MBB, UseMI, DL, TII.get(Z80old::POP16AF));
  } else {
    BuildMI(MBB, UseMI, DL, TII.get(Z80old::POP16r), Reg);
  }
  return true;
}

void Z80oldRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                          int SPAdj, unsigned FIOperandNum,
                                          RegScavenger *RS) const {
  MachineInstr &MI = *II;
  unsigned Opc = MI.getOpcode();
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const Z80oldSubtarget &STI = MF.getSubtarget<Z80oldSubtarget>();
  const Z80oldInstrInfo &TII = *STI.getInstrInfo();
  const Z80oldFrameLowering *TFI = getFrameLowering(MF);
  DebugLoc DL = MI.getDebugLoc();
  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();

  unsigned BasePtr = getFrameRegister(MF);
  LLVM_DEBUG(MF.dump(); II->dump();
             dbgs() << MF.getFunction().arg_size() << '\n');
  assert(TFI->hasFP(MF) && "Stack slot use without fp unimplemented");
  int Offset = MF.getFrameInfo().getObjectOffset(FrameIndex);
  int SlotSize = /*Is24Bit ? 3 :*/ 2;
  // Skip any saved callee saved registers
  if (TFI->hasFP(MF)) {
    Offset += SlotSize;
  }
  // Skip return address for arguments
  if (FrameIndex < 0) {
    Offset += SlotSize;
  }
  Offset += MI.getOperand(FIOperandNum + 1).getImm();
  if (isInt<8>(Offset) && Opc != Z80old::LD16rfi) {
    MI.getOperand(FIOperandNum).ChangeToRegister(BasePtr, false);
    MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset);
    return;
  }
  unsigned OffsetReg = RS->scavengeRegister(
                         &Z80old::OR16RegClass, II, SPAdj);
#if 0
  if ((Opc == Z80old::LEA24ro &&
       Z80old::A24RegClass.contains(MI.getOperand(0).getReg())) ||
      ((Opc == Z80old::LEA16ro || Opc == Z80old::LD16rfi) &&
       Z80old::AIR16RegClass.contains(MI.getOperand(0).getReg()))) {
    BuildMI(MBB, II, DL, TII.get(Is24Bit ? Z80old::LD24ri : Z80old::LD16ri),
            OffsetReg).addImm(Offset);
    MI.getOperand(FIOperandNum).ChangeToRegister(BasePtr, false);
    if (Opc == Z80old::LD16rfi) {
      MI.setDesc(TII.get(TargetOpcode::COPY));
      MI.RemoveOperand(FIOperandNum + 1);
    } else {
      MI.getOperand(FIOperandNum + 1).ChangeToImmediate(0);
    }
    BuildMI(MBB, ++II, DL, TII.get(Is24Bit ? Z80old::ADD24ao : Z80old::ADD16ao),
            MI.getOperand(0).getReg()).addReg(MI.getOperand(0).getReg())
    .addReg(OffsetReg, RegState::Kill);
    return;
  }
#endif // 0
  if (unsigned ScratchReg = RS->FindUnusedReg(&Z80old::AIR16RegClass)) {
    BuildMI(MBB, II, DL, TII.get(Z80old::LD16ri),
            OffsetReg).addImm(Offset);
    BuildMI(MBB, II, DL, TII.get(TargetOpcode::COPY), ScratchReg)
    .addReg(BasePtr);
    BuildMI(MBB, II, DL, TII.get(Z80old::ADD16ao),
            ScratchReg).addReg(ScratchReg).addReg(OffsetReg, RegState::Kill);
    MI.getOperand(FIOperandNum).ChangeToRegister(ScratchReg, false);
    if ((Z80old::IR16RegClass).contains(ScratchReg)) {
      MI.getOperand(FIOperandNum + 1).ChangeToImmediate(0);
    } else {
      switch (Opc) {
      default: llvm_unreachable("Unexpected opcode!");
      //case Z80old::LD24ro: Opc = Z80old::LD24rp; break;
      //case Z80old::LD16ro: Opc = Z80old::LD16rp; break;
      case Z80old::LD8ro: Opc = Z80old::LD8rp; break;
      case Z80old::LD8go: Opc = Z80old::LD8gp; break;
      //case Z80old::LD24or: Opc = Z80old::LD24pr; break;
      //case Z80old::LD16or: Opc = Z80old::LD16pr; break;
      case Z80old::LD8or: Opc = Z80old::LD8pr; break;
      case Z80old::LD8og: Opc = Z80old::LD8pg; break;
      //case Z80old::LEA24ro: case Z80old::LEA16ro:
      case Z80old::LD16rfi: Opc = TargetOpcode::COPY; break;
        //case Z80old::PEA24o: Opc = Z80old::PUSH24r; break;
        //case Z80old::PEA16o: Opc = Z80old::PUSH16r; break;
      }
      MI.setDesc(TII.get(Opc));
      MI.RemoveOperand(FIOperandNum + 1);
    }
    return;
  }
  BuildMI(MBB, II, DL, TII.get(Z80old::PUSH16r))
  .addReg(BasePtr);
  BuildMI(MBB, II, DL, TII.get(Z80old::LD16ri), OffsetReg)
  .addImm(Offset);
  BuildMI(MBB, II, DL, TII.get(Z80old::ADD16ao), BasePtr)
  .addReg(BasePtr).addReg(OffsetReg, RegState::Kill);
#if 0
  if (Opc == Z80old::PEA24o || Opc == Z80old::PEA16o) {
    MI.setDesc(TII.get(Opc == Z80old::PEA24o ? Z80old::EX24SP : Z80old::EX16SP));
    MI.getOperand(0).ChangeToRegister(BasePtr, true);
    MI.getOperand(1).ChangeToRegister(BasePtr, false);
    MI.tieOperands(0, 1);
  } else
#endif // 0
  {
    MI.getOperand(FIOperandNum).ChangeToRegister(BasePtr, false);
    if (Opc == Z80old::LD16rfi) {
      MI.setDesc(TII.get(TargetOpcode::COPY));
      MI.RemoveOperand(FIOperandNum + 1);
    } else {
      MI.getOperand(FIOperandNum + 1).ChangeToImmediate(0);
    }
    BuildMI(MBB, ++II, DL, TII.get(Z80old::POP16r), BasePtr);
  }
}

unsigned Z80oldRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return getFrameLowering(MF)->hasFP(MF) ? Z80old::IX : Z80old::SPS;
}

bool Z80oldRegisterInfo::
shouldCoalesce(MachineInstr *MI,
               const TargetRegisterClass *SrcRC, unsigned SrcSubReg,
               const TargetRegisterClass *DstRC, unsigned DstSubReg,
               const TargetRegisterClass *NewRC, LiveIntervals &LIS) const {
  const TargetRegisterInfo &TRI = *MI->getParent()->getParent()->getRegInfo()
                                  .getTargetRegisterInfo();
  (void)TRI;
  LLVM_DEBUG(dbgs() << TRI.getRegClassName(SrcRC) << '[' << SrcRC->getNumRegs()
             << "]:" << (SrcSubReg ? TRI.getSubRegIndexName(SrcSubReg) : "")
             << " -> " << TRI.getRegClassName(DstRC) << '[' << DstRC->getNumRegs()
             << "]:" << (DstSubReg ? TRI.getSubRegIndexName(DstSubReg) : "") << ' '
             << TRI.getRegClassName(NewRC) << '[' << NewRC->getNumRegs() << "]\n");
  // Don't coalesce if SrcRC and DstRC have a small intersection.
  return std::min(SrcRC->getNumRegs(),
                  DstRC->getNumRegs()) <= NewRC->getNumRegs();
}

bool Z80oldRegisterInfo::
requiresVirtualBaseRegisters(const MachineFunction &MF) const {
  return true;
}
bool Z80oldRegisterInfo::needsFrameBaseReg(MachineInstr *MI,
                                        int64_t Offset) const {
  const MachineFunction &MF = *MI->getParent()->getParent();
  return !isFrameOffsetLegal(MI, getFrameRegister(MF), Offset);
}
void Z80oldRegisterInfo::
materializeFrameBaseRegister(MachineBasicBlock *MBB, unsigned BaseReg,
                             int FrameIdx, int64_t Offset) const {
  MachineFunction &MF = *MBB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  MachineBasicBlock::iterator II = MBB->begin();
  DebugLoc DL = MBB->findDebugLoc(II);
  MRI.setRegClass(BaseReg, &Z80old::IR16RegClass);
  //BuildMI(*MBB, II, DL, TII.get(Z80old::LEA16ro), BaseReg)
  BuildMI(*MBB, II, DL, TII.get(Z80old::LD88ro),
          BaseReg) // $TODO: Check if this is correct. LEA: IX/Y += Offset
  .addFrameIndex(FrameIdx).addImm(Offset);
  return;
  unsigned CopyReg = MRI.createVirtualRegister(&Z80old::IR16RegClass);
  unsigned OffsetReg = MRI.createVirtualRegister(&Z80old::OR16RegClass);
  BuildMI(*MBB, II, DL, TII.get(TargetOpcode::COPY), CopyReg)
  .addReg(getFrameRegister(MF));
  BuildMI(*MBB, II, DL, TII.get(Z80old::LD16ri),
          OffsetReg).addImm(Offset);
  BuildMI(*MBB, II, DL, TII.get(Z80old::ADD16ao), BaseReg)
  .addReg(CopyReg).addReg(OffsetReg);
}
void Z80oldRegisterInfo::resolveFrameIndex(MachineInstr &MI, unsigned BaseReg,
                                        int64_t Offset) const {
  unsigned FIOperandNum = 0;
  while (!MI.getOperand(FIOperandNum).isFI()) {
    FIOperandNum++;
    assert(FIOperandNum < MI.getNumOperands() && "Expected a frame index");
  }
  MI.getOperand(FIOperandNum).ChangeToRegister(BaseReg, false);
  MI.getOperand(FIOperandNum + 1).ChangeToImmediate(
    MI.getOperand(FIOperandNum + 1).getImm() + Offset);
}
bool Z80oldRegisterInfo::isFrameOffsetLegal(const MachineInstr *MI,
                                         unsigned BaseReg,
                                         int64_t Offset) const {
  return isInt<8>(Offset);
}
