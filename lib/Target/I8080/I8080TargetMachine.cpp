//===-- I8080TargetMachine.cpp - Define TargetMachine for I8080 -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Implements the info about I8080 target spec.
//
//===----------------------------------------------------------------------===//

#include "I8080TargetMachine.h"
#include "I8080.h"
#include "MCTargetDesc/I8080MCTargetDesc.h"
#include "I8080TargetObjectFile.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Transforms/Utils.h"
using namespace llvm;

#define DEBUG_TYPE "I8080"

extern "C" void LLVMInitializeI8080Target() {
  RegisterTargetMachine<I8080TargetMachine> X(getTheI8080Target());
}

static std::string computeDataLayout(const Triple &TT) {
  return "e-m:o-S8-p:16:8-p1:8:8-i16:8-i32:8-a:8-n8:16";

  // I8080 is little endian
  std::string Ret = "e";

  Ret += DataLayout::getManglingComponent(TT);

  // I8080 has 16 bit pointers.
  Ret += "-p:16:8";

  // I8080 aligns all integers to 8 bits.
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

llvm::I8080TargetMachine::I8080TargetMachine(const Target &T,
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
    TLOF(make_unique<I8080TargetObjectFile>()) {

  initAsmInfo();
}

llvm::I8080TargetMachine::~I8080TargetMachine() = default;

const I8080Subtarget *I8080TargetMachine::getSubtargetImpl(const Function &F)
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

  //std::unique_ptr<I8080Subtarget> I = llvm::make_unique<I8080Subtarget>( TargetTriple, CPU, FS, *this );
  auto &I = SubtargetMap[Key];
  if (!I) {
    // This needs to be done before we create a new subtarget since any
    // creation will depend on the TM and the code generation flags on the
    // function that reside in TargetOptions.
    resetTargetOptions(F);
    I = llvm::make_unique<I8080Subtarget>(TargetTriple, CPU, FS, *this);
  }
  return I.get();
}

//===----------------------------------------------------------------------===//
// Pass Pipeline Configuration
//===----------------------------------------------------------------------===//

namespace {
/// I8080 Code Generator Pass Configuration Options.
class I8080PassConfig : public TargetPassConfig {
public:
  I8080PassConfig(I8080TargetMachine &TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  I8080TargetMachine &getI8080TargetMachine() const {
    return getTM<I8080TargetMachine>();
  }

  void addCodeGenPrepare() override;
  bool addInstSelector() override;
//void addPreRegAlloc() override;
//bool addPreRewrite() override;
//void addPreSched2() override;
};
} // namespace

TargetPassConfig *I8080TargetMachine::createPassConfig(PassManagerBase &PM) {
  return new I8080PassConfig(*this, PM);
}

void I8080PassConfig::addCodeGenPrepare() {
  addPass(createLowerSwitchPass());
  TargetPassConfig::addCodeGenPrepare();
}

bool I8080PassConfig::addInstSelector() {
  // Install an instruction selector.
  addPass(createI8080ISelDag(getI8080TargetMachine(), getOptLevel()));
  return false;
}

/*void I8080PassConfig::addPreRegAlloc() {
  TargetPassConfig::addPreRegAlloc();
  if (getOptLevel() != CodeGenOpt::None)
    ;//addPass(createI8080CallFrameOptimization());
}

bool I8080PassConfig::addPreRewrite() {
  //addPass(createI8080ExpandPseudoPass());
  return TargetPassConfig::addPreRewrite();
}

void I8080PassConfig::addPreSched2() {
  // I8080MachineLateOptimization pass must be run after ExpandPostRAPseudos
  if (getOptLevel() != CodeGenOpt::None)
    addPass(createI8080MachineLateOptimization());
  TargetPassConfig::addPreSched2();
}
*/
