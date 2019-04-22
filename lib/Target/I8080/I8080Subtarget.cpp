//===-- I8080Subtarget.cpp - I8080 Subtarget Information ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the I8080 specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "I8080Subtarget.h"
#include "MCTargetDesc/I8080MCTargetDesc.h"
//#include "I8080FrameLowering.h"
using namespace llvm;

#define DEBUG_TYPE "z80-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "I8080GenSubtargetInfo.inc"

I8080Subtarget &I8080Subtarget::initializeSubtargetDependencies(StringRef CPU,
                                                            StringRef FS) {
  ParseSubtargetFeatures(CPU, FS);
  HasIdxHalfRegs = HasUndocOps;
  return *this;
}

I8080Subtarget::I8080Subtarget(const Triple &TT, StringRef CPU, StringRef FS,
                           const I8080TargetMachine &TM)
  : I8080GenSubtargetInfo(TT, CPU, FS)
    //, In24BitMode(TT.getArch() == Triple::ez80)
  , In16BitMode(TT.getArch() == Triple::i8080)
  , HasIdxHalfRegs(false)
  , HasUndocOps(false)
  , InstrInfo(initializeSubtargetDependencies(CPU, FS))
  , TLInfo(TM, *this)
  , FrameLowering(*this) {
}
