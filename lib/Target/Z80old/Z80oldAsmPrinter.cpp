//===-- Z80oldAsmPrinter.cpp - Convert Z80old LLVM code to AT&T assembly --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to Z80old machine code.
//
//===----------------------------------------------------------------------===//

#include "Z80oldAsmPrinter.h"
#include "Z80old.h"
#include "MCTargetDesc/Z80oldTargetStreamer.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

//===----------------------------------------------------------------------===//
// Target Registry Stuff
//===----------------------------------------------------------------------===//

static bool isCode16(const Triple &TT) {
  return TT.getEnvironment() == Triple::CODE16;
}

void Z80oldAsmPrinter::EmitStartOfAsmFile(Module &M) {
  //const Triple &TT = TM.getTargetTriple();
}

void Z80oldAsmPrinter::emitInlineAsmEnd(const MCSubtargetInfo &StartInfo,
                                     const MCSubtargetInfo *EndInfo) const {
  return;
#if 0
  if (TM.getTargetTriple().getArch() != Triple::ez80old) {
    return;
  }
  bool Was16 = isCode16(StartInfo.getTargetTriple());
  if (!EndInfo || Was16 != isCode16(EndInfo->getTargetTriple())) {
    OutStreamer->EmitAssemblerFlag(Was16 ? MCAF_Code16 : MCAF_Code24);
  }
#endif // 0
}

void Z80oldAsmPrinter::EmitEndOfAsmFile(Module &M) {
  Z80oldTargetStreamer *TS =
    static_cast<Z80oldTargetStreamer *>(OutStreamer->getTargetStreamer());
  for (const auto &Symbol : OutContext.getSymbols())
    if (!Symbol.second->isDefined()) {
      TS->emitExtern(Symbol.second);
    }
}

void Z80oldAsmPrinter::EmitGlobalVariable(const GlobalVariable *GV) {
  Z80oldTargetStreamer *TS =
    static_cast<Z80oldTargetStreamer *>(OutStreamer->getTargetStreamer());

  if (GV->hasInitializer()) {
    // Check to see if this is a special global used by LLVM, if so, emit it.
    if (EmitSpecialLLVMGlobal(GV)) {
      return;
    }
  }

  MCSymbol *GVSym = getSymbol(GV);

  if (!GV->hasInitializer()) { // External globals require no extra code.
    return;
  }

  GVSym->redefineIfPossible();
  if (GVSym->isDefined() || GVSym->isVariable())
    report_fatal_error("symbol '" + Twine(GVSym->getName()) +
                       "' is already defined");

  SectionKind GVKind = TargetLoweringObjectFile::getKindForGlobal(GV, TM);

  const DataLayout &DL = GV->getParent()->getDataLayout();
  uint64_t Size = DL.getTypeAllocSize(GV->getType()->getElementType());

  // If the alignment is specified, we *must* obey it.  Overaligning a global
  // with a specified alignment is a prompt way to break globals emitted to
  // sections and expected to be contiguous (e.g. ObjC metadata).
  unsigned Align = DL.getPreferredAlignment(GV);

  // Determine to which section this global should be emitted.
  MCSection *TheSection = getObjFileLowering().SectionForGlobal(GV, GVKind, TM);

  OutStreamer->SwitchSection(TheSection);
  TS->emitAlign(Align);
  if (!GV->hasLocalLinkage()) {
    TS->emitGlobal(GVSym);
  }
  OutStreamer->EmitLabel(GVSym);
  if (GVKind.isBSS()) {
    TS->emitBlock(Size);
  } else {
    EmitGlobalConstant(DL, GV->getInitializer());
  }
  OutStreamer->AddBlankLine();
}

// Force static initialization.
extern "C" void LLVMInitializeZ80oldAsmPrinter() {
  RegisterAsmPrinter<Z80oldAsmPrinter> X(getTheZ80oldTarget());
  //RegisterAsmPrinter<Z80oldAsmPrinter> Y(getTheEZ80oldTarget());
}
