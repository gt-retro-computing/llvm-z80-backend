//===-- I8080MCTargetDesc.h - I8080 Target Descriptions -----------*- C++ -*-===//
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

#ifndef LLVM_LIB_TARGET_I8080_MCTARGETDESC_I8080MCTARGETDESC_H
#define LLVM_LIB_TARGET_I8080_MCTARGETDESC_I8080MCTARGETDESC_H

#include "I8080Config.h"
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

Target &getTheI8080Target();

namespace I8080_MC {
std::string ParseI8080Triple(const Triple &TT);

/// Create a I8080 MCSubtargetInfo instance.  This is exposed so Asm parser, etc.
/// do not need to go through TargetRegistry.
MCSubtargetInfo *createI8080MCSubtargetInfo(const Triple &TT, StringRef CPU,
                                          StringRef FS);
}

#if 0
MCCodeEmitter *createI8080MCCodeEmitter(const MCInstrInfo &MCII,
                                      const MCRegisterInfo &MRI,
                                      MCContext &Ctx);

MCAsmBackend *createI8080AsmBackend(const Target &T,
                                  const MCSubtargetInfo &STI,
                                  const MCRegisterInfo &MRI,
                                  const MCTargetOptions &Options);
MCAsmBackend *createI8080AsmBackend(const Target &T, const MCRegisterInfo &MRI,
                                  const Triple &TT, StringRef CPU,
                                  const MCTargetOptions &Options);
MCAsmBackend *createEI8080AsmBackend(const Target &T, const MCRegisterInfo &MRI,
                                   const Triple &TT, StringRef CPU,
                                   const MCTargetOptions &Options);

/// Construct a I8080 OMF object writer.
std::unique_ptr<MCObjectWriter> createI8080OMFObjectWriter(raw_pwrite_stream &OS);
#endif // 0

/// Construct a I8080 ELF object writer.
std::unique_ptr<MCObjectTargetWriter> createI8080ELFObjectWriter();

unsigned getI8080SuperRegisterOrZero(unsigned Reg);
} // End llvm namespace

// Defines symbolic names for I8080 registers.  This defines a mapping from
// register name to register number.
//
#define GET_REGINFO_ENUM
#include "I8080GenRegisterInfo.inc"

// Defines symbolic names for the I8080 instructions.
//
#define GET_INSTRINFO_ENUM
#include "I8080GenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "I8080GenSubtargetInfo.inc"

#endif
