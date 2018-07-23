//===-- Z80TargetInfo.cpp - Z80 Target Implementation -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Z80.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

Target &llvm::getTheZ80Target() {
  static Target TheZ80Target;
  return TheZ80Target;
}

extern "C" void LLVMInitializeZ80TargetInfo() {
  RegisterTarget<Triple::z80,
                 /*HasJIT=*/true> X(getTheZ80Target(), "Z80", "Tutorial Z80", "Z80");
}

