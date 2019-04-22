//===-- I8080AsmPrinter.h - I8080 implementation of AsmPrinter ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_I8080_I8080ASMPRINTER_H
#define LLVM_LIB_TARGET_I8080_I8080ASMPRINTER_H

#include "I8080Subtarget.h"
#include "llvm/CodeGen/AsmPrinter.h"

namespace llvm {

class LLVM_LIBRARY_VISIBILITY I8080AsmPrinter : public AsmPrinter {
  const I8080Subtarget *Subtarget;

public:
  explicit I8080AsmPrinter(TargetMachine &TM,
                         std::unique_ptr<MCStreamer> Streamer)
    : AsmPrinter(TM, std::move(Streamer)) {}

  StringRef getPassName() const override {
    return "I8080 Assembly / Object Emitter";
  }

  const I8080Subtarget &getSubtarget() const { return *Subtarget; }

  void EmitStartOfAsmFile(Module &M) override;
  void emitInlineAsmEnd(const MCSubtargetInfo &StartInfo,
                        const MCSubtargetInfo *EndInfo) const override;
  void EmitEndOfAsmFile(Module &M) override;
  void EmitGlobalVariable(const GlobalVariable *GV) override;
  void EmitInstruction(const MachineInstr *MI) override;
};
} // End llvm namespace

#endif
