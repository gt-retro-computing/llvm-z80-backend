//===-- Z80oldFixupKinds.h - Z80old Specific Fixup Entries ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Z80old_MCTARGETDESC_Z80oldFIXUPKINDS_H
#define LLVM_LIB_TARGET_Z80old_MCTARGETDESC_Z80oldFIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace Z80old {
enum Fixups {
  // Marker
  LastTargetFixupKind = FirstTargetFixupKind,
  NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};
}
}

#endif
