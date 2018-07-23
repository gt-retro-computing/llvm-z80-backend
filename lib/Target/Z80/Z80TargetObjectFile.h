//===-- llvm/Target/Z80TargetObjectFile.h - Z80 Object Info ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Z80_Z80TARGETOBJECTFILE_H
#define LLVM_LIB_TARGET_Z80_Z80TARGETOBJECTFILE_H

#include "Z80Config.h"

#include "Z80TargetMachine.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

namespace llvm {
class Z80TargetMachine;
class Z80TargetObjectFile : public TargetLoweringObjectFileELF {
  MCSection *SmallDataSection;
  MCSection *SmallBSSSection;
  const Z80TargetMachine *TM;
public:

  void Initialize(MCContext &Ctx, const TargetMachine &TM) override;

};
} // end namespace llvm

#endif

