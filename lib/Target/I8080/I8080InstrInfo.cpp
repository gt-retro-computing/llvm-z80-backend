//===-- I8080InstrInfo.cpp - I8080 Instruction Information --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the I8080 implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "I8080InstrInfo.h"
#include "I8080.h"
#include "I8080Subtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
using namespace llvm;

#define DEBUG_TYPE "z80-instr-info"

#define GET_INSTRINFO_CTOR_DTOR
#include "I8080GenInstrInfo.inc"

// Pin the vtable to this file.
void I8080InstrInfo::anchor() {}

I8080InstrInfo::I8080InstrInfo(I8080Subtarget &STI)
  : I8080GenInstrInfo(I8080::ADJCALLSTACKDOWN16, I8080::ADJCALLSTACKUP16),
    Subtarget(STI), RI(STI.getTargetTriple()) {
}

int I8080InstrInfo::getSPAdjust(const MachineInstr &MI) const {
  switch (MI.getOpcode()) {
  case I8080::POP16r:
  case I8080::POP16AF:
    return 2;
  case I8080::PUSH16r:
  case I8080::PUSH16AF:
    return -2;
  }
  return TargetInstrInfo::getSPAdjust(MI);
}

static bool isIndex(const MachineOperand &MO, const MCRegisterInfo &RI) {
  if (MO.isFI()) {
    return true;
  }
  if (MO.isReg())
    for (unsigned IndexReg : I8080::IR16RegClass)
      if (RI.isSubRegisterEq(IndexReg, MO.getReg())) {
        return true;
      }
  return false;
}

static bool hasIndex(const MachineInstr &MI, const MCRegisterInfo &RI) {
  for (const MachineOperand &Op : MI.explicit_operands())
    if (isIndex(Op, RI)) {
      return true;
    }
  return false;
}

unsigned I8080InstrInfo::getInstSizeInBytes(const MachineInstr &MI) const {
  auto TSFlags = MI.getDesc().TSFlags;
  // 1 byte for opcode
  unsigned Size = 1;
  // 1 byte if we need a suffix
  // FIXME: create an operand for the suffix
  switch (TSFlags >> I8080II::ImmSizeShift & I8080II::ImmSizeMask) {
  case 2:
    Size += false /*Subtarget.is24Bit()*/;
    break;
  case 3:
    Size += true /*Subtarget.is16Bit()*/;
    break;
  }
  // prefix byte(s)
  unsigned Prefix = TSFlags >> I8080II::PrefixShift & I8080II::PrefixMask;
  bool HasPrefix;
  if (TSFlags & I8080II::IndexedIndexPrefix) {
    Size += HasPrefix = isIndex(MI.getOperand(Prefix), getRegisterInfo());
  } else
    switch (Prefix) {
    case I8080II::NoPrefix:
      HasPrefix = false;
      break;
    case I8080II::CBPrefix:
    case I8080II::DDPrefix:
    case I8080II::EDPrefix:
    case I8080II::FDPrefix:
      Size += 1;
      HasPrefix = true;
      break;
    case I8080II::DDCBPrefix:
    case I8080II::FDCBPrefix:
      Size += 2;
      HasPrefix = true;
      break;
    case I8080II::AnyIndexPrefix:
      Size += HasPrefix = hasIndex(MI, getRegisterInfo());
      break;
    }
  // immediate byte(s)
  if (TSFlags & I8080II::HasImm) {
    unsigned ImmSize = TSFlags >> I8080II::ImmSizeShift & I8080II::ImmSizeMask;
    if (!ImmSize) {
      ImmSize = 2;
    }
    Size += ImmSize;
  }
  // 1 byte if we need an offset, but only for prefixed instructions
  if (TSFlags & I8080II::HasOff) {
    Size += HasPrefix;
  }
  return Size;
}

/// Return the inverse of the specified condition,
/// e.g. turning COND_E to COND_NE.
I8080::CondCode I8080::GetOppositeBranchCondition(I8080::CondCode CC) {
  return I8080::CondCode(CC ^ 1);
}

bool I8080InstrInfo::isUnpredicatedTerminator(const MachineInstr &MI) const {
  if (!MI.isTerminator()) { return false; }

  // Conditional branch is a special case.
  if (MI.isBranch() && !MI.isBarrier()) {
    return true;
  }
  if (!MI.isPredicable()) {
    return true;
  }
  return !isPredicated(MI);
}

bool I8080InstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                 MachineBasicBlock *&TBB,
                                 MachineBasicBlock *&FBB,
                                 SmallVectorImpl<MachineOperand> &Cond,
                                 bool AllowModify) const {
  // Start from the bottom of the block and work up, examining the
  // terminator instructions.
  MachineBasicBlock::iterator I = MBB.end(), UnCondBrIter = I;
  while (I != MBB.begin()) {
    --I;
    if (I->isDebugValue()) {
      continue;
    }

    // Working from the bottom, when we see a non-terminator instruction, we're
    // done.
    if (!isUnpredicatedTerminator(*I)) {
      break;
    }

    // A terminator that isn't a branch can't easily be handled by this
    // analysis.
    if (!I->isBranch()) {
      return true;
    }

    // Cannot handle branches that don't branch to a block.
    if (!I->getOperand(0).isMBB()) {
      return true;
    }

    // Handle unconditional branches.
    if (I->getNumOperands() == 1) {
      UnCondBrIter = I;

      if (!AllowModify) {
        TBB = I->getOperand(0).getMBB();
        continue;
      }

      // If the block has any instructions after a JMP, delete them.
      while (std::next(I) != MBB.end()) {
        std::next(I)->eraseFromParent();
      }
      Cond.clear();
      FBB = nullptr;

      // Delete the JMP if it's equivalent to a fall-through.
      if (MBB.isLayoutSuccessor(I->getOperand(0).getMBB())) {
        TBB = nullptr;
        I->eraseFromParent();
        I = MBB.end();
        UnCondBrIter = I;
        continue;
      }

      // TBB is used to indicate the unconditional destination.
      TBB = I->getOperand(0).getMBB();
      continue;
    }

    // Handle conditional branches.
    assert(I->getNumExplicitOperands() == 2 && "Invalid conditional branch");
    I8080::CondCode BranchCode = I8080::CondCode(I->getOperand(1).getImm());

    // Working from the bottom, handle the first conditional branch.
    if (Cond.empty()) {
      MachineBasicBlock *TargetBB = I->getOperand(0).getMBB();
      if (AllowModify && UnCondBrIter != MBB.end() &&
          MBB.isLayoutSuccessor(TargetBB)) {
        // If we can modify the code and it ends in something like:
        //
        //     jCC L1
        //     jmp L2
        //   L1:
        //     ...
        //   L2:
        //
        // Then we can change this to:
        //
        //     jnCC L2
        //   L1:
        //     ...
        //   L2:
        //
        // Which is a bit more efficient.
        // We conditionally jump to the fall-through block.
        BranchCode = GetOppositeBranchCondition(BranchCode);
        MachineBasicBlock::iterator OldInst = I;

        BuildMI(MBB, UnCondBrIter, MBB.findDebugLoc(I), get(I8080::JQCC))
        .addMBB(UnCondBrIter->getOperand(0).getMBB()).addImm(BranchCode);
        BuildMI(MBB, UnCondBrIter, MBB.findDebugLoc(I), get(I8080::JQ))
        .addMBB(TargetBB);

        OldInst->eraseFromParent();
        UnCondBrIter->eraseFromParent();

        // Restart the analysis.
        UnCondBrIter = MBB.end();
        I = MBB.end();
        continue;
      }

      FBB = TBB;
      TBB = I->getOperand(0).getMBB();
      Cond.push_back(MachineOperand::CreateImm(BranchCode));
      continue;
    }

    return true;
  }

  return false;
}

unsigned I8080InstrInfo::removeBranch(MachineBasicBlock &MBB,
                                    int *BytesRemoved) const {
  assert(!BytesRemoved && "code size not handled");
  MachineBasicBlock::iterator I = MBB.end();
  unsigned Count = 0;

  while (I != MBB.begin()) {
    --I;
    if (I->isDebugValue()) {
      continue;
    }
    if (I->getOpcode() != I8080::JQ &&
        I->getOpcode() != I8080::JQCC) {
      break;
    }
    // Remove the branch.
    I->eraseFromParent();
    I = MBB.end();
    ++Count;
  }

  return Count;
}

unsigned I8080InstrInfo::insertBranch(MachineBasicBlock &MBB,
                                    MachineBasicBlock *TBB,
                                    MachineBasicBlock *FBB,
                                    ArrayRef<MachineOperand> Cond,
                                    const DebugLoc &DL,
                                    int *BytesAdded) const {
  // Shouldn't be a fall through.
  assert(TBB && "InsertBranch must not be told to insert a fallthrough");
  assert(Cond.size() <= 1 && "I8080 branch conditions have one component!");
  assert(!BytesAdded && "code size not handled");

  if (Cond.empty()) {
    // Unconditional branch?
    assert(!FBB && "Unconditional branch with multiple successors!");
    BuildMI(&MBB, DL, get(I8080::JQ)).addMBB(TBB);
    return 1;
  }

  // Conditional branch.
  unsigned Count = 0;
  BuildMI(&MBB, DL, get(I8080::JQCC)).addMBB(TBB).addImm(Cond[0].getImm());
  ++Count;

  // If FBB is null, it is implied to be a fall-through block.
  if (FBB) {
    // Two-way Conditional branch. Insert the second branch.
    BuildMI(&MBB, DL, get(I8080::JQ)).addMBB(FBB);
    ++Count;
  }
  return Count;
}

bool I8080InstrInfo::
reverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const {
  assert(Cond.size() == 1 && "Invalid I8080 branch condition!");
  I8080::CondCode CC = static_cast<I8080::CondCode>(Cond[0].getImm());
  Cond[0].setImm(GetOppositeBranchCondition(CC));
  return false;
}

bool I8080::splitReg(
  unsigned ByteSize, unsigned Opc8, // unsigned Opc16, unsigned Opc24,
  unsigned &RC, unsigned &LoOpc, unsigned &LoIdx, unsigned &HiOpc,
  unsigned &HiIdx, unsigned &HiOff) {
  switch (ByteSize) {
  default: llvm_unreachable("Unexpected Size!");
  case 1:
    RC = I8080::RR8RegClassID;
    LoOpc = HiOpc = Opc8;
    LoIdx = HiIdx = I8080::NoSubRegister;
    HiOff = 0;
    return false;
#if 0
  case 2:
    RC = I8080::R16RegClassID;
    LoOpc = HiOpc = Opc8;
    LoIdx = I8080::sub_low;
    HiIdx = I8080::sub_high;
    HiOff = 1;
    return true;
#endif // 0
  }
}

bool I8080InstrInfo::canExchange(unsigned RegA, unsigned RegB) const {
  // The only regs that can be directly exchanged are DE and HL, in any order.
  bool DE = false, HL = false;
  for (unsigned Reg : {RegA, RegB}) {
    if (RI.isSubRegisterEq(I8080::DE, Reg)) {
      DE = true;
    } else if (RI.isSubRegisterEq(I8080::HL, Reg)) {
      HL = true;
    }
  }
  return DE && HL;
}

void I8080InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator MI,
                               const DebugLoc &DL, unsigned DstReg,
                               unsigned SrcReg, bool KillSrc) const {
  LLVM_DEBUG(dbgs() << RI.getName(DstReg) << " = " << RI.getName(SrcReg) << '\n');
  /*for (auto Regs : {std::make_pair(DstReg, &SrcReg),
                    std::make_pair(SrcReg, &DstReg)}) {
    if (I8080::RR8RegClassID.contains(Regs.first) &&
        (I8080::R16RegClass.contains(*Regs.second) ||
         I8080::R24RegClass.contains(*Regs.second)))
      *Regs.second = RI.getSubReg(*Regs.second, I8080::sub_low);
  }*/
  // Identity copy.
  if (DstReg == SrcReg) {
    return;
  }
  if (I8080::RR8RegClass.contains(DstReg, SrcReg)) {
    // Byte copy.
    if (I8080::GR8RegClass.contains(DstReg, SrcReg)) {
      // Neither are index registers.
      BuildMI(MBB, MI, DL, get(I8080::LD8gg), DstReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
#if 0 // $MS: Don't use index half registers
    } else if (I8080::I8RegClass.contains(DstReg, SrcReg)) {
      assert(Subtarget.hasIndexHalfRegs() && "Need  index half registers");
      // Both are index registers.
      if (I8080::X8RegClass.contains(DstReg, SrcReg)) {
        BuildMI(MBB, MI, DL, get(I8080::LD8xx), DstReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
      } else if (I8080::Y8RegClass.contains(DstReg, SrcReg)) {
        BuildMI(MBB, MI, DL, get(I8080::LD8yy), DstReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
      } else {
        // We are copying between different index registers, so we need to use
        // an intermediate register.
        BuildMI(MBB, MI, DL, get(I8080::PUSH16AF));
        BuildMI(MBB, MI, DL, get(I8080::X8RegClass.contains(SrcReg) ? I8080::LD8xx
                                 : I8080::LD8yy),
                I8080::A).addReg(SrcReg, getKillRegState(KillSrc));
        BuildMI(MBB, MI, DL, get(I8080::X8RegClass.contains(DstReg) ? I8080::LD8xx
                                 : I8080::LD8yy),
                DstReg).addReg(I8080::A);
        BuildMI(MBB, MI, DL, get(I8080::POP16AF));
      }
    } else {
      assert(Subtarget.hasIndexHalfRegs() && "Need  index half registers");
      // Only one is an index register, which isn't directly possible if one of
      // them is from HL.  If so, surround with EX DE,HL and use DE instead.
      bool NeedEX = false;
      for (unsigned *Reg : {&DstReg, &SrcReg}) {
        switch (*Reg) {
        case I8080::H: *Reg = I8080::D; NeedEX = true; break;
        case I8080::L: *Reg = I8080::E; NeedEX = true; break;
        }
      }
      unsigned ExOpc = I8080::EX16DE;
      if (NeedEX) {
        // If the prev instr was an EX DE,HL, just kill it.
        if (MI != MBB.begin() && std::prev(MI)->getOpcode() == ExOpc) {
          std::prev(MI)->eraseFromParent();
        } else {
          MachineInstr &ExMI = *BuildMI(MBB, MI, DL, get(ExOpc));
          for (unsigned Reg : { I8080::DE, I8080::HL })
            ExMI.findRegisterUseOperand(Reg)->setIsUndef();
        }
      }
      BuildMI(MBB, MI, DL,
              get(I8080::X8RegClass.contains(DstReg, SrcReg) ? I8080::LD8xx
                  : I8080::LD8yy),
              DstReg).addReg(SrcReg, getKillRegState(KillSrc));
      if (NeedEX) {
        BuildMI(MBB, MI, DL, get(ExOpc));
      }
#endif // 0
    }
    return;
  }
  // Specialized word copy.
  // Copies to SP.
  if (DstReg == I8080::SPS) {
    assert((I8080::AIR16RegClass.contains(SrcReg) || SrcReg == I8080::DE) &&
           "Unimplemented");
    if (SrcReg == I8080::DE)
      BuildMI(MBB, MI, DL, get(I8080::EX16DE))
      .addReg(DstReg, RegState::ImplicitDefine)
      .addReg(SrcReg, RegState::ImplicitKill);
    BuildMI(MBB, MI, DL, get(I8080::LD16SP))
    .addReg(SrcReg, getKillRegState(KillSrc));
    if (SrcReg == I8080::DE)
      BuildMI(MBB, MI, DL, get(I8080::EX16DE))
      .addReg(DstReg, RegState::ImplicitDefine)
      .addReg(SrcReg, RegState::ImplicitKill);
    return;
  }
  // Copies from SP.
  if (SrcReg == I8080::SPS) {
    assert((I8080::AIR16RegClass.contains(DstReg) || DstReg == I8080::DE) &&
           "Unimplemented");
    if (DstReg == I8080::DE)
      BuildMI(MBB, MI, DL, get(I8080::EX16DE))
      .addReg(DstReg, RegState::ImplicitDefine)
      .addReg(SrcReg, RegState::ImplicitKill);
    BuildMI(MBB, MI, DL, get(I8080::LD16ri),
            DstReg).addImm(0);
    BuildMI(MBB, MI, DL, get(I8080::ADD16SP),
            DstReg).addReg(DstReg);
    if (DstReg == I8080::DE)
      BuildMI(MBB, MI, DL, get(I8080::EX16DE))
      .addReg(DstReg, RegState::ImplicitDefine)
      .addReg(SrcReg, RegState::ImplicitKill);
    return;
  }
  //if (Is24Bit == Subtarget.is24Bit()) // $TODO ??? should always be true
  {
    // Special case DE/HL = HL/DE<kill> as EX DE,HL.
    if (KillSrc && canExchange(DstReg, SrcReg)) {
      MachineInstrBuilder MIB = BuildMI(MBB, MI, DL,
                                        get(I8080::EX16DE));
      MIB->findRegisterUseOperand(SrcReg)->setIsKill();
      MIB->findRegisterDefOperand(SrcReg)->setIsDead();
      MIB->findRegisterUseOperand(DstReg)->setIsUndef();
      return;
    }
    bool IsSrcIndexReg = I8080::IR16RegClass.contains(SrcReg);
    // If both are 24-bit then the upper byte needs to be preserved.
    // Otherwise copies of index registers may need to use this method if:
    // - We are optimizing for size and exactly one reg is an index reg because
    //     PUSH SrcReg \ POP DstReg is (2 + NumIndexRegs) bytes but slower
    //     LD DstRegLo,SrcRegLo \ LD DstRegHi,SrcRegHi is 4 bytes but faster
    // - We don't have undocumented half index copies
    bool IsDstIndexReg = I8080::IR16RegClass.contains(DstReg);
    unsigned NumIndexRegs = IsSrcIndexReg + IsDstIndexReg;
    bool OptSize = MBB.getParent()->getFunction().getAttributes()
                   .hasAttribute(AttributeList::FunctionIndex, Attribute::OptimizeForSize);
    if ((NumIndexRegs == 1 && OptSize) ||
        (NumIndexRegs && !Subtarget.hasIndexHalfRegs())) {
      BuildMI(MBB, MI, DL, get(I8080::PUSH16r))
      .addReg(SrcReg, getKillRegState(KillSrc));
      BuildMI(MBB, MI, DL, get(I8080::POP16r), DstReg);
      return;
    }
  }
  // Otherwise, implement as two copies. A 16-bit copy should copy high and low
  // 8 bits separately.
  assert(I8080::R16RegClass.contains(DstReg, SrcReg) && "Unknown copy width");
  unsigned SubLo = I8080::sub_low;
  unsigned SubHi = I8080::sub_high;
  unsigned DstLoReg = RI.getSubReg(DstReg, SubLo);
  unsigned SrcLoReg = RI.getSubReg(SrcReg, SubLo);
  unsigned DstHiReg = RI.getSubReg(DstReg, SubHi);
  unsigned SrcHiReg = RI.getSubReg(SrcReg, SubHi);
  /*bool DstLoSrcHiOverlap = RI.regsOverlap(DstLoReg, SrcHiReg);
  bool SrcLoDstHiOverlap = RI.regsOverlap(SrcLoReg, DstHiReg);
  if (DstLoSrcHiOverlap && SrcLoDstHiOverlap) {
    assert(KillSrc &&
           "Both parts of SrcReg and DstReg overlap but not killing source!");
    // e.g. EUHL = LUDE so just swap the operands
    unsigned OtherReg;
    if (canExchange(DstLoReg, SrcLoReg)) {
      BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? I8080::EX24DE : I8080::EX16DE))
        .addReg(DstReg, RegState::ImplicitDefine)
        .addReg(SrcReg, RegState::ImplicitKill);
    } else if ((OtherReg = DstLoReg, RI.isSubRegisterEq(I8080::UHL, SrcLoReg)) ||
               (OtherReg = SrcLoReg, RI.isSubRegisterEq(I8080::UHL, DstLoReg))) {
      BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? I8080::PUSH24r : I8080::PUSH16r))
        .addReg(OtherReg, RegState::Kill);
      BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? I8080::EX24SP : I8080::EX16SP));
      BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? I8080::POP24r : I8080::POP16r),
              OtherReg);
    } else {
      BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? I8080::PUSH24r : I8080::PUSH16r))
        .addReg(SrcLoReg, RegState::Kill);
      BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? I8080::PUSH24r : I8080::PUSH16r))
        .addReg(DstLoReg, RegState::Kill);
      BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? I8080::POP24r : I8080::POP16r),
              SrcLoReg);
      BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? I8080::POP24r : I8080::POP16r),
              DstLoReg);
    }
    // Check if top needs to be moved (e.g. EUHL = HUDE).
    unsigned DstHiIdx = RI.getSubRegIndex(SrcLoReg, DstHiReg);
    unsigned SrcHiIdx = RI.getSubRegIndex(DstLoReg, SrcHiReg);
    if (DstHiIdx != SrcHiIdx)
      copyPhysReg(MBB, MI, DL, DstHiReg,
                  RI.getSubReg(DstLoReg, SrcHiIdx), KillSrc);
  } else if (DstLoSrcHiOverlap) {
    // Copy out SrcHi before SrcLo overwrites it.
    copyPhysReg(MBB, MI, DL, DstHiReg, SrcHiReg, KillSrc);
    copyPhysReg(MBB, MI, DL, DstLoReg, SrcLoReg, KillSrc);
  } else*/ {
    // If SrcLoDstHiOverlap then copy out SrcLo before SrcHi overwrites it,
    // otherwise the order doesn't matter.
    copyPhysReg(MBB, MI, DL, DstLoReg, SrcLoReg, KillSrc);
    copyPhysReg(MBB, MI, DL, DstHiReg, SrcHiReg, KillSrc);
  }
  --MI;
  MI->addRegisterDefined(DstReg, &RI);
  if (KillSrc) {
    MI->addRegisterKilled(SrcReg, &RI, true);
  }
}

static const MachineInstrBuilder &
addSubReg(const MachineInstrBuilder &MIB, unsigned Reg, unsigned Idx,
          const MCRegisterInfo *TRI, unsigned Flags = 0) {
  if (Idx && TargetRegisterInfo::isPhysicalRegister(Reg)) {
    Reg = TRI->getSubReg(Reg, Idx);
    Idx = 0;
  }
  return MIB.addReg(Reg, Flags, Idx);
}

void I8080InstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator MI,
                                       unsigned SrcReg, bool IsKill, int FI,
                                       const TargetRegisterClass *TRC,
                                       const TargetRegisterInfo *TRI) const {
  unsigned Opc;
  switch (TRI->getSpillSize(*TRC)) {
  default:
    llvm_unreachable("Unexpected regclass size");
  case 1:
    Opc = I8080::LD8or;
    break;
  case 2:
    Opc = I8080::LD88or;
    break;
  }
  BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(Opc)).addFrameIndex(FI).addImm(0)
  .addReg(SrcReg, getKillRegState(IsKill));
  return;
  unsigned RC, LoOpc, LoIdx, HiOpc, HiIdx, HiOff;
  bool Split =
    I8080::splitReg(TRI->getSpillSize(*TRC), I8080::LD8or, // I8080::LD16or,
                  RC, LoOpc, LoIdx, HiOpc, HiIdx, HiOff);
  MachineInstrBuilder LoMIB =
    addSubReg(BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(LoOpc))
              .addFrameIndex(FI).addImm(0), SrcReg, LoIdx, TRI,
              getKillRegState(IsKill));
  if (Split) {
    MachineInstrBuilder HiMIB = addSubReg(
                                  BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(HiOpc))
                                  .addFrameIndex(FI).addImm(HiOff), SrcReg, HiIdx, TRI,
                                  getKillRegState(IsKill));
    if (IsKill) {
      HiMIB->addRegisterKilled(SrcReg, TRI, true);
    }
    //HiMIB->bundleWithPred();
    //finalizeBundle(MBB, MachineBasicBlock::instr_iterator(LoMIB),
    //               std::next(MachineBasicBlock::instr_iterator(HiMIB)));
  }
}

void I8080InstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator MI,
                                        unsigned DstReg, int FI,
                                        const TargetRegisterClass *TRC,
                                        const TargetRegisterInfo *TRI) const {
  unsigned Opc;
  switch (TRI->getSpillSize(*TRC)) {
  default:
    llvm_unreachable("Unexpected regclass size");
  case 1:
    Opc = I8080::LD8ro;
    break;
  case 2:
    Opc = I8080::LD88ro;
    break;
  }
  BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(Opc), DstReg).addFrameIndex(FI)
  .addImm(0);
  return;
  unsigned RC, LoOpc, LoIdx, HiOpc, HiIdx, HiOff;
  bool Split =
    I8080::splitReg(TRI->getSpillSize(*TRC), I8080::LD8ro, // I8080::LD16ro,
                  RC, LoOpc, LoIdx, HiOpc, HiIdx, HiOff);
  MachineInstrBuilder LoMIB =
    addSubReg(BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(LoOpc)), DstReg, LoIdx,
              TRI, RegState::DefineNoRead).addFrameIndex(FI).addImm(0);
  if (Split) {
    MachineInstrBuilder HiMIB = addSubReg(
                                  BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(HiOpc)), DstReg, HiIdx,
                                  TRI, RegState::Define).addFrameIndex(FI).addImm(HiOff);
    HiMIB->addRegisterDefined(DstReg, TRI);
    //HiMIB->bundleWithPred();
    //finalizeBundle(MBB, MachineBasicBlock::instr_iterator(LoMIB),
    //               std::next(MachineBasicBlock::instr_iterator(HiMIB)));
  }
}

/// Return true and the FrameIndex if the specified
/// operand and follow operands form a reference to the stack frame.
bool I8080InstrInfo::isFrameOperand(const MachineInstr &MI, unsigned int Op,
                                  int &FrameIndex) const {
  if (MI.getOperand(Op).isFI() &&
      MI.getOperand(Op + 1).isImm() && MI.getOperand(Op + 1).getImm() == 0) {
    FrameIndex = MI.getOperand(Op).getIndex();
    return true;
  }
  return false;
}

static bool isFrameLoadOpcode(int Opcode) {
  switch (Opcode) {
  default:
    return false;
  case I8080::LD8ro:
  case I8080::LD88ro:
    return true;
  }
}
unsigned I8080InstrInfo::isLoadFromStackSlot(const MachineInstr &MI,
                                           int &FrameIndex) const {
  if (isFrameLoadOpcode(MI.getOpcode()) && !MI.getOperand(0).getSubReg() &&
      isFrameOperand(MI, 1, FrameIndex)) {
    return MI.getOperand(0).getReg();
  }
  return 0;
}

static bool isFrameStoreOpcode(int Opcode) {
  switch (Opcode) {
  default:
    return false;
  case I8080::LD8or:
  case I8080::LD88or:
    return true;
  }
}
unsigned I8080InstrInfo::isStoreToStackSlot(const MachineInstr &MI,
                                          int &FrameIndex) const {
  if (isFrameStoreOpcode(MI.getOpcode()) && !MI.getOperand(2).getSubReg() &&
      isFrameOperand(MI, 0, FrameIndex)) {
    return MI.getOperand(2).getReg();
  }
  return 0;
}

bool I8080InstrInfo::isReallyTriviallyReMaterializable(const MachineInstr &MI,
                                                     AliasAnalysis *AA) const {
  switch (MI.getOpcode()) {
  case I8080::LD8r0:
    return true;
  }
  return false;
}

void I8080InstrInfo::reMaterialize(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator I,
                                 unsigned DstReg, unsigned SubIdx,
                                 const MachineInstr &Orig,
                                 const TargetRegisterInfo &TRI) const {
  if (!Orig.modifiesRegister(I8080::F, &TRI) ||
      MBB.computeRegisterLiveness(&TRI, I8080::F, I) ==
      MachineBasicBlock::LQR_Dead) {
    return TargetInstrInfo::reMaterialize(MBB, I, DstReg, SubIdx, Orig, TRI);
  }
  // The instruction clobbers F. Re-materialize as LDri to avoid side effects.
  unsigned Opc;
  int Val;
  switch (Orig.getOpcode()) {
  default: llvm_unreachable("Unexpected instruction!");
  case I8080::LD8r0:   Opc = I8080::LD8ri;  Val =  0; break;
  }
  BuildMI(MBB, I, Orig.getDebugLoc(), get(Opc))
  .addReg(DstReg, RegState::Define, SubIdx).addImm(Val);
}

void I8080InstrInfo::
expandLoadStoreWord(const TargetRegisterClass *ARC, unsigned AOpc,
                    const TargetRegisterClass *ORC, unsigned OOpc,
                    MachineInstr &MI, unsigned RegIdx) const {
  unsigned Reg = MI.getOperand(RegIdx).getReg();
  assert(ARC->contains(Reg) != ORC->contains(Reg) &&
         "RegClasses should be covering and disjoint");
  MI.setDesc(get(ARC->contains(Reg) ? AOpc : OOpc));
  (void)ORC;
}

bool I8080InstrInfo::expandPostRAPseudo(MachineInstr &MI) const {
  DebugLoc DL = MI.getDebugLoc();
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  auto Next = ++MachineBasicBlock::iterator(MI);
  MachineInstrBuilder MIB(MF, MI);
  const TargetRegisterInfo &TRI = getRegisterInfo();
  //bool Is24Bit = false; // Subtarget.is24Bit();
  bool UseLEA = false; // = Is24Bit && !MF.getFunction().getAttributes()
  //.hasAttribute(AttributeList::FunctionIndex, Attribute::OptimizeForSize);
  LLVM_DEBUG(dbgs() << "\nI8080InstrInfo::expandPostRAPseudo:"; MI.dump());
  switch (unsigned Opc = MI.getOpcode()) {
  default:
    return false;
  case I8080::RCF:
    MI.setDesc(get(I8080::OR8ar));
    MIB.addReg(I8080::A, RegState::Undef);
    break;
  case I8080::LD8r0:
    if (MI.getOperand(0).getReg() == I8080::A) {
      MI.setDesc(get(I8080::XOR8ar));
      MI.getOperand(0).setIsUse();
      MI.getOperand(0).setIsUndef();
      MIB.addReg(I8080::A, RegState::ImplicitDefine);
    } else {
      MI.setDesc(get(I8080::LD8ri));
      MI.findRegisterDefOperand(I8080::F)->ChangeToImmediate(0);
    }
    break;
  case I8080::CP16ao: {
      unsigned Reg = Opc == I8080::HL;
      if (MBB.computeRegisterLiveness(&TRI, Reg, Next) !=
          MachineBasicBlock::LQR_Dead) {
        BuildMI(MBB, Next, DL, get(I8080::ADD16ao), Reg)
        .addReg(Reg).add(MI.getOperand(0));
        MI.getOperand(0).setIsKill(false);
      }
      LLVM_FALLTHROUGH;
    }
  case I8080::SUB16ao:
    expandPostRAPseudo(*BuildMI(MBB, MI, DL, get(I8080::RCF)));
    MI.setDesc(get(I8080::SBC16ao));
    break;
  case I8080::CP16a0: {
      unsigned Reg = Opc == I8080::HL;
      unsigned UndefReg = Opc == I8080::BC;
      BuildMI(MBB, MI, DL, get(I8080::ADD16ao),
              Reg).addReg(Reg).addReg(UndefReg, RegState::Undef);
      expandPostRAPseudo(*BuildMI(MBB, MI, DL, get(I8080::RCF)));
      MI.setDesc(get(Opc == I8080::SBC16ao));
      MIB.addReg(UndefReg, RegState::Undef);
      break;
    }
  case I8080::LD8ro:
  case I8080::LD8rp: {
#if 0 // $MS: Requires index half regs
      MachineOperand &DstOp = MI.getOperand(0);
      if (I8080::I8RegClass.contains(DstOp.getReg())) {
        BuildMI(MBB, MI, DL, get(I8080::PUSH16AF));
        DstOp.setReg(I8080::A);
        copyPhysReg(MBB, Next, DL, DstOp.getReg(), I8080::A, true);
        BuildMI(MBB, Next, DL, get(I8080::POP16AF));
      }
#endif // 0
      MI.setDesc(get(Opc == I8080::LD8ro ? I8080::LD8go : I8080::LD8gp));
      break;
    }
  case I8080::LD88rp:
  case I8080::LD88ro: {
      //assert(!Subtarget.has16BitEI8080Ops() &&
      //       "LD88rp/LD88ro is not used on the ez80 in 16-bit mode");
      unsigned NewOpc = Opc == I8080::LD88rp ? I8080::LD8rp : I8080::LD8ro;
      MachineOperand &DstOp = MI.getOperand(0);
      const MachineOperand &AddrOp = MI.getOperand(1);
      unsigned OrigReg = DstOp.getReg();
      unsigned Reg = OrigReg;
      bool Index = I8080::IR16RegClass.contains(Reg);
      bool Overlap = NewOpc == I8080::LD8rp && Reg == I8080::HL;
      unsigned ScratchReg;
      if (Index || Overlap) {
        Reg = Index ? I8080::HL : I8080::DE;
        ScratchReg = Reg;
        BuildMI(MBB, MI, DL, get(I8080::PUSH16r))
        .addReg(ScratchReg, RegState::Undef);
      }
      MIB = BuildMI(MBB, MI, DL, get(NewOpc), TRI.getSubReg(Reg, I8080::sub_low))
            .addReg(AddrOp.getReg());
      if (NewOpc == I8080::LD8rp)
        BuildMI(MBB, MI, DL, get(I8080::INC16r),
                AddrOp.getReg()).addReg(AddrOp.getReg());
      MI.setDesc(get(NewOpc));
      DstOp.setReg(TRI.getSubReg(Reg, I8080::sub_high));
      if (NewOpc == I8080::LD8ro) {
        MachineOperand &OffOp = MI.getOperand(2);
        MIB.addImm(OffOp.getImm());
        OffOp.setImm(OffOp.getImm() + 1);
      } else if (!AddrOp.isKill())
        BuildMI(MBB, Next, DL, get(I8080::DEC16r),
                AddrOp.getReg()).addReg(AddrOp.getReg());
      if (Index)
        BuildMI(MBB, Next, DL, get(I8080::EX16SP),
                ScratchReg).addReg(ScratchReg);
      else if (Overlap) {
        copyPhysReg(MBB, Next, DL, AddrOp.getReg(), ScratchReg, true);
      }
      if (Index || Overlap)
        BuildMI(MBB, Next, DL, get(I8080::POP16r),
                Overlap ? ScratchReg : OrigReg);
      expandPostRAPseudo(*MIB);
      expandPostRAPseudo(MI);
      LLVM_DEBUG(MI.dump());
      break;
    }
  case I8080::LD8or:
  case I8080::LD8pr: {
#if 0 // $MS: Requires index half regs
      MachineOperand &SrcOp = MI.getOperand(MI.getNumExplicitOperands() - 1);
      if (I8080::I8RegClass.contains(SrcOp.getReg())) {
        BuildMI(MBB, MI, DL, get(I8080::PUSH16AF));
        copyPhysReg(MBB, MI, DL, I8080::A, SrcOp.getReg(), SrcOp.isKill());
        SrcOp.setReg(I8080::A);
        SrcOp.setIsKill();
        BuildMI(MBB, Next, DL, get(I8080::POP16AF));
      }
#endif // 0
      MI.setDesc(get(Opc == I8080::LD8or ? I8080::LD8og : I8080::LD8pg));
      break;
    }
  case I8080::LD88pr:
  case I8080::LD88or: {
      //assert(!Subtarget.has16BitEI8080Ops() &&
      //       "LD88pr/LD88or is not used on the ez80 in 16-bit mode");
      unsigned NewOpc = Opc == I8080::LD88pr ? I8080::LD8pr : I8080::LD8or;
      const MachineOperand &AddrOp = MI.getOperand(0);
      MachineOperand &SrcOp = MI.getOperand(MI.getNumExplicitOperands() - 1);
      unsigned Reg = SrcOp.getReg();
      bool Index = I8080::IR16RegClass.contains(Reg);
      bool Overlap = NewOpc == I8080::LD8pr && Reg == I8080::HL;
      unsigned ScratchReg = I8080::HL;
      if (Index) {
        //unsigned SuperReg = TRI.getMatchingSuperReg(Reg, I8080::sub_short,
        //                                            &I8080::R24RegClass);
        BuildMI(MBB, MI, DL, get(I8080::PUSH16r))
        .addReg(UseLEA || Overlap ? ScratchReg : Reg,
                RegState::Undef);
        //if (UseLEA)
        //  BuildMI(MBB, MI, DL, get(I8080::LEA24ro), I8080::UHL)
        //    .addReg(SuperReg).addImm(0);
        //else
        BuildMI(MBB, MI, DL, get(I8080::EX16SP),
                ScratchReg).addReg(ScratchReg);
        Reg = I8080::HL;
      } else if (Overlap) {
        BuildMI(MBB, MI, DL, get(I8080::PUSH16AF));
        Reg = I8080::HL;
      }
      MIB = BuildMI(MBB, MI, DL, get(NewOpc)).addReg(AddrOp.getReg());
      if (NewOpc == I8080::LD8or) {
        MachineOperand &OffOp = MI.getOperand(1);
        MIB.addImm(OffOp.getImm());
        OffOp.setImm(OffOp.getImm() + 1);
      } else if (!AddrOp.isKill())
        BuildMI(MBB, Next, DL, get(I8080::DEC16r),
                AddrOp.getReg()).addReg(AddrOp.getReg());
      MIB.addReg(TRI.getSubReg(Reg, I8080::sub_low));
      Reg = TRI.getSubReg(Reg, I8080::sub_high);
      if (Overlap) {
        copyPhysReg(MBB, MI, DL, I8080::A, Reg, false);
        Reg = I8080::A;
      }
      if (NewOpc == I8080::LD8pr)
        BuildMI(MBB, MI, DL, get(I8080::INC16r),
                AddrOp.getReg()).addReg(AddrOp.getReg());
      MI.setDesc(get(NewOpc));
      SrcOp.setReg(Reg);
      if (Index)
        BuildMI(MBB, Next, DL, get(I8080::POP16r),
                ScratchReg);
      else if (Overlap) {
        BuildMI(MBB, Next, DL, get(I8080::POP16AF));
      }
      expandPostRAPseudo(*MIB);
      expandPostRAPseudo(MI);
      LLVM_DEBUG(MI.dump());
      break;
    }
  case I8080::LD16rm:
    expandLoadStoreWord(&I8080::AIR16RegClass, I8080::LD16am,
                        &I8080::OR16RegClass, I8080::LD16om, MI, 0);
    break;
  case I8080::LD16mr:
    expandLoadStoreWord(&I8080::AIR16RegClass, I8080::LD16ma,
                        &I8080::OR16RegClass, I8080::LD16mo, MI, 1);
    break;
  case I8080::CALL16r: {
      const char *Symbol;
      switch (MIB->getOperand(0).getReg()) {
      default: llvm_unreachable("Unexpected indcall register");
      case I8080::HL: Symbol = "_indcallhl"; break;
      case I8080::IX: Symbol = "_indcallix"; break;
      //case I8080::IY: Symbol = "_indcall"; break;
      }
      MI.setDesc(get(I8080::CALL16i));
      MI.getOperand(0).ChangeToES(Symbol);
      break;
    }
  case I8080::EI_RETI:
    BuildMI(MBB, MI, DL, get(I8080::EI));
    MI.setDesc(get(I8080::RETI));
    break;
  case I8080::TCRETURN16i:
    MI.setDesc(get(I8080::JP16));
    break;
  case I8080::TCRETURN16r:
    MI.setDesc(get(I8080::JP16r));
    break;
  case I8080::PUSH8r: {
      unsigned SrcReg8 = MI.getOperand(0).getReg();
      unsigned DstReg16 = llvm::getI8080SuperRegisterOrZero(SrcReg8);
      assert(DstReg16 && "Cannot push 8 bit register via 16 bit pair");
      MI.setDesc(get(I8080::PUSH16r));
      MI.getOperand(0).setReg(DstReg16);
    }
    break;
  }
  LLVM_DEBUG(MIB->dump());
  return true;
}

bool I8080InstrInfo::analyzeCompare(const MachineInstr &MI,
                                  unsigned &SrcReg, unsigned &SrcReg2,
                                  int &CmpMask, int &CmpValue) const {
  switch (MI.getOpcode()) {
  default: return false;
  case I8080::OR8ar:
    SrcReg = I8080::A;
    if (MI.getOperand(1).getReg() != SrcReg) {
      return false;
    }
    // Compare against zero.
    SrcReg2 = 0;
    CmpMask = ~0;
    CmpValue = 0;
    break;
  case I8080::CP8ai:
  case I8080::SUB8ai:
    SrcReg = I8080::A;
    SrcReg2 = 0;
    CmpMask = CmpValue = 0;
    if (MI.getOperand(0).isImm()) {
      CmpMask = ~0;
      CmpValue = MI.getOperand(0).getImm();
    }
    break;
  case I8080::CP8ar:
  case I8080::SUB8ar:
    SrcReg = I8080::A;
    SrcReg2 = MI.getOperand(0).getReg();
    CmpMask = CmpValue = 0;
    break;
  case I8080::CP8ap:
  case I8080::CP8ao:
  case I8080::SUB8ap:
  case I8080::SUB8ao:
    SrcReg = I8080::A;
    SrcReg2 = CmpMask = CmpValue = 0;
    break;
  }
  MachineBasicBlock::const_reverse_iterator I = MI, E = MI.getParent()->rend();
  while (++I != E && I->isFullCopy())
    for (unsigned *Reg : {&SrcReg, &SrcReg2})
      if (TargetRegisterInfo::isPhysicalRegister(*Reg) &&
          *Reg == I->getOperand(0).getReg()) {
        *Reg = I->getOperand(1).getReg();
      }
  return true;
}

/// Check whether the first instruction, whose only purpose is to update flags,
/// can be made redundant. CP8ar is made redundant by SUB8ar if the operands are
/// the same.
/// SrcReg, SrcReg2: register operands for FlagI.
/// ImmValue: immediate for FlagI if it takes an immediate.
inline static bool isRedundantFlagInstr(MachineInstr &FI, unsigned SrcReg,
                                        unsigned SrcReg2, int ImmMask,
                                        int ImmValue, MachineInstr &OI) {
  if (ImmMask)
    return (FI.getOpcode() == I8080::CP8ai && OI.getOpcode() == I8080::SUB8ai) &&
           OI.getOperand(1).getImm() == ImmValue;
  else
    return (FI.getOpcode() == I8080::CP8ar && OI.getOpcode() == I8080::SUB8ar) &&
           OI.getOperand(1).getReg() == SrcReg2;
}

/// Check whether the instruction sets the sign and zero flag based on its
/// result.
inline static bool isSZSettingInstr(MachineInstr &MI) {
  switch (MI.getOpcode()) {
  default: return false;
  case I8080::INC8r:  case I8080::INC8p:  case I8080::INC8o:
  case I8080::DEC8r:  case I8080::DEC8p:  case I8080::DEC8o:
  case I8080::ADD8ar: case I8080::ADD8ai: case I8080::ADD8ap: case I8080::ADD8ao:
  case I8080::ADC8ar: case I8080::ADC8ai: case I8080::ADC8ap: case I8080::ADC8ao:
  case I8080::SUB8ar: case I8080::SUB8ai: case I8080::SUB8ap: case I8080::SUB8ao:
  case I8080::SBC8ar: case I8080::SBC8ai: case I8080::SBC8ap: case I8080::SBC8ao:
  case I8080::AND8ar: case I8080::AND8ai: case I8080::AND8ap: case I8080::AND8ao:
  case I8080::XOR8ar: case I8080::XOR8ai: case I8080::XOR8ap: case I8080::XOR8ao:
  case I8080:: OR8ar: case I8080:: OR8ai: case I8080:: OR8ap: case I8080:: OR8ao:
  case I8080::SBC16ao: case I8080::NEG8:   case I8080::ADC16ao:
  case I8080::SUB16ao:
  case I8080::RLC8r:  case I8080::RLC8p:  case I8080::RLC8o:
  case I8080::RRC8r:  case I8080::RRC8p:  case I8080::RRC8o:
  case I8080:: RL8r:  case I8080:: RL8p:  case I8080:: RL8o:
  case I8080:: RR8r:  case I8080:: RR8p:  case I8080:: RR8o:
  case I8080::SLA8r:  case I8080::SLA8p:  case I8080::SLA8o:
  case I8080::SRA8r:  case I8080::SRA8p:  case I8080::SRA8o:
  case I8080::SRL8r:  case I8080::SRL8p:  case I8080::SRL8o:
    return true;
  }
}

/// Check if there exists an earlier instruction that operates on the same
/// source operands and sets flags in the same way as Compare; remove Compare if
/// possible.
bool I8080InstrInfo::optimizeCompareInstr(MachineInstr &CmpInstr,
                                        unsigned SrcReg, unsigned SrcReg2,
                                        int CmpMask, int CmpValue,
                                        const MachineRegisterInfo *MRI) const {
  // If we are comparing against zero, check whether we can use MI to update F.
  bool IsCmpZero = CmpMask && !CmpValue;

  // Check whether we can replace SUB with CP.
  unsigned CpOp;
  switch (CmpInstr.getOpcode()) {
  default: CpOp = 0; break;
  case I8080::SUB8ai: CpOp = IsCmpZero ? I8080::OR8ar : I8080::CP8ai; break;
  case I8080::SUB8ar: CpOp = I8080::CP8ar; break;
  case I8080::SUB8ap: CpOp = I8080::CP8ap; break;
  case I8080::SUB8ao: CpOp = I8080::CP8ao; break;
  }
  if (CpOp) {
    int DeadDef = CmpInstr.findRegisterDefOperandIdx(I8080::A, /*isDead*/true);
    if (DeadDef == -1) {
      return false;
    }
    // There is no use of the destination register, so we replace SUB with CP.
    CmpInstr.setDesc(get(CpOp));
    if (CpOp == I8080::OR8ar) {
      CmpInstr.getOperand(0).ChangeToRegister(I8080::A, false);
    } else {
      CmpInstr.RemoveOperand(DeadDef);
    }
  }

  // Get the unique definition of SrcReg.
  MachineInstr *MI = MRI->getUniqueVRegDef(SrcReg);
  if (!MI) { return false; }

  MachineBasicBlock::iterator I = CmpInstr, Def = MI;

  const TargetRegisterInfo *TRI = &getRegisterInfo();
  for (auto RI = ++Def.getReverse(), RE = MI->getParent()->rend();
       MI->isFullCopy() && RI != RE; ++RI)
    if (RI->definesRegister(MI->getOperand(1).getReg(), TRI)) {
      MI = &*RI;
    }

  // If MI is not in the same BB as CmpInstr, do not optimize.
  if (IsCmpZero && (MI->getParent() != CmpInstr.getParent() ||
                    !isSZSettingInstr(*MI))) {
    return false;
  }

  // We are searching for an earlier instruction, which will be stored in
  // SubInstr, that can make CmpInstr redundant.
  MachineInstr *SubInstr = nullptr;

  // We iterate backwards, starting from the instruction before CmpInstr, and
  // stopping when we reach the definition of a source register or the end of
  // the BB. RI points to the instruction before CmpInstr. If the definition is
  // in this BB, RE points to it, otherwise RE is the beginning of the BB.
  MachineBasicBlock::reverse_iterator RE = CmpInstr.getParent()->rend();
  if (CmpInstr.getParent() == MI->getParent()) {
    RE = Def.getReverse();  // points to MI
  }
  for (auto RI = ++I.getReverse(); RI != RE; ++RI) {
    MachineInstr &Instr = *RI;
    // Check whether CmpInstr can be made redundant by the current instruction.
    if (!IsCmpZero && isRedundantFlagInstr(CmpInstr, SrcReg, SrcReg2, CmpMask,
                                           CmpValue, Instr)) {
      SubInstr = &Instr;
      break;
    }

    // If this instruction modifies F, we can't remove CmpInstr.
    if (Instr.modifiesRegister(I8080::F, TRI)) {
      return false;
    }
  }

  // Return false if no candidates exist.
  if (!IsCmpZero && !SubInstr) {
    return false;
  }

  // Scan forward from the instruction after CmpInstr for uses of F.
  // It is safe to remove CmpInstr if F is redefined or killed.
  // If we are at the end of the BB, we need to check whether F is live-out.
  bool IsSafe = false;
  MachineBasicBlock::iterator E = CmpInstr.getParent()->end();
  for (++I; I != E; ++I) {
    const MachineInstr &Instr = *I;
    bool ModifiesFlags = Instr.modifiesRegister(I8080::F, TRI);
    bool UsesFlags = Instr.readsRegister(I8080::F, TRI);
    if (ModifiesFlags && !UsesFlags) {
      IsSafe = true;
      break;
    }
    if (!ModifiesFlags && !UsesFlags) {
      continue;
    }
    if (IsCmpZero) {
      I8080::CondCode OldCC;
      switch (Instr.getOpcode()) {
      default:
        OldCC = I8080::COND_INVALID;
        break;
      case I8080::JQCC:
        OldCC = static_cast<I8080::CondCode>(Instr.getOperand(1).getImm());
        break;
      case I8080::ADC8ar: case I8080::ADC8ai: case I8080::ADC8ap: case I8080::ADC8ao:
      case I8080::SBC8ar: case I8080::SBC8ai: case I8080::SBC8ap: case I8080::SBC8ao:
      case I8080::SBC16ao: case I8080::ADC16ao:
        OldCC = I8080::COND_C;
        break;
      }
      switch (OldCC) {
      default: break;
      case I8080::COND_NC: case I8080::COND_C:
      case I8080::COND_PO: case I8080::COND_PE:
        // CF or PV are used, we can't perform this optimization.
        return false;
      }
    }
    if (ModifiesFlags || Instr.killsRegister(I8080::F, TRI)) {
      // It is safe to remove CmpInstr if F is updated again or killed.
      IsSafe = true;
      break;
    }
  }
  if (IsCmpZero && !IsSafe) {
    MachineBasicBlock *MBB = CmpInstr.getParent();
    for (MachineBasicBlock *Successor : MBB->successors())
      if (Successor->isLiveIn(I8080::F)) {
        return false;
      }
  }

  // The instruction to be updated is either Sub or MI.
  if (IsCmpZero) {
    SubInstr = MI;
  }

  // Make sure Sub instruction defines F and mark the def live.
  unsigned i = 0, e = SubInstr->getNumOperands();
  for (; i != e; ++i) {
    MachineOperand &MO = SubInstr->getOperand(i);
    if (MO.isReg() && MO.isDef() && MO.getReg() == I8080::F) {
      MO.setIsDead(false);
      break;
    }
  }
  assert(i != e && "Unable to locate a def EFLAGS operand");

  CmpInstr.eraseFromParent();
  return true;

  // Check whether we can replace SUB with CMP.
  switch (CmpInstr.getOpcode()) {
  default: return false;
  case I8080::SUB8ai:
    // cp a,0 -> or a,a (a szhc have same behavior)
    // FIXME: This doesn't work if the pv flag is used.
    if (!CmpInstr.getOperand(0).getImm()) {
      CmpInstr.setDesc(get(I8080::OR8ar));
      CmpInstr.getOperand(0).ChangeToRegister(I8080::A, /*isDef=*/false);
      return true;
    }
    LLVM_FALLTHROUGH;
  case I8080::SUB8ar:
  case I8080::SUB8ap:
  case I8080::SUB8ao: {
      if (!CmpInstr.registerDefIsDead(I8080::A)) {
        return false;
      }
      // There is no use of the destination register, we can replace SUB with CMP.
      unsigned NewOpcode = 0;
      switch (CmpInstr.getOpcode()) {
      default: llvm_unreachable("Unreachable!");
      case I8080::SUB8ai: NewOpcode = I8080::CP8ai; break;
      case I8080::SUB8ar: NewOpcode = I8080::CP8ar; break;
      case I8080::SUB8ap: NewOpcode = I8080::CP8ap; break;
      case I8080::SUB8ao: NewOpcode = I8080::CP8ao; break;
      }
      CmpInstr.setDesc(get(NewOpcode));
      //CmpInstr.findRegisterDefOperand(I8080::A)->setIsDead(false);
      //BuildMI(*CmpInstr.getParent(), ++MachineBasicBlock::iterator(CmpInstr), CmpInstr.getDebugLoc(), get(TargetOpcode::COPY), SrcReg).addReg(I8080::A, RegState::Kill);
      return true;
    }
  }
}

MachineInstr *
I8080InstrInfo::foldMemoryOperandImpl(MachineFunction &MF, MachineInstr &MI,
                                    ArrayRef<unsigned> Ops,
                                    MachineBasicBlock::iterator InsertPt,
                                    int FrameIndex, LiveIntervals *LIS) const {
  return nullptr;
  MachineBasicBlock &MBB = *InsertPt->getParent();
  if (Ops.size() == 1 && Ops[0] == 1 && MI.isFullCopy()) {
    unsigned DstReg = MI.getOperand(0).getReg();
    if (TargetRegisterInfo::isPhysicalRegister(DstReg)) {
      unsigned Opc;
      if (I8080::RR8RegClass.contains(DstReg)) {
        Opc = I8080::LD8ro;
      } else {
        assert(false/*(I8080::R16RegClass)
               .contains(DstReg)*/ && "Unexpected physical reg");
        //Opc = I8080::LD16ro;
      }
      return BuildMI(MBB, InsertPt, MI.getDebugLoc(), get(Opc), DstReg)
             .addFrameIndex(FrameIndex).addImm(0);
    }
  }
  dbgs() << Ops.size() << ": ";
  for (unsigned Op : Ops) {
    dbgs() << Op << ' ';
  }
  MI.dump();
  return nullptr;
}
MachineInstr *
I8080InstrInfo::foldMemoryOperandImpl(MachineFunction &MF, MachineInstr &MI,
                                    ArrayRef<unsigned> Ops,
                                    MachineBasicBlock::iterator InsertPt,
                                    MachineInstr &LoadMI,
                                    LiveIntervals *LIS) const {
  return nullptr;
  llvm_unreachable("Unimplemented");
}
