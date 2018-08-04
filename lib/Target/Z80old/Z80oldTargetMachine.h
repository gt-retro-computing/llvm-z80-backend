//===-- Z80oldTargetMachine.h - Define TargetMachine for Z80old -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the Z80old specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Z80old_Z80oldTARGETMACHINE_H
#define LLVM_LIB_TARGET_Z80old_Z80oldTARGETMACHINE_H

//#include "Z80oldConfig.h"
#include "Z80oldSubtarget.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Target/TargetMachine.h"
#include <memory>

namespace llvm {
class Z80oldTargetMachine final : public LLVMTargetMachine {
  //std::unique_ptr<TargetLoweringObjectFile> TLOF;
  //Z80oldSubtarget Subtarget;
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  mutable StringMap<std::unique_ptr<Z80oldSubtarget>> SubtargetMap;

public:
  Z80oldTargetMachine(const Target &T, const Triple &TT, StringRef CPU, StringRef FS,
                   const TargetOptions &Options, Optional<Reloc::Model> RM,
                   Optional<CodeModel::Model> CM, CodeGenOpt::Level OL, bool JIT);
  ~Z80oldTargetMachine() override;


  const Z80oldSubtarget *getSubtargetImpl(const Function &F) const override;
  // DO NOT IMPLEMENT: There is no such thing as a valid default subtarget,
  // subtargets are per-function entities based on the target-specific
  // attributes of each function.
  const Z80oldSubtarget *getSubtargetImpl() const = delete;

  //const Z80oldSubtarget *getSubtargetImpl( const Function& F ) const override {
  //	return &Subtarget;
  //}

  // Set up the pass pipeline.
  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
};
}

#endif

