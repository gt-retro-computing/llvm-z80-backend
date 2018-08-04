//===-- Z80oldSubtarget.cpp - Z80old Subtarget Information ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Z80old specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "Z80oldSubtarget.h"
#include "MCTargetDesc/Z80oldMCTargetDesc.h"
//#include "Z80oldFrameLowering.h"
using namespace llvm;

#define DEBUG_TYPE "z80old-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "Z80oldGenSubtargetInfo.inc"

Z80oldSubtarget &Z80oldSubtarget::initializeSubtargetDependencies(StringRef CPU,
                                                            StringRef FS) {
  ParseSubtargetFeatures(CPU, FS);
  HasIdxHalfRegs = HasUndocOps;
  return *this;
}

Z80oldSubtarget::Z80oldSubtarget(const Triple &TT, StringRef CPU, StringRef FS,
                           const Z80oldTargetMachine &TM)
  : Z80oldGenSubtargetInfo(TT, CPU, FS)
    //, In24BitMode(TT.getArch() == Triple::ez80old)
  , In16BitMode(TT.getArch() == Triple::z80old)
  , HasIdxHalfRegs(false)
  , HasUndocOps(false)
  , InstrInfo(initializeSubtargetDependencies(CPU, FS))
  , TLInfo(TM, *this)
  , FrameLowering(*this) {
}
