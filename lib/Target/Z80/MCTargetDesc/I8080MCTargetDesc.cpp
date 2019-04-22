//===-- Z80MCTargetDesc.cpp - Z80 Target Descriptions -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides Z80 specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "Z80MCTargetDesc.h"
#include "Z80MCAsmInfo.h"
#include "I8080TargetStreamer.h"
#include "InstPrinter/Z80InstPrinter.h"
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
#include "Z80GenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "Z80GenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "Z80GenRegisterInfo.inc"

std::string Z80_MC::ParseZ80Triple(const Triple &TT) {
  std::string FS;

  FS = "+16bit-mode";

  return FS;
}

MCSubtargetInfo *Z80_MC::createZ80MCSubtargetInfo(const Triple &TT,
                                                  StringRef CPU, StringRef FS) {
  std::string ArchFS = Z80_MC::ParseZ80Triple(TT);
  if (!FS.empty()) {
    if (!ArchFS.empty()) {
      ArchFS = (Twine(ArchFS) + "," + FS).str();
    } else {
      ArchFS = FS;
    }
  }

  return createZ80MCSubtargetInfoImpl(TT, CPU, ArchFS);
}

static MCInstrInfo *createZ80MCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitZ80MCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createZ80MCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitZ80MCRegisterInfo(X, Z80::PC);
  return X;
}

static MCAsmInfo *createZ80MCAsmInfo(const MCRegisterInfo &MRI,
                                     const Triple &TheTriple) {
  MCAsmInfo *MAI = new Z80MCAsmInfo(TheTriple);

  // Initialize initial frame state.
  // Calculate amount of bytes used for return address storing
  int stackGrowth = -2;

  unsigned StackPtr = 20; // Z80::SPS; $TODO / remove $HACK
  MCCFIInstruction Inst = MCCFIInstruction::createDefCfa(
                            nullptr, MRI.getDwarfRegNum(StackPtr, true), -stackGrowth);
  MAI->addInitialFrameState(Inst);

  // Add return address to move list
  unsigned InstPtr = 12; // Z80::IX;
  MCCFIInstruction Inst2 = MCCFIInstruction::createOffset(
                             nullptr, MRI.getDwarfRegNum(InstPtr, true), stackGrowth);
  MAI->addInitialFrameState(Inst2);

  return MAI;
}

static MCInstPrinter *createZ80MCInstPrinter(const Triple &TT,
                                             unsigned SyntaxVariant,
                                             const MCAsmInfo &MAI,
                                             const MCInstrInfo &MII,
                                             const MCRegisterInfo &MRI) {
  switch (SyntaxVariant) {
  default: return nullptr;
  case 0: return new Z80InstPrinter(MAI, MII, MRI);
    //case 1: return new Z80EInstPrinter(MAI, MII, MRI);
  }
}

static MCTargetStreamer *createAsmTargetStreamer(MCStreamer &S,
                                                 formatted_raw_ostream &OS,
                                                 MCInstPrinter * /*InstPrint*/,
                                                 bool /*isVerboseAsm*/) {
  return new Z80TargetAsmStreamer(S, OS);
}

extern "C" void LLVMInitializeZ80TargetMC() {
  for (Target *T : {&getTheZ80Target()/*, &getTheEZ80Target()*/}) {
    // Register the MC asm info.
    RegisterMCAsmInfoFn X(*T, createZ80MCAsmInfo);

    // Register the MC instruction info.
    TargetRegistry::RegisterMCInstrInfo(*T, createZ80MCInstrInfo);

    // Register the MC register info.
    TargetRegistry::RegisterMCRegInfo(*T, createZ80MCRegisterInfo);

    // Register the MC subtarget info.
    TargetRegistry::RegisterMCSubtargetInfo(*T,
                                            Z80_MC::createZ80MCSubtargetInfo);

    // Register the MCInstPrinter.
    TargetRegistry::RegisterMCInstPrinter(*T, createZ80MCInstPrinter);

    // Register the asm target streamer.
    TargetRegistry::RegisterAsmTargetStreamer(*T, createAsmTargetStreamer);
  }

  //TargetRegistry::RegisterMCAsmBackend(getTheZ80Target(), createZ80AsmBackend);
#if 0
  TargetRegistry::RegisterMCAsmBackend(getTheEZ80Target(),
                                       createEZ80AsmBackend);
#endif // 0
}

unsigned llvm::getZ80SuperRegisterOrZero(unsigned Reg) {
  switch (Reg) {
  default: return Z80::NoRegister;
  case Z80::A:
    return Z80::AF;
  case Z80::H: case Z80::L:
    return Z80::HL;
  case Z80::D: case Z80::E:
    return Z80::DE;
  case Z80::B: case Z80::C:
    return Z80::BC;
    //case Z80::IXH: case Z80::IXL:
    //  return Z80::IX;
    //case Z80::IYH: case Z80::IYL:
    //  return Z80::IY;
  }
}
