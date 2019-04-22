//===----- I8080MachineLateOptimization.cpp - Optimize late machine inst -----===//
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

#include "I8080.h"
#include "I8080RegisterInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
using namespace llvm;

#define DEBUG_TYPE "z80-ml-opt"

namespace {
class I8080MachineLateOptimization : public MachineFunctionPass {
public:
  I8080MachineLateOptimization() : MachineFunctionPass(ID) {}
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
    return "I8080 Machine Late Optimization";
  }

  static const uint8_t Carry, Subtract, ParityOverflow, HalfCarry, Zero, Sign;
  static char ID;
};

const uint8_t I8080MachineLateOptimization::Carry = 1 << 0;
const uint8_t I8080MachineLateOptimization::Subtract = 1 << 0;
const uint8_t I8080MachineLateOptimization::ParityOverflow = 1 << 0;
const uint8_t I8080MachineLateOptimization::HalfCarry = 1 << 0;
const uint8_t I8080MachineLateOptimization::Zero = 1 << 0;
const uint8_t I8080MachineLateOptimization::Sign = 1 << 0;

char I8080MachineLateOptimization::ID = 0;
} // end anonymous namespace

FunctionPass *llvm::createI8080MachineLateOptimization() {
  return new I8080MachineLateOptimization();
}

bool I8080MachineLateOptimization::runOnMachineFunction(MachineFunction &MF) {
  const TargetSubtargetInfo &STI = MF.getSubtarget();
  assert(MF.getRegInfo().tracksLiveness() && "Liveness not being tracked!");
  const TargetInstrInfo *TII = STI.getInstrInfo();
  bool Changed = false;
  if (!MF.getRegInfo().isPhysRegModified(I8080::F)) {
    return Changed;
  }
  //bool OptSize = MF.getFunction().getAttributes()
  //  .hasAttribute(AttributeList::FunctionIndex, Attribute::OptimizeForSize);
  APInt FlagsZero(8, 0), FlagsOne(8, 0);
  for (auto &MBB : MF) {
    bool UnusedFlags = true;
    for (MachineBasicBlock *Successor : MBB.successors())
      if (Successor->isLiveIn(I8080::F)) {
        UnusedFlags = false;
      }
    for (auto I = MBB.rbegin(), E = MBB.rend(); I != E; ++I) {
      if (UnusedFlags)
        switch (I->getOpcode()) {
        case I8080::LD8ri: // ld a, 0 -> xor a, a
          if (I->getOperand(0).getReg() == I8080::A &&
              I->getOperand(1).getImm() == 0) {
            I->setDesc(TII->get(I8080::XOR8ar));
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
        case I8080::LD24ri: // ld hl, -1/0 -> set-cf \ sbc hl, hl
          if (I->getOperand(0).getReg() == I8080::UHL &&
              I->getOperand(1).isImm()) {
            int Imm = I->getOperand(1).getImm();
            if (Imm == 0 || Imm == -1) {
              while (I->getNumOperands()) {
                I->RemoveOperand(0);
              }
              I->setDesc(TII->get(I8080::SBC24aa));
              I->addImplicitDefUseOperands(MF);
              for (auto &Op : I->uses()) {
                Op.setIsUndef();
              }
              MachineInstrBuilder MIB =
                BuildMI(MBB, I.getReverse(), I->getDebugLoc(),
                        TII->get(Imm ? I8080::SCF : I8080::OR8ar));
              if (!Imm) {
                MIB.addReg(I8080::A);
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
        case I8080::POP24r: // push reg \ pop hl -> rcf \ sbc hl,hl \ add hl,reg
          if (!OptSize && I->getOperand(0).getReg() == I8080::UHL) {
            auto P = std::next(I);
            if (P != E && P->getOpcode() == I8080::PUSH24r &&
                I8080::O24RegClass.contains(P->getOperand(0).getReg())) {
              while (I->getNumOperands() != I->getNumExplicitOperands()) {
                I->RemoveOperand(I->getNumExplicitOperands());
              }
              I->setDesc(TII->get(I8080::ADD24ao));
              I->addOperand(MachineOperand::CreateReg(I8080::UHL,
                                                      /*isDef*/false));
              I->addOperand(P->getOperand(0));
              I->addImplicitDefUseOperands(MF);
              I->findRegisterDefOperand(I8080::F)->setIsDead();
              while (P->getNumOperands()) {
                P->RemoveOperand(0);
              }
              P->setDesc(TII->get(I8080::SBC24aa));
              P->addImplicitDefUseOperands(MF);
              for (auto &Op : P->uses()) {
                Op.setIsUndef();
              }
              I = *BuildMI(MBB, P.getReverse(), P->getDebugLoc(),
                           TII->get(I8080::OR8ar)).addReg(I8080::A);
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
      UnusedFlags |= I->definesRegister(I8080::F);
      if (MachineOperand *FlagsUse = I->findRegisterUseOperand(I8080::F)) {
        UnusedFlags &= FlagsUse->isUndef();
      }
      LLVM_DEBUG(dbgs() << (UnusedFlags ? "unused" : "used") << '\t'; I->dump());
    }
    LLVM_DEBUG(dbgs() << '\n');
  }
  return Changed;
}

void I8080MachineLateOptimization::
computeKnownFlags(MachineInstr &MI, APInt &KnownZero, APInt &KnownOne) const {
  switch (MI.getOpcode()) {
  default:
    if (MI.definesRegister(I8080::F)) {
      KnownZero = KnownOne = 0;
      LLVM_DEBUG(dbgs() << '?');
    }
    break;
  case I8080::RCF:
    KnownZero = HalfCarry | Subtract | Carry;
    KnownOne = 0;
    break;
  case I8080::SCF:
    KnownZero = HalfCarry | Subtract;
    KnownOne = Carry;
    break;
  }
}
