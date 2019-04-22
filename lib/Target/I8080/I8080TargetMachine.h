//===-- I8080TargetMachine.h - Define TargetMachine for I8080 -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the I8080 specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_I8080_I8080TARGETMACHINE_H
#define LLVM_LIB_TARGET_I8080_I8080TARGETMACHINE_H

//#include "I8080Config.h"
#include "I8080Subtarget.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Target/TargetMachine.h"
#include <memory>

namespace llvm {
class I8080TargetMachine final : public LLVMTargetMachine {
  //std::unique_ptr<TargetLoweringObjectFile> TLOF;
  //I8080Subtarget Subtarget;
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  mutable StringMap<std::unique_ptr<I8080Subtarget>> SubtargetMap;

public:
  I8080TargetMachine(const Target &T, const Triple &TT, StringRef CPU, StringRef FS,
                   const TargetOptions &Options, Optional<Reloc::Model> RM,
                   Optional<CodeModel::Model> CM, CodeGenOpt::Level OL, bool JIT);
  ~I8080TargetMachine() override;


  const I8080Subtarget *getSubtargetImpl(const Function &F) const override;
  // DO NOT IMPLEMENT: There is no such thing as a valid default subtarget,
  // subtargets are per-function entities based on the target-specific
  // attributes of each function.
  const I8080Subtarget *getSubtargetImpl() const = delete;

  //const I8080Subtarget *getSubtargetImpl( const Function& F ) const override {
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

