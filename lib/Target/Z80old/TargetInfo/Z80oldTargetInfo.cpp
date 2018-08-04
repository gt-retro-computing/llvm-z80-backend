//===-- Z80oldTargetInfo.cpp - Z80old Target Implementation -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Z80old.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

Target &llvm::getTheZ80oldTarget() {
  static Target TheZ80oldTarget;
  return TheZ80oldTarget;
}

extern "C" void LLVMInitializeZ80oldTargetInfo() {
  RegisterTarget<Triple::z80old,
                 /*HasJIT=*/true> X(getTheZ80oldTarget(), "Z80old", "Tutorial Z80old (old)", "Z80old");
}

