//===-- Z80oldMCTargetDesc.cpp - Z80old Target Descriptions -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides Z80old specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "Z80oldMCTargetDesc.h"
#include "Z80oldMCAsmInfo.h"
#include "Z80oldTargetStreamer.h"
#include "InstPrinter/Z80oldInstPrinter.h"
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
#include "Z80oldGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "Z80oldGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "Z80oldGenRegisterInfo.inc"

std::string Z80old_MC::ParseZ80oldTriple(const Triple &TT) {
  std::string FS;

  FS = "+16bit-mode";

  return FS;
}

MCSubtargetInfo *Z80old_MC::createZ80oldMCSubtargetInfo(const Triple &TT,
                                                  StringRef CPU, StringRef FS) {
  std::string ArchFS = Z80old_MC::ParseZ80oldTriple(TT);
  if (!FS.empty()) {
    if (!ArchFS.empty()) {
      ArchFS = (Twine(ArchFS) + "," + FS).str();
    } else {
      ArchFS = FS;
    }
  }

  return createZ80oldMCSubtargetInfoImpl(TT, CPU, ArchFS);
}

static MCInstrInfo *createZ80oldMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitZ80oldMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createZ80oldMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitZ80oldMCRegisterInfo(X, Z80old::PC);
  return X;
}

static MCAsmInfo *createZ80oldMCAsmInfo(const MCRegisterInfo &MRI,
                                     const Triple &TheTriple) {
  MCAsmInfo *MAI = new Z80oldMCAsmInfo(TheTriple);

  // Initialize initial frame state.
  // Calculate amount of bytes used for return address storing
  int stackGrowth = -2;

  unsigned StackPtr = 20; // Z80old::SPS; $TODO / remove $HACK
  MCCFIInstruction Inst = MCCFIInstruction::createDefCfa(
                            nullptr, MRI.getDwarfRegNum(StackPtr, true), -stackGrowth);
  MAI->addInitialFrameState(Inst);

  // Add return address to move list
  unsigned InstPtr = 12; // Z80old::IX;
  MCCFIInstruction Inst2 = MCCFIInstruction::createOffset(
                             nullptr, MRI.getDwarfRegNum(InstPtr, true), stackGrowth);
  MAI->addInitialFrameState(Inst2);

  return MAI;
}

static MCInstPrinter *createZ80oldMCInstPrinter(const Triple &TT,
                                             unsigned SyntaxVariant,
                                             const MCAsmInfo &MAI,
                                             const MCInstrInfo &MII,
                                             const MCRegisterInfo &MRI) {
  switch (SyntaxVariant) {
  default: return nullptr;
  case 0: return new Z80oldInstPrinter(MAI, MII, MRI);
    //case 1: return new Z80oldEInstPrinter(MAI, MII, MRI);
  }
}

static MCTargetStreamer *createAsmTargetStreamer(MCStreamer &S,
                                                 formatted_raw_ostream &OS,
                                                 MCInstPrinter * /*InstPrint*/,
                                                 bool /*isVerboseAsm*/) {
  return new Z80oldTargetAsmStreamer(S, OS);
}

extern "C" void LLVMInitializeZ80oldTargetMC() {
  for (Target *T : {&getTheZ80oldTarget()/*, &getTheEZ80oldTarget()*/}) {
    // Register the MC asm info.
    RegisterMCAsmInfoFn X(*T, createZ80oldMCAsmInfo);

    // Register the MC instruction info.
    TargetRegistry::RegisterMCInstrInfo(*T, createZ80oldMCInstrInfo);

    // Register the MC register info.
    TargetRegistry::RegisterMCRegInfo(*T, createZ80oldMCRegisterInfo);

    // Register the MC subtarget info.
    TargetRegistry::RegisterMCSubtargetInfo(*T,
                                            Z80old_MC::createZ80oldMCSubtargetInfo);

    // Register the MCInstPrinter.
    TargetRegistry::RegisterMCInstPrinter(*T, createZ80oldMCInstPrinter);

    // Register the asm target streamer.
    TargetRegistry::RegisterAsmTargetStreamer(*T, createAsmTargetStreamer);
  }

  //TargetRegistry::RegisterMCAsmBackend(getTheZ80oldTarget(), createZ80oldAsmBackend);
#if 0
  TargetRegistry::RegisterMCAsmBackend(getTheEZ80oldTarget(),
                                       createEZ80oldAsmBackend);
#endif // 0
}

unsigned llvm::getZ80oldSuperRegisterOrZero(unsigned Reg) {
  switch (Reg) {
  default: return Z80old::NoRegister;
  case Z80old::A:
    return Z80old::AF;
  case Z80old::H: case Z80old::L:
    return Z80old::HL;
  case Z80old::D: case Z80old::E:
    return Z80old::DE;
  case Z80old::B: case Z80old::C:
    return Z80old::BC;
    //case Z80old::IXH: case Z80old::IXL:
    //  return Z80old::IX;
    //case Z80old::IYH: case Z80old::IYL:
    //  return Z80old::IY;
  }
}
