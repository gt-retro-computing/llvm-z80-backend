//===-- I8080MCTargetDesc.cpp - I8080 Target Descriptions -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides I8080 specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "I8080MCTargetDesc.h"
#include "I8080MCAsmInfo.h"
#include "I8080TargetStreamer.h"
#include "InstPrinter/I8080InstPrinter.h"
#include "llvm/MC/MachineLocation.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "I8080GenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "I8080GenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "I8080GenRegisterInfo.inc"

std::string I8080_MC::ParseI8080Triple(const Triple &TT) {
  std::string FS;

  FS = "+16bit-mode";

  return FS;
}

MCSubtargetInfo *I8080_MC::createI8080MCSubtargetInfo(const Triple &TT,
                                                  StringRef CPU, StringRef FS) {
  std::string ArchFS = I8080_MC::ParseI8080Triple(TT);
  if (!FS.empty()) {
    if (!ArchFS.empty()) {
      ArchFS = (Twine(ArchFS) + "," + FS).str();
    } else {
      ArchFS = FS;
    }
  }

  return createI8080MCSubtargetInfoImpl(TT, CPU, ArchFS);
}

static MCInstrInfo *createI8080MCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitI8080MCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createI8080MCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitI8080MCRegisterInfo(X, I8080::PC);
  return X;
}

static MCAsmInfo *createI8080MCAsmInfo(const MCRegisterInfo &MRI,
                                     const Triple &TheTriple) {
  MCAsmInfo *MAI = new I8080MCAsmInfo(TheTriple);

  // Initialize initial frame state.
  // Calculate amount of bytes used for return address storing
  int stackGrowth = -2;

  unsigned StackPtr = 20; // I8080::SPS; $TODO / remove $HACK
  MCCFIInstruction Inst = MCCFIInstruction::createDefCfa(
                            nullptr, MRI.getDwarfRegNum(StackPtr, true), -stackGrowth);
  MAI->addInitialFrameState(Inst);

  // Add return address to move list
  unsigned InstPtr = 12; // I8080::IX;
  MCCFIInstruction Inst2 = MCCFIInstruction::createOffset(
                             nullptr, MRI.getDwarfRegNum(InstPtr, true), stackGrowth);
  MAI->addInitialFrameState(Inst2);

  return MAI;
}

static MCInstPrinter *createI8080MCInstPrinter(const Triple &TT,
                                             unsigned SyntaxVariant,
                                             const MCAsmInfo &MAI,
                                             const MCInstrInfo &MII,
                                             const MCRegisterInfo &MRI) {
  switch (SyntaxVariant) {
  default: return nullptr;
  case 0: return new I8080InstPrinter(MAI, MII, MRI);
    //case 1: return new I8080EInstPrinter(MAI, MII, MRI);
  }
}

static MCTargetStreamer *createAsmTargetStreamer(MCStreamer &S,
                                                 formatted_raw_ostream &OS,
                                                 MCInstPrinter * /*InstPrint*/,
                                                 bool /*isVerboseAsm*/) {
  return new I8080TargetAsmStreamer(S, OS);
}

extern "C" void LLVMInitializeI8080TargetMC() {
  for (Target *T : {&getTheI8080Target()/*, &getTheEI8080Target()*/}) {
    // Register the MC asm info.
    RegisterMCAsmInfoFn X(*T, createI8080MCAsmInfo);

    // Register the MC instruction info.
    TargetRegistry::RegisterMCInstrInfo(*T, createI8080MCInstrInfo);

    // Register the MC register info.
    TargetRegistry::RegisterMCRegInfo(*T, createI8080MCRegisterInfo);

    // Register the MC subtarget info.
    TargetRegistry::RegisterMCSubtargetInfo(*T,
                                            I8080_MC::createI8080MCSubtargetInfo);

    // Register the MCInstPrinter.
    TargetRegistry::RegisterMCInstPrinter(*T, createI8080MCInstPrinter);

    // Register the asm target streamer.
    TargetRegistry::RegisterAsmTargetStreamer(*T, createAsmTargetStreamer);
  }

  //TargetRegistry::RegisterMCAsmBackend(getTheI8080Target(), createI8080AsmBackend);
#if 0
  TargetRegistry::RegisterMCAsmBackend(getTheEI8080Target(),
                                       createEI8080AsmBackend);
#endif // 0
}

unsigned llvm::getI8080SuperRegisterOrZero(unsigned Reg) {
  switch (Reg) {
  default: return I8080::NoRegister;
  case I8080::A:
    return I8080::AF;
  case I8080::H: case I8080::L:
    return I8080::HL;
  case I8080::D: case I8080::E:
    return I8080::DE;
  case I8080::B: case I8080::C:
    return I8080::BC;
    //case I8080::IXH: case I8080::IXL:
    //  return I8080::IX;
    //case I8080::IYH: case I8080::IYL:
    //  return I8080::IY;
  }
}
