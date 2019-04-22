//==-- I8080TargetStreamer.h - I8080 Target Streamer -----------------*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares I8080-specific target streamer classes.
/// These are for implementing support for target-specific assembly directives.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_I8080_MCTARGETDESC_I8080TARGETSTREAMER_H
#define LLVM_LIB_TARGET_I8080_MCTARGETDESC_I8080TARGETSTREAMER_H

#include "llvm/MC/MCStreamer.h"

namespace llvm {

class I8080TargetStreamer : public MCTargetStreamer {
public:
  explicit I8080TargetStreamer(MCStreamer &S);

  // .align
  virtual void emitAlign(unsigned ByteAlignment) = 0;

  // .block
  virtual void emitBlock(uint64_t NumBytes) = 0;

  // .global
  virtual void emitGlobal(MCSymbol *Symbol) = 0;

  // .extern
  virtual void emitExtern(MCSymbol *Symbol) = 0;
};

class I8080TargetAsmStreamer final : public I8080TargetStreamer {
  const MCAsmInfo *MAI;
  formatted_raw_ostream &OS;

public:
  I8080TargetAsmStreamer(MCStreamer &S, formatted_raw_ostream &OS);

  void emitAlign(unsigned ByteAlignment) override;
  void emitBlock(uint64_t NumBytes) override;
  void emitGlobal(MCSymbol *Symbol) override;
  void emitExtern(MCSymbol *Symbol) override;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_I8080_MCTARGETDESC_I8080TARGETSTREAMER_H
