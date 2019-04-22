//===-- I8080MCInstLower.cpp - Convert I8080 MachineInstr to an MCInst --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains code to lower I8080 MachineInstrs to their corresponding
// MCInst records.
//
//===----------------------------------------------------------------------===//

#include "I8080AsmPrinter.h"
#include "llvm/IR/Mangler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
using namespace llvm;

#define DEBUG_TYPE "z80-mclower"

namespace {
/// I8080MCInstLower - This class is used to lower a MachineInstr into an MCInst.
class I8080MCInstLower {
  MCContext &Ctx;
  const MachineFunction &Func;
  I8080AsmPrinter &AsmPrinter;

public:
  I8080MCInstLower(const MachineFunction &MF, I8080AsmPrinter &AP);

  Optional<MCOperand> LowerMachineOperand(const MachineInstr *MI,
                                          const MachineOperand &MO) const;

  MCSymbol *GetGlobalAddressSymbol(const MachineOperand &MO) const;
  MCSymbol *GetExternalSymbolSymbol(const MachineOperand &MO) const;
  MCOperand LowerSymbolOperand(const MachineOperand &MO, MCSymbol *Sym) const;

  void Lower(const MachineInstr *MI, MCInst &OutMI) const;
};
}

I8080MCInstLower::I8080MCInstLower(const MachineFunction &MF, I8080AsmPrinter &AP)
  : Ctx(MF.getContext()), Func(MF), AsmPrinter(AP) {}

/// GetGlobalAddressSymbol - Lower an MO_GlobalAddress operand to an MCSymbol.
MCSymbol *
I8080MCInstLower::GetGlobalAddressSymbol(const MachineOperand &MO) const {
  assert(!MO.getTargetFlags() && "Unknown target flag on GV operand");
  return AsmPrinter.getSymbol(MO.getGlobal());
}

/// GetExternalSymbolSymbol - Lower an MO_ExternalSymbol operand to an MCSymbol.
MCSymbol *
I8080MCInstLower::GetExternalSymbolSymbol(const MachineOperand &MO) const {
  assert(!MO.getTargetFlags() && "Unknown target flag on GV operand");
  return AsmPrinter.GetExternalSymbolSymbol(MO.getSymbolName());
}

MCOperand I8080MCInstLower::LowerSymbolOperand(const MachineOperand &MO,
                                             MCSymbol *Sym) const {
  assert(!MO.getTargetFlags() && "Unknown target flag on GV operand");
  const MCExpr *Expr = MCSymbolRefExpr::create(Sym, Ctx);
  if (auto Off = MO.getOffset()) {
    Expr = MCBinaryExpr::createAdd(Expr, MCConstantExpr::create(Off, Ctx), Ctx);
  }
  return MCOperand::createExpr(Expr);
}

Optional<MCOperand>
I8080MCInstLower::LowerMachineOperand(const MachineInstr *MI,
                                    const MachineOperand &MO) const {
  switch (MO.getType()) {
  default:
    LLVM_DEBUG(MI->dump());
    llvm_unreachable("unknown operand type");
  case MachineOperand::MO_Register:
    return MCOperand::createReg(MO.getReg());
  case MachineOperand::MO_Immediate:
    return MCOperand::createImm(MO.getImm());
  case MachineOperand::MO_MachineBasicBlock:
    return MCOperand::createExpr(MCSymbolRefExpr::create(
                                   MO.getMBB()->getSymbol(), Ctx));
  case MachineOperand::MO_GlobalAddress:
    return LowerSymbolOperand(MO, GetGlobalAddressSymbol(MO));
  case MachineOperand::MO_ExternalSymbol:
    return LowerSymbolOperand(MO, GetExternalSymbolSymbol(MO));
  case MachineOperand::MO_RegisterMask:
    return None; // Ignore call clobbers.
  }
}

void I8080MCInstLower::Lower(const MachineInstr *MI, MCInst &OutMI) const {
  OutMI.setOpcode(MI->getOpcode());

  MCOperand MCOp;
  for (const MachineOperand &MO : MI->operands())
    if (auto PossibleMCOp = LowerMachineOperand(MI, MO)) {
      OutMI.addOperand(*PossibleMCOp);
    }
}

void I8080AsmPrinter::EmitInstruction(const MachineInstr *MI) {
  I8080MCInstLower MCInstLowering(*MF, *this);

  MCInst TmpInst;
  MCInstLowering.Lower(MI, TmpInst);
  EmitToStreamer(*OutStreamer, TmpInst);
}
