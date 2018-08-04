//===-- Z80oldTargetMachine.cpp - Define TargetMachine for Z80old -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Implements the info about Z80old target spec.
//
//===----------------------------------------------------------------------===//

#include "Z80oldTargetMachine.h"
#include "Z80old.h"
#include "MCTargetDesc/Z80oldMCTargetDesc.h"
#include "Z80oldTargetObjectFile.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Transforms/Utils.h"
using namespace llvm;

#define DEBUG_TYPE "Z80old"

extern "C" void LLVMInitializeZ80oldTarget() {
  RegisterTargetMachine<Z80oldTargetMachine> X(getTheZ80oldTarget());
}

static std::string computeDataLayout(const Triple &TT) {
  return "e-m:o-S8-p:16:8-p1:8:8-i16:8-i32:8-a:8-n8:16";

  // Z80old is little endian
  std::string Ret = "e";

  Ret += DataLayout::getManglingComponent(TT);

  // Z80old has 16 bit pointers.
  Ret += "-p:16:8";

  // Z80old aligns all integers to 8 bits.
  Ret += "-i16:8";
  Ret += "-i32:8";

  // The registers can hold 8 and 16 bits.
  Ret += "-n8:16";

  // The stack is aligned to 8 bits.
  Ret += "-a:0:8-S8";

  return Ret;
}

static Reloc::Model getEffectiveRelocModel(const Triple &TT,
                                           Optional<Reloc::Model> RM) {
  return Reloc::Static;
}

static CodeModel::Model getEffectiveCodeModel(Optional<CodeModel::Model> CM,
                                              bool JIT) {
  return CodeModel::Small;
}

llvm::Z80oldTargetMachine::Z80oldTargetMachine(const Target &T,
                                         const Triple &TT,
                                         StringRef CPU,
                                         StringRef FS,
                                         const TargetOptions &Options,
                                         Optional<Reloc::Model> RM,
                                         Optional<CodeModel::Model> CM,
                                         CodeGenOpt::Level OL,
                                         bool JIT)
  : LLVMTargetMachine(
      T,
      computeDataLayout(TT),
      TT,
      CPU,
      FS,
      Options,
      getEffectiveRelocModel(TT, RM),
      getEffectiveCodeModel(CM, JIT),
      OL),
    TLOF(make_unique<Z80oldTargetObjectFile>()) {

  initAsmInfo();
}

llvm::Z80oldTargetMachine::~Z80oldTargetMachine() = default;

const Z80oldSubtarget *Z80oldTargetMachine::getSubtargetImpl(const Function &F)
const {

  Attribute CPUAttr = F.getFnAttribute("target-cpu");
  Attribute FSAttr = F.getFnAttribute("target-features");

  StringRef CPU = !CPUAttr.hasAttribute(Attribute::None)
                  ? CPUAttr.getValueAsString()
                  : (StringRef)TargetCPU;
  StringRef FS = !FSAttr.hasAttribute(Attribute::None)
                 ? FSAttr.getValueAsString()
                 : (StringRef)TargetFS;

  SmallString<512> Key;
  Key.reserve(CPU.size() + FS.size());
  Key += CPU;
  Key += FS;

#if 0
  // FIXME: This is related to the code below to reset the target options,
  // we need to know whether or not the soft float flag is set on the
  // function before we can generate a subtarget. We also need to use
  // it as a key for the subtarget since that can be the only difference
  // between two functions.
  bool SoftFloat = true;
  // If the soft float attribute is set on the function turn on the soft float
  // subtarget feature.
  if (SoftFloat) {
    Key += FS.empty() ? "+soft-float" : ",+soft-float";
  }
#endif // 0

  // Keep track of the key width after all features are added so we can extract
  // the feature string out later.
  unsigned CPUFSWidth = Key.size();

  // Extracted here so that we make sure there is backing for the StringRef. If
  // we assigned earlier, its possible the SmallString reallocated leaving a
  // dangling StringRef.
  FS = Key.slice(CPU.size(), CPUFSWidth);

  //std::unique_ptr<Z80oldSubtarget> I = llvm::make_unique<Z80oldSubtarget>( TargetTriple, CPU, FS, *this );
  auto &I = SubtargetMap[Key];
  if (!I) {
    // This needs to be done before we create a new subtarget since any
    // creation will depend on the TM and the code generation flags on the
    // function that reside in TargetOptions.
    resetTargetOptions(F);
    I = llvm::make_unique<Z80oldSubtarget>(TargetTriple, CPU, FS, *this);
  }
  return I.get();
}

//===----------------------------------------------------------------------===//
// Pass Pipeline Configuration
//===----------------------------------------------------------------------===//

namespace {
/// Z80old Code Generator Pass Configuration Options.
class Z80oldPassConfig : public TargetPassConfig {
public:
  Z80oldPassConfig(Z80oldTargetMachine &TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  Z80oldTargetMachine &getZ80oldTargetMachine() const {
    return getTM<Z80oldTargetMachine>();
  }

  void addCodeGenPrepare() override;
  bool addInstSelector() override;
//void addPreRegAlloc() override;
//bool addPreRewrite() override;
//void addPreSched2() override;
};
} // namespace

TargetPassConfig *Z80oldTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new Z80oldPassConfig(*this, PM);
}

void Z80oldPassConfig::addCodeGenPrepare() {
  addPass(createLowerSwitchPass());
  TargetPassConfig::addCodeGenPrepare();
}

bool Z80oldPassConfig::addInstSelector() {
  // Install an instruction selector.
  addPass(createZ80oldISelDag(getZ80oldTargetMachine(), getOptLevel()));
  return false;
}

/*void Z80oldPassConfig::addPreRegAlloc() {
  TargetPassConfig::addPreRegAlloc();
  if (getOptLevel() != CodeGenOpt::None)
    ;//addPass(createZ80oldCallFrameOptimization());
}

bool Z80oldPassConfig::addPreRewrite() {
  //addPass(createZ80oldExpandPseudoPass());
  return TargetPassConfig::addPreRewrite();
}

void Z80oldPassConfig::addPreSched2() {
  // Z80oldMachineLateOptimization pass must be run after ExpandPostRAPseudos
  if (getOptLevel() != CodeGenOpt::None)
    addPass(createZ80oldMachineLateOptimization());
  TargetPassConfig::addPreSched2();
}
*/
