//===- I8080TargetStreamer.cpp - I8080TargetStreamer class --*- C++ -*---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the I8080TargetStreamer class.
//
//===----------------------------------------------------------------------===//

#include "I8080TargetStreamer.h"
#include "llvm/MC/MCContext.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;

I8080TargetStreamer::I8080TargetStreamer(MCStreamer &S)
  : MCTargetStreamer(S) {}

I8080TargetAsmStreamer::I8080TargetAsmStreamer(MCStreamer &S,
                                           formatted_raw_ostream &OS)
  : I8080TargetStreamer(S), MAI(S.getContext().getAsmInfo()), OS(OS) {}

void I8080TargetAsmStreamer::emitAlign(unsigned ByteAlignment) {
  if (ByteAlignment > 1) {
    OS << "\tALIGN\t" << ByteAlignment << '\n';
  }
}

void I8080TargetAsmStreamer::emitBlock(uint64_t NumBytes) {
  if (NumBytes) {
    OS << "\tDS\t" << NumBytes << '\n';
  }
}

void I8080TargetAsmStreamer::emitGlobal(MCSymbol *Symbol) {
  OS << "\tXDEF\t";
  Symbol->print(OS, MAI);
  OS << '\n';
}

void I8080TargetAsmStreamer::emitExtern(MCSymbol *Symbol) {
  OS << "\tXREF\t";
  Symbol->print(OS, MAI);
  OS << '\n';
}
