//===-- Z80oldMCTargetDesc.h - Z80old Target Descriptions -----------*- C++ -*-===//
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

#ifndef LLVM_LIB_TARGET_Z80old_MCTARGETDESC_Z80oldMCTARGETDESC_H
#define LLVM_LIB_TARGET_Z80old_MCTARGETDESC_Z80oldMCTARGETDESC_H

#include "Z80oldConfig.h"
#include "llvm/Support/DataTypes.h"
#include <memory>
#include <string>

namespace llvm {
class Target;
class Triple;
class MCCodeEmitter;
class MCSubtargetInfo;
class MCInstrInfo;
class MCRegisterInfo;
class MCContext;
class MCTargetOptions;
class MCAsmBackend;
//class MCObjectWriter;
class MCObjectTargetWriter;
class StringRef;
class raw_pwrite_stream;

Target &getTheZ80oldTarget();

namespace Z80old_MC {
std::string ParseZ80oldTriple(const Triple &TT);

/// Create a Z80old MCSubtargetInfo instance.  This is exposed so Asm parser, etc.
/// do not need to go through TargetRegistry.
MCSubtargetInfo *createZ80oldMCSubtargetInfo(const Triple &TT, StringRef CPU,
                                          StringRef FS);
}

#if 0
MCCodeEmitter *createZ80oldMCCodeEmitter(const MCInstrInfo &MCII,
                                      const MCRegisterInfo &MRI,
                                      MCContext &Ctx);

MCAsmBackend *createZ80oldAsmBackend(const Target &T,
                                  const MCSubtargetInfo &STI,
                                  const MCRegisterInfo &MRI,
                                  const MCTargetOptions &Options);
MCAsmBackend *createZ80oldAsmBackend(const Target &T, const MCRegisterInfo &MRI,
                                  const Triple &TT, StringRef CPU,
                                  const MCTargetOptions &Options);
MCAsmBackend *createEZ80oldAsmBackend(const Target &T, const MCRegisterInfo &MRI,
                                   const Triple &TT, StringRef CPU,
                                   const MCTargetOptions &Options);

/// Construct a Z80old OMF object writer.
std::unique_ptr<MCObjectWriter> createZ80oldOMFObjectWriter(raw_pwrite_stream &OS);
#endif // 0

/// Construct a Z80old ELF object writer.
std::unique_ptr<MCObjectTargetWriter> createZ80oldELFObjectWriter();

unsigned getZ80oldSuperRegisterOrZero(unsigned Reg);
} // End llvm namespace

// Defines symbolic names for Z80old registers.  This defines a mapping from
// register name to register number.
//
#define GET_REGINFO_ENUM
#include "Z80oldGenRegisterInfo.inc"

// Defines symbolic names for the Z80old instructions.
//
#define GET_INSTRINFO_ENUM
#include "Z80oldGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "Z80oldGenSubtargetInfo.inc"

#endif
