//===--- I8080.cpp - Implement I8080 target feature support -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements I8080 TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#include "I8080.h"
#include "llvm/ADT/StringSwitch.h"

using namespace clang;
using namespace clang::targets;

bool I8080TargetInfo::setCPU(const std::string &Name) {
  return llvm::StringSwitch<bool>(Name)
    .Case("generic", true)
    .Case("i8080",     true)
    .Default(false);
}

bool I8080TargetInfo::
initFeatureMap(llvm::StringMap<bool> &Features,
               DiagnosticsEngine &Diags, StringRef CPU,
               const std::vector<std::string> &FeaturesVec) const {
  if (CPU == "i8080")
    Features["undoc"] = true;
  return TargetInfo::initFeatureMap(Features, Diags, CPU, FeaturesVec);
}

ArrayRef<const char *> I8080TargetInfo::getGCCRegNames() const {
  static const char *const GCCRegNames[] = {
    "a", "f", "b", "c", "d", "e", "h", "l",
    "a'", "f'", "b'", "c'", "d'", "e'", "h'", "l'",
    "ixh", "ixl", "iyh", "iyl", "i", "r",
    "af", "bc", "de", "hl", "af'", "bc'", "de'", "hl'",
    "ix", "iy", "sp", "pc"
  };
  return GCCRegNames;
}

void I8080TargetInfo::getTargetDefines(const LangOptions &Opts,
                                     MacroBuilder &Builder) const {
  I8080TargetInfoBase::getTargetDefines(Opts, Builder);
  defineCPUMacros(Builder, "I8080", /*Tuning=*/false);
  if (getTargetOpts().CPU == "undoc")
    defineCPUMacros(Builder, "I8080_UNDOC", /*Tuning=*/false);
}
