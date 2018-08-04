//===-- Z80old.td - Describe the Z80old Target Machine ---------*- tablegen -*-===//
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
//include "Z80oldSchedule.td"
include "Z80oldInstrInfo.td"

def Z80oldInstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// Z80old Subtarget state
//===----------------------------------------------------------------------===//
def Mode16Bit : SubtargetFeature<"16bit-mode", "In16BitMode", "true",
                                  "16-bit mode">;

//===----------------------------------------------------------------------===//
// Z80old Subtarget features
//===----------------------------------------------------------------------===// 
def FeatureUndoc   : SubtargetFeature<"undoc", "HasUndocOps", "true",
                                      "Enable undocumented z80old instructions">;
//def FeatureZ180    : SubtargetFeature<"z180", "HasZ180Ops", "true",
//                                      "Support z180 instructions">;
//def FeatureEZ80old    : SubtargetFeature<"ez80old", "HasEZ80oldOps", "true",
//                                      "Support ez80old instructions">;
def FeatureIdxHalf : SubtargetFeature<"idxhalf", "HasIdxHalfRegs", "true",
                                      "Support index half registers">;

//===----------------------------------------------------------------------===//
// Z80old processors supported.
//===----------------------------------------------------------------------===//


let CompleteModel = 0 in def GenericModel : SchedMachineModel;

class Proc<string Name, list<SubtargetFeature> Features>
 : ProcessorModel<Name, GenericModel, Features>;

def : Proc<"generic",     []>; 
def : Proc<"z80old",         [FeatureUndoc, FeatureIdxHalf]>;

//===----------------------------------------------------------------------===//
// Calling Conventions
//===----------------------------------------------------------------------===//

include "Z80oldCallingConv.td" 

//===----------------------------------------------------------------------===//
// Assembly writer
//===----------------------------------------------------------------------===//

def Z80oldAsmWriter : AsmWriter;

// Will generate Z80oldGenAsmWrite.inc included by Z80oldInstPrinter.cpp, contents 
//  as follows,
// void Z80oldInstPrinter::printInstruction(const MCInst *MI, raw_ostream &O) {...}
// const char *Z80oldInstPrinter::getRegisterName(unsigned RegNo) {...}
def Z80old : Target {
// def Z80oldInstrInfo : InstrInfo as before.
  let InstructionSet = Z80oldInstrInfo;
  let AssemblyWriters = [Z80oldAsmWriter];
}
