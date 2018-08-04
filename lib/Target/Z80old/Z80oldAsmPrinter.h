//===-- Z80oldAsmPrinter.h - Z80old implementation of AsmPrinter ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Z80old_Z80oldASMPRINTER_H
#define LLVM_LIB_TARGET_Z80old_Z80oldASMPRINTER_H

#include "Z80oldSubtarget.h"
#include "llvm/CodeGen/AsmPrinter.h"

namespace llvm {

class LLVM_LIBRARY_VISIBILITY Z80oldAsmPrinter : public AsmPrinter {
  const Z80oldSubtarget *Subtarget;

public:
  explicit Z80oldAsmPrinter(TargetMachine &TM,
                         std::unique_ptr<MCStreamer> Streamer)
    : AsmPrinter(TM, std::move(Streamer)) {}

  StringRef getPassName() const override {
    return "Z80old Assembly / Object Emitter";
  }

  const Z80oldSubtarget &getSubtarget() const { return *Subtarget; }

  void EmitStartOfAsmFile(Module &M) override;
  void emitInlineAsmEnd(const MCSubtargetInfo &StartInfo,
                        const MCSubtargetInfo *EndInfo) const override;
  void EmitEndOfAsmFile(Module &M) override;
  void EmitGlobalVariable(const GlobalVariable *GV) override;
  void EmitInstruction(const MachineInstr *MI) override;
};
} // End llvm namespace

#endif
