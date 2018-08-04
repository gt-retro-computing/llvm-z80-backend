//===- Z80oldTargetStreamer.cpp - Z80oldTargetStreamer class --*- C++ -*---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Z80oldTargetStreamer class.
//
//===----------------------------------------------------------------------===//

#include "Z80oldTargetStreamer.h"
#include "llvm/MC/MCContext.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;

Z80oldTargetStreamer::Z80oldTargetStreamer(MCStreamer &S)
  : MCTargetStreamer(S) {}

Z80oldTargetAsmStreamer::Z80oldTargetAsmStreamer(MCStreamer &S,
                                           formatted_raw_ostream &OS)
  : Z80oldTargetStreamer(S), MAI(S.getContext().getAsmInfo()), OS(OS) {}

void Z80oldTargetAsmStreamer::emitAlign(unsigned ByteAlignment) {
  if (ByteAlignment > 1) {
    OS << "\tALIGN\t" << ByteAlignment << '\n';
  }
}

void Z80oldTargetAsmStreamer::emitBlock(uint64_t NumBytes) {
  if (NumBytes) {
    OS << "\tDS\t" << NumBytes << '\n';
  }
}

void Z80oldTargetAsmStreamer::emitGlobal(MCSymbol *Symbol) {
  OS << "\tXDEF\t";
  Symbol->print(OS, MAI);
  OS << '\n';
}

void Z80oldTargetAsmStreamer::emitExtern(MCSymbol *Symbol) {
  OS << "\tXREF\t";
  Symbol->print(OS, MAI);
  OS << '\n';
}
