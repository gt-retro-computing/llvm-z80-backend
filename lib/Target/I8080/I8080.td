//===-- I8080.td - Describe the I8080 Target Machine ---------*- tablegen -*-===//
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
//include "I8080Schedule.td"
include "I8080InstrInfo.td"

def I8080InstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// I8080 Subtarget state
//===----------------------------------------------------------------------===//
def Mode16Bit : SubtargetFeature<"16bit-mode", "In16BitMode", "true",
                                  "16-bit mode">;

//===----------------------------------------------------------------------===//
// I8080 Subtarget features
//===----------------------------------------------------------------------===// 
def FeatureUndoc   : SubtargetFeature<"undoc", "HasUndocOps", "true",
                                      "Enable undocumented i8080 instructions">;
//def FeatureZ180    : SubtargetFeature<"z180", "HasZ180Ops", "true",
//                                      "Support z180 instructions">;
//def FeatureEI8080    : SubtargetFeature<"ez80", "HasEI8080Ops", "true",
//                                      "Support ez80 instructions">;
def FeatureIdxHalf : SubtargetFeature<"idxhalf", "HasIdxHalfRegs", "true",
                                      "Support index half registers">;

//===----------------------------------------------------------------------===//
// I8080 processors supported.
//===----------------------------------------------------------------------===//


let CompleteModel = 0 in def GenericModel : SchedMachineModel;

class Proc<string Name, list<SubtargetFeature> Features>
 : ProcessorModel<Name, GenericModel, Features>;

def : Proc<"generic",     []>; 
def : Proc<"i8080",         [FeatureUndoc, FeatureIdxHalf]>;

//===----------------------------------------------------------------------===//
// Calling Conventions
//===----------------------------------------------------------------------===//

include "I8080CallingConv.td"

//===----------------------------------------------------------------------===//
// Assembly writer
//===----------------------------------------------------------------------===//

def I8080AsmWriter : AsmWriter;

// Will generate I8080GenAsmWrite.inc included by I8080InstPrinter.cpp, contents
//  as follows,
// void I8080InstPrinter::printInstruction(const MCInst *MI, raw_ostream &O) {...}
// const char *I8080InstPrinter::getRegisterName(unsigned RegNo) {...}
def I8080 : Target {
// def I8080InstrInfo : InstrInfo as before.
  let InstructionSet = I8080InstrInfo;
  let AssemblyWriters = [I8080AsmWriter];
}
