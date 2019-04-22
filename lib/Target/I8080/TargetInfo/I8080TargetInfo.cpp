//===-- I8080TargetInfo.cpp - I8080 Target Implementation -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "I8080.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

Target &llvm::getTheI8080Target() {
  static Target TheI8080Target;
  return TheI8080Target;
}

extern "C" void LLVMInitializeI8080TargetInfo() {
  RegisterTarget<Triple::i8080,
                 /*HasJIT=*/true> X(getTheI8080Target(), "I8080", "Intel 8080", "I8080");
}

