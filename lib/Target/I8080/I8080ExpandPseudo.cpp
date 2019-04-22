//===------- I8080ExpandPseudo.cpp - Expand pseudo instructions -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that expands pseudo instructions into target
// instructions to allow proper scheduling, if-conversion, other late
// optimizations, or simply the encoding of the instructions.
//
//===----------------------------------------------------------------------===//

#include "I8080.h"
#include "I8080InstrInfo.h"
#include "I8080Subtarget.h"
#include "../../CodeGen/LiveDebugVariables.h"
#include "../../CodeGen/SpillPlacement.h"
#include "llvm/CodeGen/CalcSpillWeights.h"
#include "llvm/CodeGen/EdgeBundles.h"
#include "llvm/CodeGen/LiveStacks.h"
#include "llvm/CodeGen/LiveIntervals.h"
#include "llvm/CodeGen/LiveRangeEdit.h"
#include "llvm/CodeGen/LiveRegMatrix.h"
//#include "llvm/CodeGen/LiveStackAnalysis.h"
#include "llvm/CodeGen/MachineBlockFrequencyInfo.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegAllocRegistry.h"
#include "llvm/CodeGen/VirtRegMap.h"
using namespace llvm;

#define DEBUG_TYPE "z80-pseudo"

namespace {
class I8080ExpandPseudo : public MachineFunctionPass {
public:
  I8080ExpandPseudo() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties()
           .set(MachineFunctionProperties::Property::TracksLiveness);
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MachineBlockFrequencyInfo>();
    AU.addPreserved<MachineBlockFrequencyInfo>();
    //AU.addRequired<AAResultsWrapperPass>();
    //AU.addPreserved<AAResultsWrapperPass>();
    AU.addRequired<LiveIntervals>();
    AU.addPreserved<LiveIntervals>();
    AU.addRequired<SlotIndexes>();
    AU.addPreserved<SlotIndexes>();
    //AU.addRequired<LiveDebugVariables>();
    //AU.addPreserved<LiveDebugVariables>();
    AU.addRequired<LiveStacks>();
    AU.addPreserved<LiveStacks>();
    AU.addRequired<MachineDominatorTree>();
    AU.addPreserved<MachineDominatorTree>();
    AU.addRequired<MachineLoopInfo>();
    AU.addPreserved<MachineLoopInfo>();
    AU.addRequired<VirtRegMap>();
    AU.addPreserved<VirtRegMap>();
    AU.addRequired<LiveRegMatrix>();
    AU.addPreserved<LiveRegMatrix>();
    AU.addRequired<EdgeBundles>();
    AU.addRequired<SpillPlacement>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  StringRef getPassName() const override {
    return "I8080 Expand Pseudo Instructions";
  }

private:
  void ExpandCmp(MachineInstr &MI, MachineBasicBlock &MBB);
  void ExpandCmp0(MachineInstr &MI, MachineBasicBlock &MBB);
  bool ExpandMI(MachineBasicBlock::iterator &MI, MachineBasicBlock &MBB);
  bool ExpandMBB(MachineBasicBlock &MBB);

  const TargetInstrInfo *TII;
  static char ID;
};

char I8080ExpandPseudo::ID = 0;
} // end anonymous namespace

FunctionPass *llvm::createI8080ExpandPseudoPass() {
  return new I8080ExpandPseudo();
}

void I8080ExpandPseudo::ExpandCmp(MachineInstr &MI, MachineBasicBlock &MBB) {
  bool Is24Bit = false; // MI.getOpcode() == I8080::CP24ao;
  assert((Is24Bit || MI.getOpcode() == I8080::CP16ao) && "Unexpected opcode");
  DebugLoc DL = MI.getDebugLoc();
  dbgs() << "I8080ExpandPseudo::ExpandCmp";
  MI.dump();
  //BuildMI(MBB, MI, DL, TII->get(I8080::RCF));
  //BuildMI(MBB, MI, DL, TII->get(Is24Bit ? I8080::SBC24ar : I8080::SBC16ar))
  //  .addReg(MI.getOperand(0).getReg());
  //BuildMI(MBB, MI, DL, TII->get(Is24Bit ? I8080::ADD24ao : I8080::ADD16ao),
  //        Is24Bit ? I8080::UHL : I8080::HL).addReg(Is24Bit ? I8080::UHL : I8080::HL)
  //  .addReg(MI.getOperand(0).getReg());
}
void I8080ExpandPseudo::ExpandCmp0(MachineInstr &MI, MachineBasicBlock &MBB) {
  bool Is24Bit = false; // MI.getOpcode() == I8080::CP24a0;
  assert((Is24Bit || MI.getOpcode() == I8080::CP16a0) && "Unexpected opcode");
  DebugLoc DL = MI.getDebugLoc();
  dbgs() << "I8080ExpandPseudo::ExpandCmp";
  MI.dump();
  //BuildMI(MBB, MI, DL, TII->get(Is24Bit ? I8080::ADD24ao : I8080::ADD16ao),
  //        Is24Bit ? I8080::UHL : I8080::HL).addReg(Is24Bit ? I8080::UHL : I8080::HL)
  //  .addReg(MI.getOperand(0).getReg());
  //BuildMI(MBB, MI, DL, TII->get(I8080::RCF));
  //BuildMI(MBB, MI, DL, TII->get(Is24Bit ? I8080::SBC24ar : I8080::SBC16ar))
  //  .addReg(MI.getOperand(0).getReg());
}

bool I8080ExpandPseudo::ExpandMI(MachineBasicBlock::iterator &MI,
                               MachineBasicBlock &MBB) {
  switch (MI->getOpcode()) {
  default: return false;
  case I8080::CP16ao:
    //case I8080::CP24ao:
    ExpandCmp(*MI, MBB);
    break;
  case I8080::CP16a0:
    //case I8080::CP24a0:
    ExpandCmp0(*MI, MBB);
    break;
  }
  return false;
  MI = MBB.erase(MI);
  return true;
}

bool I8080ExpandPseudo::ExpandMBB(MachineBasicBlock &MBB) {
  bool Modified = false;
  for (auto I = MBB.begin(), E = MBB.end(); I != E; ++I) {
    Modified |= ExpandMI(I, MBB);
  }
  return Modified;
}

bool I8080ExpandPseudo::runOnMachineFunction(MachineFunction &MF) {
  getAnalysis<LiveIntervals>().addKillFlags(&getAnalysis<VirtRegMap>());
  TII = MF.getSubtarget().getInstrInfo();
  bool Modified = false;
  for (auto &MBB : MF) {
    Modified |= ExpandMBB(MBB);
  }
  return Modified;
}
