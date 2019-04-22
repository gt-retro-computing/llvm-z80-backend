//===-- I8080RegisterInfo.cpp - I8080 Register Information --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the I8080 implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#include "I8080RegisterInfo.h"
#include "I8080FrameLowering.h"
#include "I8080Subtarget.h"
#include "MCTargetDesc/I8080MCTargetDesc.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
using namespace llvm;

#define DEBUG_TYPE "z80reginfo"

#define GET_REGINFO_TARGET_DESC
#include "I8080GenRegisterInfo.inc"

I8080RegisterInfo::I8080RegisterInfo(const Triple &TT)
  : I8080GenRegisterInfo(I8080::PC) {
  // Cache some information
}

const TargetRegisterClass *
I8080RegisterInfo::getPointerRegClass(const MachineFunction &MF,
                                    unsigned Kind) const {
  //const I8080Subtarget& Subtarget = MF.getSubtarget<I8080Subtarget>();
  switch (Kind) {
  default: llvm_unreachable("Unexpected Kind in getPointerRegClass!");
  case 0: return &I8080::GR16RegClass;
  case 1: return &I8080::AIR16RegClass;
  case 2: return &I8080::IR16RegClass;
  }
}

const TargetRegisterClass *
I8080RegisterInfo::getLargestLegalSuperClass(const TargetRegisterClass *RC,
                                           const MachineFunction &) const {
  const TargetRegisterClass *Super = RC;
  TargetRegisterClass::sc_iterator I = RC->getSuperClasses();
  do {
    switch (Super->getID()) {
    case I8080::RR8RegClassID:
    case I8080::R16RegClassID:
      //case I8080::R24RegClassID:
      return Super;
    }
    Super = *I++;
  } while (Super);
  return RC;
}

unsigned I8080RegisterInfo::getRegPressureLimit(const TargetRegisterClass *RC,
                                              MachineFunction &MF) const {
  return 3;
  const I8080FrameLowering *TFI = getFrameLowering(MF);

  switch (RC->getID()) {
  default:
    return 0;
  case I8080::R16RegClassID:
    //case I8080::R24RegClassID:
    return 2;
  }
}

const MCPhysReg *
I8080RegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  switch (MF->getFunction().getCallingConv()) {
  default: llvm_unreachable("Unsupported calling convention");
  case CallingConv::C:
  case CallingConv::Fast:
    return /*Is24Bit ? CSR_EI8080_C_SaveList :*/ CSR_I8080_C_SaveList;
  case CallingConv::PreserveAll:
  case CallingConv::I8080_LibCall:
  case CallingConv::I8080_LibCall_AC:
  case CallingConv::I8080_LibCall_BC:
  case CallingConv::I8080_LibCall_C:
  case CallingConv::I8080_LibCall_L:
    return /*Is24Bit ? CSR_EI8080_AllRegs_SaveList :*/ CSR_I8080_AllRegs_SaveList;
  }
}

const uint32_t *
I8080RegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                      CallingConv::ID CC) const {
  switch (CC) {
  default: llvm_unreachable("Unsupported calling convention");
  case CallingConv::C:
  case CallingConv::Fast:
    return /*Is24Bit ? CSR_EI8080_C_RegMask :*/ CSR_I8080_C_RegMask;
  case CallingConv::PreserveAll:
  case CallingConv::I8080_LibCall:
  case CallingConv::I8080_LibCall_AC:
  case CallingConv::I8080_LibCall_BC:
  case CallingConv::I8080_LibCall_C:
  case CallingConv::I8080_LibCall_L:
    return /*Is24Bit ? CSR_EI8080_AllRegs_RegMask :*/ CSR_I8080_AllRegs_RegMask;
  }
}
const uint32_t *I8080RegisterInfo::getNoPreservedMask() const {
  return CSR_NoRegs_RegMask;
}

BitVector I8080RegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  const I8080FrameLowering *TFI = getFrameLowering(MF);

  // Set the stack-pointer registers as reserved.
  Reserved.set(I8080::SPS);

  // Set the program-counter register as reserved.
  Reserved.set(I8080::PC);

  // Set the frame-pointer register and its aliases as reserved if needed.
  if (TFI->hasFP(MF)) {
    for (MCSubRegIterator I(I8080::IX, this, /*IncludesSelf=*/true); I.isValid();
         ++I) {
      Reserved.set(*I);
    }
  }

  return Reserved;
}

bool I8080RegisterInfo::saveScavengerRegister(MachineBasicBlock &MBB,
                                            MachineBasicBlock::iterator MI,
                                            MachineBasicBlock::iterator &UseMI,
                                            const TargetRegisterClass *RC,
                                            unsigned Reg) const {
  return false;
  const I8080Subtarget &STI = MBB.getParent()->getSubtarget<I8080Subtarget>();
  const TargetInstrInfo &TII = *STI.getInstrInfo();
  const TargetRegisterInfo *TRI = STI.getRegisterInfo();
  DebugLoc DL;
  if (Reg == I8080::AF) {
    BuildMI(MBB, MI, DL, TII.get(I8080::PUSH16AF));
  } else
    BuildMI(MBB, MI, DL, TII.get(I8080::PUSH16r))
    .addReg(Reg);
  for (MachineBasicBlock::iterator II = MI; II != UseMI ; ++II) {
    if (II->isDebugValue()) {
      continue;
    }
    if (II->modifiesRegister(Reg, TRI)) {
      UseMI = II;
    }
  }
  if (Reg == I8080::AF) {
    BuildMI(MBB, UseMI, DL, TII.get(I8080::POP16AF));
  } else {
    BuildMI(MBB, UseMI, DL, TII.get(I8080::POP16r), Reg);
  }
  return true;
}

void I8080RegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                          int SPAdj, unsigned FIOperandNum,
                                          RegScavenger *RS) const {
  MachineInstr &MI = *II;
  unsigned Opc = MI.getOpcode();
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const I8080Subtarget &STI = MF.getSubtarget<I8080Subtarget>();
  const I8080InstrInfo &TII = *STI.getInstrInfo();
  const I8080FrameLowering *TFI = getFrameLowering(MF);
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
  if (isInt<8>(Offset) && Opc != I8080::LD16rfi) {
    MI.getOperand(FIOperandNum).ChangeToRegister(BasePtr, false);
    MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset);
    return;
  }
  unsigned OffsetReg = RS->scavengeRegister(
                         &I8080::OR16RegClass, II, SPAdj);
#if 0
  if ((Opc == I8080::LEA24ro &&
       I8080::A24RegClass.contains(MI.getOperand(0).getReg())) ||
      ((Opc == I8080::LEA16ro || Opc == I8080::LD16rfi) &&
       I8080::AIR16RegClass.contains(MI.getOperand(0).getReg()))) {
    BuildMI(MBB, II, DL, TII.get(Is24Bit ? I8080::LD24ri : I8080::LD16ri),
            OffsetReg).addImm(Offset);
    MI.getOperand(FIOperandNum).ChangeToRegister(BasePtr, false);
    if (Opc == I8080::LD16rfi) {
      MI.setDesc(TII.get(TargetOpcode::COPY));
      MI.RemoveOperand(FIOperandNum + 1);
    } else {
      MI.getOperand(FIOperandNum + 1).ChangeToImmediate(0);
    }
    BuildMI(MBB, ++II, DL, TII.get(Is24Bit ? I8080::ADD24ao : I8080::ADD16ao),
            MI.getOperand(0).getReg()).addReg(MI.getOperand(0).getReg())
    .addReg(OffsetReg, RegState::Kill);
    return;
  }
#endif // 0
  if (unsigned ScratchReg = RS->FindUnusedReg(&I8080::AIR16RegClass)) {
    BuildMI(MBB, II, DL, TII.get(I8080::LD16ri),
            OffsetReg).addImm(Offset);
    BuildMI(MBB, II, DL, TII.get(TargetOpcode::COPY), ScratchReg)
    .addReg(BasePtr);
    BuildMI(MBB, II, DL, TII.get(I8080::ADD16ao),
            ScratchReg).addReg(ScratchReg).addReg(OffsetReg, RegState::Kill);
    MI.getOperand(FIOperandNum).ChangeToRegister(ScratchReg, false);
    if ((I8080::IR16RegClass).contains(ScratchReg)) {
      MI.getOperand(FIOperandNum + 1).ChangeToImmediate(0);
    } else {
      switch (Opc) {
      default: llvm_unreachable("Unexpected opcode!");
      //case I8080::LD24ro: Opc = I8080::LD24rp; break;
      //case I8080::LD16ro: Opc = I8080::LD16rp; break;
      case I8080::LD8ro: Opc = I8080::LD8rp; break;
      case I8080::LD8go: Opc = I8080::LD8gp; break;
      //case I8080::LD24or: Opc = I8080::LD24pr; break;
      //case I8080::LD16or: Opc = I8080::LD16pr; break;
      case I8080::LD8or: Opc = I8080::LD8pr; break;
      case I8080::LD8og: Opc = I8080::LD8pg; break;
      //case I8080::LEA24ro: case I8080::LEA16ro:
      case I8080::LD16rfi: Opc = TargetOpcode::COPY; break;
        //case I8080::PEA24o: Opc = I8080::PUSH24r; break;
        //case I8080::PEA16o: Opc = I8080::PUSH16r; break;
      }
      MI.setDesc(TII.get(Opc));
      MI.RemoveOperand(FIOperandNum + 1);
    }
    return;
  }
  BuildMI(MBB, II, DL, TII.get(I8080::PUSH16r))
  .addReg(BasePtr);
  BuildMI(MBB, II, DL, TII.get(I8080::LD16ri), OffsetReg)
  .addImm(Offset);
  BuildMI(MBB, II, DL, TII.get(I8080::ADD16ao), BasePtr)
  .addReg(BasePtr).addReg(OffsetReg, RegState::Kill);
#if 0
  if (Opc == I8080::PEA24o || Opc == I8080::PEA16o) {
    MI.setDesc(TII.get(Opc == I8080::PEA24o ? I8080::EX24SP : I8080::EX16SP));
    MI.getOperand(0).ChangeToRegister(BasePtr, true);
    MI.getOperand(1).ChangeToRegister(BasePtr, false);
    MI.tieOperands(0, 1);
  } else
#endif // 0
  {
    MI.getOperand(FIOperandNum).ChangeToRegister(BasePtr, false);
    if (Opc == I8080::LD16rfi) {
      MI.setDesc(TII.get(TargetOpcode::COPY));
      MI.RemoveOperand(FIOperandNum + 1);
    } else {
      MI.getOperand(FIOperandNum + 1).ChangeToImmediate(0);
    }
    BuildMI(MBB, ++II, DL, TII.get(I8080::POP16r), BasePtr);
  }
}

unsigned I8080RegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return getFrameLowering(MF)->hasFP(MF) ? I8080::IX : I8080::SPS;
}

bool I8080RegisterInfo::
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

bool I8080RegisterInfo::
requiresVirtualBaseRegisters(const MachineFunction &MF) const {
  return true;
}
bool I8080RegisterInfo::needsFrameBaseReg(MachineInstr *MI,
                                        int64_t Offset) const {
  const MachineFunction &MF = *MI->getParent()->getParent();
  return !isFrameOffsetLegal(MI, getFrameRegister(MF), Offset);
}
void I8080RegisterInfo::
materializeFrameBaseRegister(MachineBasicBlock *MBB, unsigned BaseReg,
                             int FrameIdx, int64_t Offset) const {
  MachineFunction &MF = *MBB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  MachineBasicBlock::iterator II = MBB->begin();
  DebugLoc DL = MBB->findDebugLoc(II);
  MRI.setRegClass(BaseReg, &I8080::IR16RegClass);
  //BuildMI(*MBB, II, DL, TII.get(I8080::LEA16ro), BaseReg)
  BuildMI(*MBB, II, DL, TII.get(I8080::LD88ro),
          BaseReg) // $TODO: Check if this is correct. LEA: IX/Y += Offset
  .addFrameIndex(FrameIdx).addImm(Offset);
  return;
  unsigned CopyReg = MRI.createVirtualRegister(&I8080::IR16RegClass);
  unsigned OffsetReg = MRI.createVirtualRegister(&I8080::OR16RegClass);
  BuildMI(*MBB, II, DL, TII.get(TargetOpcode::COPY), CopyReg)
  .addReg(getFrameRegister(MF));
  BuildMI(*MBB, II, DL, TII.get(I8080::LD16ri),
          OffsetReg).addImm(Offset);
  BuildMI(*MBB, II, DL, TII.get(I8080::ADD16ao), BaseReg)
  .addReg(CopyReg).addReg(OffsetReg);
}
void I8080RegisterInfo::resolveFrameIndex(MachineInstr &MI, unsigned BaseReg,
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
bool I8080RegisterInfo::isFrameOffsetLegal(const MachineInstr *MI,
                                         unsigned BaseReg,
                                         int64_t Offset) const {
  return isInt<8>(Offset);
}
