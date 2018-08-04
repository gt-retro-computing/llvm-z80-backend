//===-- Z80oldSelectionDAGInfo.h - Z80old SelectionDAG Info -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the Z80old subclass for SelectionDAGTargetInfo.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Z80old_Z80oldSELECTIONDAGINFO_H
#define LLVM_LIB_TARGET_Z80old_Z80oldSELECTIONDAGINFO_H

#include "llvm/CodeGen/SelectionDAGTargetInfo.h"

namespace llvm {

class Z80oldSelectionDAGInfo : public SelectionDAGTargetInfo {
public:
  explicit Z80oldSelectionDAGInfo() = default;
};

}

#endif // LLVM_LIB_TARGET_Z80old_Z80oldSELECTIONDAGINFO_H
