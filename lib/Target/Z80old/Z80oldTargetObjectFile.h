//===-- llvm/Target/Z80oldTargetObjectFile.h - Z80old Object Info ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Z80old_Z80oldTARGETOBJECTFILE_H
#define LLVM_LIB_TARGET_Z80old_Z80oldTARGETOBJECTFILE_H

#include "Z80oldConfig.h"

#include "Z80oldTargetMachine.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

namespace llvm {
class Z80oldTargetMachine;
class Z80oldTargetObjectFile : public TargetLoweringObjectFileELF {
  MCSection *SmallDataSection;
  MCSection *SmallBSSSection;
  const Z80oldTargetMachine *TM;
public:

  void Initialize(MCContext &Ctx, const TargetMachine &TM) override;

};
} // end namespace llvm

#endif

