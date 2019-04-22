//===-- I8080MCAsmInfo.h - I8080 asm properties --------------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the I8080MCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_I8080_MCTARGETDESC_I8080MCASMINFO_H
#define LLVM_LIB_TARGET_I8080_MCTARGETDESC_I8080MCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class I8080MCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit I8080MCAsmInfo(const Triple &TheTriple);

};
} // End llvm namespace

#endif
