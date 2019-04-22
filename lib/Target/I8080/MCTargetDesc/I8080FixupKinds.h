//===-- I8080FixupKinds.h - I8080 Specific Fixup Entries ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_I8080_MCTARGETDESC_I8080FIXUPKINDS_H
#define LLVM_LIB_TARGET_I8080_MCTARGETDESC_I8080FIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace I8080 {
enum Fixups {
  // Marker
  LastTargetFixupKind = FirstTargetFixupKind,
  NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};
}
}

#endif
