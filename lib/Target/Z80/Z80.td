//===-- Z80.td - Describe the Z80 Target Machine ---------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Calling Conv, Instruction Descriptions
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"
//include "Z80Schedule.td"
include "Z80InstrInfo.td"

def Z80InstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// Z80 Subtarget state
//===----------------------------------------------------------------------===//
def Mode16Bit : SubtargetFeature<"16bit-mode", "In16BitMode", "true",
                                  "16-bit mode">;

//===----------------------------------------------------------------------===//
// Z80 Subtarget features
//===----------------------------------------------------------------------===// 
def FeatureUndoc   : SubtargetFeature<"undoc", "HasUndocOps", "true",
                                      "Enable undocumented z80 instructions">;
//def FeatureZ180    : SubtargetFeature<"z180", "HasZ180Ops", "true",
//                                      "Support z180 instructions">;
//def FeatureEZ80    : SubtargetFeature<"ez80", "HasEZ80Ops", "true",
//                                      "Support ez80 instructions">;
def FeatureIdxHalf : SubtargetFeature<"idxhalf", "HasIdxHalfRegs", "true",
                                      "Support index half registers">;

//===----------------------------------------------------------------------===//
// Z80 processors supported.
//===----------------------------------------------------------------------===//


let CompleteModel = 0 in def GenericModel : SchedMachineModel;

class Proc<string Name, list<SubtargetFeature> Features>
 : ProcessorModel<Name, GenericModel, Features>;

def : Proc<"generic",     []>; 
def : Proc<"z80",         [FeatureUndoc, FeatureIdxHalf]>;

//===----------------------------------------------------------------------===//
// Calling Conventions
//===----------------------------------------------------------------------===//

include "Z80CallingConv.td" 

//===----------------------------------------------------------------------===//
// Assembly writer
//===----------------------------------------------------------------------===//

def Z80AsmWriter : AsmWriter;

// Will generate Z80GenAsmWrite.inc included by Z80InstPrinter.cpp, contents 
//  as follows,
// void Z80InstPrinter::printInstruction(const MCInst *MI, raw_ostream &O) {...}
// const char *Z80InstPrinter::getRegisterName(unsigned RegNo) {...}
def Z80 : Target {
// def Z80InstrInfo : InstrInfo as before.
  let InstructionSet = Z80InstrInfo;
  let AssemblyWriters = [Z80AsmWriter];
}
