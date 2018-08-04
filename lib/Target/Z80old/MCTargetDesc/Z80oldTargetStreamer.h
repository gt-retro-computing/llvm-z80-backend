//==-- Z80oldTargetStreamer.h - Z80old Target Streamer -----------------*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares Z80old-specific target streamer classes.
/// These are for implementing support for target-specific assembly directives.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Z80old_MCTARGETDESC_Z80oldTARGETSTREAMER_H
#define LLVM_LIB_TARGET_Z80old_MCTARGETDESC_Z80oldTARGETSTREAMER_H

#include "llvm/MC/MCStreamer.h"

namespace llvm {

class Z80oldTargetStreamer : public MCTargetStreamer {
public:
  explicit Z80oldTargetStreamer(MCStreamer &S);

  // .align
  virtual void emitAlign(unsigned ByteAlignment) = 0;

  // .block
  virtual void emitBlock(uint64_t NumBytes) = 0;

  // .global
  virtual void emitGlobal(MCSymbol *Symbol) = 0;

  // .extern
  virtual void emitExtern(MCSymbol *Symbol) = 0;
};

class Z80oldTargetAsmStreamer final : public Z80oldTargetStreamer {
  const MCAsmInfo *MAI;
  formatted_raw_ostream &OS;

public:
  Z80oldTargetAsmStreamer(MCStreamer &S, formatted_raw_ostream &OS);

  void emitAlign(unsigned ByteAlignment) override;
  void emitBlock(uint64_t NumBytes) override;
  void emitGlobal(MCSymbol *Symbol) override;
  void emitExtern(MCSymbol *Symbol) override;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_Z80old_MCTARGETDESC_Z80oldTARGETSTREAMER_H
