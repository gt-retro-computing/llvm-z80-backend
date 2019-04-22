//===-- llvm/Target/I8080TargetObjectFile.h - I8080 Object Info ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_I8080_I8080TARGETOBJECTFILE_H
#define LLVM_LIB_TARGET_I8080_I8080TARGETOBJECTFILE_H

#include "I8080Config.h"

#include "I8080TargetMachine.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

namespace llvm {
class I8080TargetMachine;
class I8080TargetObjectFile : public TargetLoweringObjectFileELF {
  MCSection *SmallDataSection;
  MCSection *SmallBSSSection;
  const I8080TargetMachine *TM;
public:

  void Initialize(MCContext &Ctx, const TargetMachine &TM) override;

};
} // end namespace llvm

#endif

