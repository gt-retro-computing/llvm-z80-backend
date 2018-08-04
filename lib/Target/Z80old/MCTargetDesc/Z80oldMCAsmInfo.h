//===-- Z80oldMCAsmInfo.h - Z80old asm properties --------------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the Z80oldMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Z80old_MCTARGETDESC_Z80oldMCASMINFO_H
#define LLVM_LIB_TARGET_Z80old_MCTARGETDESC_Z80oldMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class Z80oldMCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit Z80oldMCAsmInfo(const Triple &TheTriple);

};
} // End llvm namespace

#endif
