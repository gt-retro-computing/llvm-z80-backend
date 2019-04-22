//===-- I8080SelectionDAGInfo.h - I8080 SelectionDAG Info -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the I8080 subclass for SelectionDAGTargetInfo.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_I8080_I8080SELECTIONDAGINFO_H
#define LLVM_LIB_TARGET_I8080_I8080SELECTIONDAGINFO_H

#include "llvm/CodeGen/SelectionDAGTargetInfo.h"

namespace llvm {

class I8080SelectionDAGInfo : public SelectionDAGTargetInfo {
public:
  explicit I8080SelectionDAGInfo() = default;
};

}

#endif // LLVM_LIB_TARGET_I8080_I8080SELECTIONDAGINFO_H
