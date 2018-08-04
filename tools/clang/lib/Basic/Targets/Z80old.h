//===--- Z80old.h - Declare Z80old target feature support -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares Z80old TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_Z80old_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_Z80old_H

#include "Targets.h"
#include "clang/Basic/TargetInfo.h"
#include "llvm/Support/Compiler.h"

namespace clang {
namespace targets {

class LLVM_LIBRARY_VISIBILITY Z80oldTargetInfoBase : public TargetInfo {
public:
  Z80oldTargetInfoBase(const llvm::Triple &Triple) : TargetInfo(Triple) {
    TLSSupported = false;
    PointerAlign = BoolAlign = IntAlign = HalfAlign = FloatAlign =
        DoubleAlign = LongDoubleAlign = LongAlign = LongLongAlign =
        SuitableAlign = MinGlobalAlign = 8;
    DefaultAlignForAttributeAligned = 32;
    SizeType = UnsignedInt;
    PtrDiffType = IntPtrType = SignedInt;
    Char32Type = UnsignedLong;
    UseBitFieldTypeAlignment = false;
  }
  ArrayRef<Builtin::Info> getTargetBuiltins() const final { return None; }
  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::CharPtrBuiltinVaList;
  }
  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &Info) const override {
    return false;
  }
  const char *getClobbers() const override { return ""; }
  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override {
  }
};

class LLVM_LIBRARY_VISIBILITY Z80oldTargetInfo : public Z80oldTargetInfoBase {
public:
  explicit Z80oldTargetInfo(const llvm::Triple &T) : Z80oldTargetInfoBase(T) {
    PointerWidth = IntWidth = 16;
    //resetDataLayout("e-m:o-p:16:8-p1:8:8-i16:8-i24:8-i32:8-i48:8-i64:8-i96:8-f32:8-f64:8-a:8-n8:16-S8");
    resetDataLayout("e-m:o-S8-p:16:8-p1:8:8-i16:8-i32:8-a:8-n8:16");
  }

private:
  bool setCPU(const std::string &Name) override;
  bool
    initFeatureMap(llvm::StringMap<bool> &Features, DiagnosticsEngine &Diags,
                   StringRef CPU,
                   const std::vector<std::string> &FeaturesVec) const override;
  ArrayRef<const char *> getGCCRegNames() const override;
  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override {
    return None;
  }
  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;
};

} // namespace targets
} // namespace clang

#endif // LLVM_CLANG_LIB_BASIC_TARGETS_Z80old_H
