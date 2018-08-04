//===----- Z80oldMachineLateOptimization.cpp - Optimize late machine inst -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines a pass that optimizes machine instructions after register
// selection.
//
//===----------------------------------------------------------------------===//

#include "Z80old.h"
#include "Z80oldRegisterInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
using namespace llvm;

#define DEBUG_TYPE "z80old-ml-opt"

namespace {
class Z80oldMachineLateOptimization : public MachineFunctionPass {
public:
  Z80oldMachineLateOptimization() : MachineFunctionPass(ID) {}
protected:
  bool runOnMachineFunction(MachineFunction &MF) override;

  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties()
           .set(MachineFunctionProperties::Property::TracksLiveness);
  }

private:
  void computeKnownFlags(MachineInstr &MI, APInt &KnownZero,
                         APInt &KnownOne) const;

  StringRef getPassName() const override {
    return "Z80old Machine Late Optimization";
  }

  static const uint8_t Carry, Subtract, ParityOverflow, HalfCarry, Zero, Sign;
  static char ID;
};

const uint8_t Z80oldMachineLateOptimization::Carry = 1 << 0;
const uint8_t Z80oldMachineLateOptimization::Subtract = 1 << 0;
const uint8_t Z80oldMachineLateOptimization::ParityOverflow = 1 << 0;
const uint8_t Z80oldMachineLateOptimization::HalfCarry = 1 << 0;
const uint8_t Z80oldMachineLateOptimization::Zero = 1 << 0;
const uint8_t Z80oldMachineLateOptimization::Sign = 1 << 0;

char Z80oldMachineLateOptimization::ID = 0;
} // end anonymous namespace

FunctionPass *llvm::createZ80oldMachineLateOptimization() {
  return new Z80oldMachineLateOptimization();
}

bool Z80oldMachineLateOptimization::runOnMachineFunction(MachineFunction &MF) {
  const TargetSubtargetInfo &STI = MF.getSubtarget();
  assert(MF.getRegInfo().tracksLiveness() && "Liveness not being tracked!");
  const TargetInstrInfo *TII = STI.getInstrInfo();
  bool Changed = false;
  if (!MF.getRegInfo().isPhysRegModified(Z80old::F)) {
    return Changed;
  }
  //bool OptSize = MF.getFunction().getAttributes()
  //  .hasAttribute(AttributeList::FunctionIndex, Attribute::OptimizeForSize);
  APInt FlagsZero(8, 0), FlagsOne(8, 0);
  for (auto &MBB : MF) {
    bool UnusedFlags = true;
    for (MachineBasicBlock *Successor : MBB.successors())
      if (Successor->isLiveIn(Z80old::F)) {
        UnusedFlags = false;
      }
    for (auto I = MBB.rbegin(), E = MBB.rend(); I != E; ++I) {
      if (UnusedFlags)
        switch (I->getOpcode()) {
        case Z80old::LD8ri: // ld a, 0 -> xor a, a
          if (I->getOperand(0).getReg() == Z80old::A &&
              I->getOperand(1).getImm() == 0) {
            I->setDesc(TII->get(Z80old::XOR8ar));
            I->RemoveOperand(1);
            I->getOperand(0).setIsUse();
            I->addImplicitDefUseOperands(MF);
            for (auto &Op : I->uses()) {
              Op.setIsUndef();
            }
            FlagsZero = HalfCarry | Subtract | Carry;
            FlagsOne = 0;
            LLVM_DEBUG(dbgs() << '!');
            Changed = true;
          }
          break;
#if 0
        case Z80old::LD24ri: // ld hl, -1/0 -> set-cf \ sbc hl, hl
          if (I->getOperand(0).getReg() == Z80old::UHL &&
              I->getOperand(1).isImm()) {
            int Imm = I->getOperand(1).getImm();
            if (Imm == 0 || Imm == -1) {
              while (I->getNumOperands()) {
                I->RemoveOperand(0);
              }
              I->setDesc(TII->get(Z80old::SBC24aa));
              I->addImplicitDefUseOperands(MF);
              for (auto &Op : I->uses()) {
                Op.setIsUndef();
              }
              MachineInstrBuilder MIB =
                BuildMI(MBB, I.getReverse(), I->getDebugLoc(),
                        TII->get(Imm ? Z80old::SCF : Z80old::OR8ar));
              if (!Imm) {
                MIB.addReg(Z80old::A);
              }
              I = *MIB;
              for (auto &Op : I->uses()) {
                Op.setIsUndef();
              }
              FlagsZero = (Imm & (Sign | HalfCarry | Carry)) | (~Imm & Zero) |
                          ParityOverflow;
              FlagsOne = (~Imm & (Sign | HalfCarry | Carry)) | (Imm & Zero) |
                         Subtract;
              LLVM_DEBUG(dbgs() << '!');
              Changed = true;
            }
          }
          break;
        case Z80old::POP24r: // push reg \ pop hl -> rcf \ sbc hl,hl \ add hl,reg
          if (!OptSize && I->getOperand(0).getReg() == Z80old::UHL) {
            auto P = std::next(I);
            if (P != E && P->getOpcode() == Z80old::PUSH24r &&
                Z80old::O24RegClass.contains(P->getOperand(0).getReg())) {
              while (I->getNumOperands() != I->getNumExplicitOperands()) {
                I->RemoveOperand(I->getNumExplicitOperands());
              }
              I->setDesc(TII->get(Z80old::ADD24ao));
              I->addOperand(MachineOperand::CreateReg(Z80old::UHL,
                                                      /*isDef*/false));
              I->addOperand(P->getOperand(0));
              I->addImplicitDefUseOperands(MF);
              I->findRegisterDefOperand(Z80old::F)->setIsDead();
              while (P->getNumOperands()) {
                P->RemoveOperand(0);
              }
              P->setDesc(TII->get(Z80old::SBC24aa));
              P->addImplicitDefUseOperands(MF);
              for (auto &Op : P->uses()) {
                Op.setIsUndef();
              }
              I = *BuildMI(MBB, P.getReverse(), P->getDebugLoc(),
                           TII->get(Z80old::OR8ar)).addReg(Z80old::A);
              for (auto &Op : I->uses()) {
                Op.setIsUndef();
              }
              FlagsZero = Sign | ParityOverflow | Subtract | Carry;
              FlagsOne = Zero;
              LLVM_DEBUG(dbgs() << '!');
              Changed = true;
            }
          }
          break;
#endif // 0
        default:
          computeKnownFlags(*I, FlagsZero, FlagsOne);
          break;
        }
      UnusedFlags |= I->definesRegister(Z80old::F);
      if (MachineOperand *FlagsUse = I->findRegisterUseOperand(Z80old::F)) {
        UnusedFlags &= FlagsUse->isUndef();
      }
      LLVM_DEBUG(dbgs() << (UnusedFlags ? "unused" : "used") << '\t'; I->dump());
    }
    LLVM_DEBUG(dbgs() << '\n');
  }
  return Changed;
}

void Z80oldMachineLateOptimization::
computeKnownFlags(MachineInstr &MI, APInt &KnownZero, APInt &KnownOne) const {
  switch (MI.getOpcode()) {
  default:
    if (MI.definesRegister(Z80old::F)) {
      KnownZero = KnownOne = 0;
      LLVM_DEBUG(dbgs() << '?');
    }
    break;
  case Z80old::RCF:
    KnownZero = HalfCarry | Subtract | Carry;
    KnownOne = 0;
    break;
  case Z80old::SCF:
    KnownZero = HalfCarry | Subtract;
    KnownOne = Carry;
    break;
  }
}
