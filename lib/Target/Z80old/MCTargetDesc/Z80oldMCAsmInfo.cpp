//===-- Z80oldMCAsmInfo.cpp - Z80old asm properties -----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of the Z80oldMCAsmInfo properties.
//
//===----------------------------------------------------------------------===//

#include "Z80oldMCAsmInfo.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Triple.h"
using namespace llvm;

void Z80oldMCAsmInfo::anchor() { }

Z80oldMCAsmInfo::Z80oldMCAsmInfo(const Triple &T) {
  //bool Is16Bit = T.isArch16Bit() || T.getEnvironment() == Triple::CODE16;
  CodePointerSize = CalleeSaveStackSlotSize = 2; // Is16Bit ? 2 : 3;
  MaxInstLength = 6;
  DollarIsPC = true;
  SeparatorString = nullptr;
  CommentString = ";";
  PrivateGlobalPrefix = PrivateLabelPrefix = "";
  Code16Directive = ".assume\tadl = 0";
  //Code24Directive = ".assume\tadl = 1";
  Code32Directive = Code64Directive = nullptr;
  AssemblerDialect = 0; //!Is16Bit;
  SupportsQuotedNames = false;
  ZeroDirective = AsciiDirective = AscizDirective = nullptr;
  Data8bitsDirective = "\tDB\t";
  Data16bitsDirective = "\tDW\t";
  //Data24bitsDirective = "\tDW24\t";
  Data32bitsDirective = "\tDL\t";
  Data64bitsDirective = nullptr;
  //AssignmentDirective = " EQU ";
  GlobalDirective = "\tXDEF\t";
  HasFunctionAlignment = false;
  HasDotTypeDotSizeDirective = false;
  WeakDirective = nullptr;
  UseIntegratedAssembler = false;
  WeakDirective = nullptr;
  UseLogicalShr = false;
}

#if 0
const char *Z80oldMCAsmInfo::getBlockDirective(int64_t Size) const {
  switch (Size) {
  default: return nullptr;
  case 1: return "\tBLKB\t";
  case 2: return "\tBLKW\t";
  case 3: return "\tBLKP\t";
  case 4: return "\tBLKL\t";
  }
}
#endif // 0
