//===-- Z80ISelLowering.cpp - Z80 DAG Lowering Implementation -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Z80 uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#include "Z80ISelLowering.h"
#include "MCTargetDesc/Z80MCTargetDesc.h"
#include "Z80MachineFunctionInfo.h"
#include "Z80Subtarget.h"
#include "Z80TargetMachine.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/Support/KnownBits.h"
using namespace llvm;

#define DEBUG_TYPE "z80-isel"


/// Return true if the calling convention is one that we can guarantee TCO for.
static bool canGuaranteeTCO(CallingConv::ID CC) {
  return (CC == CallingConv::Fast || CC == CallingConv::GHC ||
          CC == CallingConv::X86_RegCall || CC == CallingConv::HiPE ||
          CC == CallingConv::HHVM);
}

/// Return true if the function is being made into a tailcall target by
/// changing its ABI.
static bool shouldGuaranteeTCO(CallingConv::ID CC, bool GuaranteedTailCallOpt) {
  return GuaranteedTailCallOpt && canGuaranteeTCO(CC);
}

/// CallIsStructReturn - Determines whether a call uses struct return
/// semantics.
enum StructReturnType {
  NotStructReturn,
  RegStructReturn,
  StackStructReturn
};
static StructReturnType
callIsStructReturn(ArrayRef<ISD::OutputArg> Outs/*, bool IsMCU*/) {
  if (Outs.empty()) {
    return NotStructReturn;
  }

  const ISD::ArgFlagsTy &Flags = Outs[0].Flags;
  if (!Flags.isSRet()) {
    return NotStructReturn;
  }
  if (Flags.isInReg() /*|| IsMCU*/) {
    return RegStructReturn;
  }
  return StackStructReturn;
}

/// Determines whether a function uses struct return semantics.
static StructReturnType
argsAreStructReturn(ArrayRef<ISD::InputArg> Ins/*, bool IsMCU*/) {
  if (Ins.empty()) {
    return NotStructReturn;
  }

  const ISD::ArgFlagsTy &Flags = Ins[0].Flags;
  if (!Flags.isSRet()) {
    return NotStructReturn;
  }
  if (Flags.isInReg() /*|| IsMCU*/) {
    return RegStructReturn;
  }
  return StackStructReturn;
}

/// Determines whether the callee is required to pop its own arguments.
/// Callee pop is necessary to support tail calls.
bool Z80::isCalleePop(CallingConv::ID CallingConv,
                      /*bool is64Bit,*/ bool IsVarArg, bool GuaranteeTCO) {
  // If GuaranteeTCO is true, we force some calls to be callee pop so that we
  // can guarantee TCO.
  if (!IsVarArg && shouldGuaranteeTCO(CallingConv, GuaranteeTCO)) {
    return true;
  }

  switch (CallingConv) {
  default:
    return false;
  case CallingConv::X86_StdCall:
  case CallingConv::X86_FastCall:
  case CallingConv::X86_ThisCall:
  case CallingConv::X86_VectorCall:
    return false; // !is64Bit;  $TODO
  }
}


Z80TargetLowering::Z80TargetLowering(const Z80TargetMachine &TM,
                                     const Z80Subtarget &STI)
  : TargetLowering(TM), Subtarget(STI) {
  //bool HasEZ80Ops = Subtarget.hasEZ80Ops(), Is24Bit = Subtarget.is24Bit();
  MVT PtrVT = MVT::getIntegerVT(8 * TM.getPointerSize(0));

  setSchedulingPreference(Sched::RegPressure);

  // Set up the register classes.
  addRegisterClass(MVT::i8, &Z80::RR8RegClass);
  addRegisterClass(MVT::i16, &Z80::R16RegClass);
  //if (Is24Bit)
  //  addRegisterClass(MVT::i24, &Z80::R24RegClass);
  for (unsigned Opc : { ISD::ROTL, ISD::ROTR })
    setOperationAction(Opc, MVT::i8, Custom);
  for (MVT VT : { MVT::i16, MVT::i32 }) {
    for (unsigned Opc : { //ISD::ADD, ISD::SUB,
           ISD::AND, ISD::OR, ISD::XOR,
           ISD::ANY_EXTEND, ISD::ZERO_EXTEND, ISD::SIGN_EXTEND
         })
      setOperationAction(Opc, VT, Custom);
    for (unsigned Opc : { ISD::ROTL, ISD::ROTR })
      setOperationAction(Opc, VT, Expand);
  }
  for (MVT VT : { MVT::i8, MVT::i16, MVT::i32 }) {
    for (unsigned Opc : { ISD::SHL, ISD::SRA, ISD::SRL })
      setOperationAction(Opc, VT, Custom);
    for (unsigned Opc : {
           ISD::MUL,
           ISD::SDIV,    ISD::UDIV,
           ISD::SREM,    ISD::UREM,
           ISD::SDIVREM, ISD::UDIVREM
         })
      setOperationAction(Opc, VT, LibCall);
    for (unsigned Opc : {
           ISD::SMUL_LOHI, ISD::UMUL_LOHI,
           ISD::SMULO,     ISD::UMULO,
           ISD::MULHU,     ISD::MULHS,
           ISD::SMIN,      ISD::SMAX,
           ISD::UMIN,      ISD::UMAX,
           ISD::BSWAP,     ISD::CTTZ,      ISD::CTLZ,
           ISD::CTPOP,     ISD::BITREVERSE,
           ISD::CTTZ_ZERO_UNDEF,           ISD::CTLZ_ZERO_UNDEF,
           ISD::SELECT,    ISD::SETCC,
           ISD::SETCCCARRY, // $TODO: Was SETCCE before, see comment in CodeGen/ISDOpcodes.h:414
           ISD::SHL_PARTS, ISD::SRA_PARTS, ISD::SRL_PARTS
         })
      setOperationAction(Opc, VT, Expand);
    for (unsigned Opc : { ISD::EXTLOAD, ISD::SEXTLOAD, ISD::ZEXTLOAD })
      setLoadExtAction(Opc, VT, MVT::i1, Promote);
    for (MVT MemVT : { MVT::i8, MVT::i16 }) {
      if (MemVT.bitsGE(VT)) {
        break;
      }
      for (unsigned Opc : { ISD::SEXTLOAD, ISD::ZEXTLOAD })
        setLoadExtAction(Opc, VT, MemVT, Expand);
      setTruncStoreAction(VT, MemVT, Expand);
    }
  }
  for (MVT VT : { MVT::i1, MVT::i8, MVT::i16 })
    setOperationAction(ISD::SIGN_EXTEND_INREG, VT, Expand);
  for (MVT VT : { MVT::i8, MVT::i16, MVT::f32 })
    for (unsigned Opc : { ISD::BR_CC, ISD::SELECT_CC })
      setOperationAction(Opc, VT, Custom);
  for (unsigned Opc : { ISD::BRCOND, ISD::BR_JT })
    setOperationAction(Opc, MVT::Other, Expand);
  //if (Subtarget.hasZ180Ops())
  //  for (MVT VT : { MVT::i8, MVT::i16 })
  //    setOperationAction(ISD::MUL, VT, Custom);

  //if (!HasEZ80Ops)
  //setOperationAction(ISD::LOAD, MVT::i16, Custom);
  //if (!HasEZ80Ops || Is24Bit)
  //setOperationAction(ISD::STORE, MVT::i16, Custom);
  //if (Is24Bit)
  //setLoadExtAction(ISD::EXTLOAD, MVT::i24, MVT::i16, Legal);
  setOperationAction(ISD::DYNAMIC_STACKALLOC, PtrVT, Expand);
  for (unsigned Opc : {
         ISD::GlobalAddress, ISD::ExternalSymbol,
         ISD::BlockAddress
       })
    setOperationAction(Opc, PtrVT, Custom);

  setOperationAction(ISD::VASTART, MVT::Other, Custom);
  setOperationAction(ISD::VAARG,   MVT::Other, Expand);
  setOperationAction(ISD::VAEND,   MVT::Other, Expand);
  setOperationAction(ISD::VACOPY,  MVT::Other, Expand);

  setStackPointerRegisterToSaveRestore(Z80::SPS);

  setTargetDAGCombine(ISD::MUL);
  setTargetDAGCombine(ISD::AND);
  setTargetDAGCombine(ISD::ZERO_EXTEND);
  setTargetDAGCombine(ISD::TRUNCATE);

  // Compute derived properties from the register classes
  computeRegisterProperties(STI.getRegisterInfo());

  setBooleanContents(UndefinedBooleanContent);

  // old: setLibcallName(RTLIB::ZEXT_I16_I24, "_stoiu");
  // old: setLibcallCallingConv(RTLIB::ZEXT_I16_I24, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::SEXT_I16_I24, "_stoi");
  // old: setLibcallCallingConv(RTLIB::SEXT_I16_I24, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::SEXT_I24_I32, "_itol");
  // old: setLibcallCallingConv(RTLIB::SEXT_I24_I32, CallingConv::Z80_LibCall);
  //setLibcallName(RTLIB::NOT_I16, "_snot");
  //setLibcallCallingConv(RTLIB::NOT_I16, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::NOT_I24, "_inot");
  // old: setLibcallCallingConv(RTLIB::NOT_I24, CallingConv::Z80_LibCall);
  //setLibcallName(RTLIB::NOT_I32, "_lnot");
  //setLibcallCallingConv(RTLIB::NOT_I32, CallingConv::Z80_LibCall);
  //setLibcallName(RTLIB::AND_I16, "_sand");
  //setLibcallCallingConv(RTLIB::AND_I16, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::AND_I24, "_iand");
  // old: setLibcallCallingConv(RTLIB::AND_I24, CallingConv::Z80_LibCall);
  //setLibcallName(RTLIB::AND_I32, "_land");
  //setLibcallCallingConv(RTLIB::AND_I32, CallingConv::Z80_LibCall);
  //setLibcallName(RTLIB::OR_I16, "_sor");
  //setLibcallCallingConv(RTLIB::OR_I16, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::OR_I24, "_ior");
  // old: setLibcallCallingConv(RTLIB::OR_I24, CallingConv::Z80_LibCall);
  //setLibcallName(RTLIB::OR_I32, "_lor");
  //setLibcallCallingConv(RTLIB::OR_I32, CallingConv::Z80_LibCall);
  //setLibcallName(RTLIB::XOR_I16, "_sxor");
  //setLibcallCallingConv(RTLIB::XOR_I16, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::XOR_I24, "_ixor");
  // old: setLibcallCallingConv(RTLIB::XOR_I24, CallingConv::Z80_LibCall);
  //setLibcallName(RTLIB::XOR_I32, "_lxor");
  //setLibcallCallingConv(RTLIB::XOR_I32, CallingConv::Z80_LibCall);
  //setLibcallName(RTLIB::SHL_I8, "_bshl");
  //setLibcallCallingConv(RTLIB::SHL_I8, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::SHL_I16, "_sshl");
  setLibcallCallingConv(RTLIB::SHL_I16, CallingConv::Z80_LibCall_C);
  //setLibcallName(RTLIB::SHL_I16_I8, "_sshl_b");
  //setLibcallCallingConv(RTLIB::SHL_I16_I8, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::SHL_I24, "_ishl");
  // old: setLibcallCallingConv(RTLIB::SHL_I24, CallingConv::Z80_LibCall_C);
  // old: setLibcallName(RTLIB::SHL_I24_I8, "_ishl_b");
  // old: setLibcallCallingConv(RTLIB::SHL_I24_I8, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::SHL_I32, "_lshl");
  setLibcallCallingConv(RTLIB::SHL_I32, CallingConv::Z80_LibCall_L);
  //setLibcallName(RTLIB::SRA_I8, "_bshrs");
  //setLibcallCallingConv(RTLIB::SRA_I8, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::SRA_I16, "_sshrs");
  setLibcallCallingConv(RTLIB::SRA_I16, CallingConv::Z80_LibCall_C);
  //setLibcallName(RTLIB::SRA_I16_I8, "_sshrs_b");
  //setLibcallCallingConv(RTLIB::SRA_I16_I8, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::SRA_I24, "_ishrs");
  // old: setLibcallCallingConv(RTLIB::SRA_I24, CallingConv::Z80_LibCall_C);
  // old: setLibcallName(RTLIB::SRA_I24_I8, "_ishrs_b");
  // old: setLibcallCallingConv(RTLIB::SRA_I24_I8, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::SRA_I32, "_lshrs");
  setLibcallCallingConv(RTLIB::SRA_I32, CallingConv::Z80_LibCall_L);
  //setLibcallName(RTLIB::SRL_I8, "_bshl");
  //setLibcallCallingConv(RTLIB::SRL_I8, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::SRL_I16, "_sshru");
  setLibcallCallingConv(RTLIB::SRL_I16, CallingConv::Z80_LibCall_C);
  //setLibcallName(RTLIB::SRL_I16_I8, "_sshru_b");
  //setLibcallCallingConv(RTLIB::SRL_I16_I8, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::SRL_I24, "_ishru");
  // old: setLibcallCallingConv(RTLIB::SRL_I24, CallingConv::Z80_LibCall_C);
  // old: setLibcallName(RTLIB::SRL_I24_I8, "_ishru_b");
  // old: setLibcallCallingConv(RTLIB::SRL_I24_I8, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::SRL_I32, "_lshru");
  setLibcallCallingConv(RTLIB::SRL_I32, CallingConv::Z80_LibCall_L);
  //setLibcallName(RTLIB::CMP_I32, "_lcmpu");
  //setLibcallCallingConv(RTLIB::CMP_I32, CallingConv::Z80_LibCall);
  //setLibcallName(RTLIB::CMP_I16_0, "_scmpzero");
  //setLibcallCallingConv(RTLIB::CMP_I16_0, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::CMP_I24_0, "_icmpzero");
  // old: setLibcallCallingConv(RTLIB::CMP_I24_0, CallingConv::Z80_LibCall);
  //setLibcallName(RTLIB::CMP_I32_0, "_lcmpzero");
  //setLibcallCallingConv(RTLIB::CMP_I32_0, CallingConv::Z80_LibCall);
  //setLibcallName(RTLIB::SCMP, "_setflag");
  //setLibcallCallingConv(RTLIB::SCMP, CallingConv::Z80_LibCall);
  //setLibcallName(RTLIB::NEG_I16, "_sneg");
  //setLibcallCallingConv(RTLIB::NEG_I16, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::NEG_I24, "_ineg");
  // old: setLibcallCallingConv(RTLIB::NEG_I24, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::NEG_I32, "_lneg");
  setLibcallCallingConv(RTLIB::NEG_I32, CallingConv::Z80_LibCall);
  //setLibcallName(RTLIB::ADD_I32, "_ladd");
  //setLibcallCallingConv(RTLIB::ADD_I32, CallingConv::Z80_LibCall);
  //setLibcallName(RTLIB::ADD_I32_I8, "_ladd_b");
  //setLibcallCallingConv(RTLIB::ADD_I32_I8, CallingConv::Z80_LibCall);
  //setLibcallName(RTLIB::SUB_I32, "_lsub");
  //setLibcallCallingConv(RTLIB::SUB_I32, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::MUL_I8, "_bmulu");
  setLibcallCallingConv(RTLIB::MUL_I8, CallingConv::Z80_LibCall_BC);
  setLibcallName(RTLIB::MUL_I16, "_smulu");
  setLibcallCallingConv(RTLIB::MUL_I16, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::MUL_I24, "_imulu");
  // old: setLibcallCallingConv(RTLIB::MUL_I24, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::MUL_I24_I8, "_imul_b");
  // old: setLibcallCallingConv(RTLIB::MUL_I24_I8, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::MUL_I32, "_lmulu");
  setLibcallCallingConv(RTLIB::MUL_I32, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::SDIV_I8, "_bdivs");
  setLibcallCallingConv(RTLIB::SDIV_I8, CallingConv::Z80_LibCall_BC);
  setLibcallName(RTLIB::SDIV_I16, "_sdivs");
  setLibcallCallingConv(RTLIB::SDIV_I16, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::SDIV_I24, "_idivs");
  // old: setLibcallCallingConv(RTLIB::SDIV_I24, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::SDIV_I32, "_ldivs");
  setLibcallCallingConv(RTLIB::SDIV_I32, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::UDIV_I8, "_bdivu");
  setLibcallCallingConv(RTLIB::UDIV_I8, CallingConv::Z80_LibCall_BC);
  setLibcallName(RTLIB::UDIV_I16, "_sdivu");
  setLibcallCallingConv(RTLIB::UDIV_I16, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::UDIV_I24, "_idivu");
  // old: setLibcallCallingConv(RTLIB::UDIV_I24, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::UDIV_I32, "_ldivu");
  setLibcallCallingConv(RTLIB::UDIV_I32, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::SREM_I8, "_brems");
  setLibcallCallingConv(RTLIB::SREM_I8, CallingConv::Z80_LibCall_AC);
  setLibcallName(RTLIB::SREM_I16, "_srems");
  setLibcallCallingConv(RTLIB::SREM_I16, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::SREM_I24, "_irems");
  // old: setLibcallCallingConv(RTLIB::SREM_I24, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::SREM_I32, "_lrems");
  setLibcallCallingConv(RTLIB::SREM_I32, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::UREM_I8, "_bremu");
  setLibcallCallingConv(RTLIB::UREM_I8, CallingConv::Z80_LibCall_AC);
  setLibcallName(RTLIB::UREM_I16, "_sremu");
  setLibcallCallingConv(RTLIB::UREM_I16, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::UREM_I24, "_iremu");
  // old: setLibcallCallingConv(RTLIB::UREM_I24, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::UREM_I32, "_lremu");
  setLibcallCallingConv(RTLIB::UREM_I32, CallingConv::Z80_LibCall);
  // old: setLibcallName(RTLIB::UDIVREM_I24, "_idvrmu");
  // old: setLibcallCallingConv(RTLIB::UDIVREM_I24, CallingConv::Z80_LibCall);
  setLibcallName(RTLIB::UDIVREM_I32, "_ldvrmu");
  setLibcallCallingConv(RTLIB::UDIVREM_I32, CallingConv::Z80_LibCall);
  setLibcallCallingConv(RTLIB::ADD_F32, CallingConv::Z80_LibCall_L);
}

// SelectionDAG Helpers

SDValue Z80TargetLowering::EmitOffset(int64_t Amount, const SDLoc &DL,
                                      SDValue Op, SelectionDAG &DAG) const {
  EVT VT = Op.getValueType();
  return DAG.getNode(ISD::ADD, DL, VT, Op, DAG.getConstant(Amount, DL, VT));
}

SDValue Z80TargetLowering::EmitNegate(const SDLoc &DL, SDValue Op,
                                      SelectionDAG &DAG) const {
  EVT VT = Op.getValueType();
  return DAG.getNode(ISD::SUB, DL, VT, DAG.getConstant(0, DL, VT), Op);
}

SDValue Z80TargetLowering::EmitFlipSign(const SDLoc &DL, SDValue Op,
                                        SelectionDAG &DAG) const {
  EVT VT = Op.getValueType();
  return DAG.getNode(ISD::ADD, DL, VT, Op, DAG.getConstant(
                       APInt::getSignMask(VT.getSizeInBits()), DL, VT));
}

SDValue Z80TargetLowering::EmitPair(const SDLoc &DL, SDValue Hi, SDValue Lo,
                                    SelectionDAG &DAG) const {
  assert(Hi.getValueType() == MVT::i8 && Lo.getValueType() == MVT::i8 &&
         "Can only emit a pair of i8 to i16");
  SDValue Ops[] = {
    DAG.getTargetConstant(Z80::GR16RegClassID, DL, MVT::i32),
    Hi, DAG.getTargetConstant(Z80::sub_high, DL, MVT::i32),
    Lo, DAG.getTargetConstant(Z80::sub_low, DL, MVT::i32),
  };
  SDNode *Res = DAG.getMachineNode(TargetOpcode::REG_SEQUENCE, DL, MVT::i16, Ops);
  return SDValue(Res, 0);
}

SDValue Z80TargetLowering::EmitLow(SDValue Op, SelectionDAG &DAG) const {
  return DAG.getNode(ISD::TRUNCATE, SDLoc(Op), MVT::i8, Op);
}

SDValue Z80TargetLowering::EmitHigh(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  return EmitLow(DAG.getNode(ISD::SRL, DL, Op.getValueType(), Op,
                             DAG.getConstant(8, DL, MVT::i8)), DAG);
}

SDValue Z80TargetLowering::EmitSignToCarry(SDValue Op,
                                           SelectionDAG &DAG) const {
  SDLoc DL(Op);
  EVT VT = Op.getValueType();
#if 0
  if (VT == MVT::i24) {
    if (Op.hasOneUse() && ISD::isNormalLoad(Op.getNode())) {
      Op = DAG.getNode(ISD::SRL, DL, MVT::i24, Op,
                       DAG.getConstant(16, DL, MVT::i8));
      Op = DAG.getNode(ISD::TRUNCATE, DL, MVT::i8, Op);
    } else
      return DAG.getNode(Z80ISD::ADD, DL, DAG.getVTList(VT, MVT::i8), Op, Op)
             .getValue(1);
  } else
#endif // 0
    if (VT == MVT::i16) {
      Op = DAG.getTargetExtractSubreg(Z80::sub_high, DL, MVT::i8, Op);
    }
  assert(Op.getValueType() == MVT::i8 && "Unexpected type!");
  return DAG.getNode(Z80ISD::SLA, DL, DAG.getVTList(MVT::i8, MVT::i8), Op)
         .getValue(1);
}

// Legalize Types Helpers

void Z80TargetLowering::ReplaceNodeResults(SDNode *N,
                                           SmallVectorImpl<SDValue> &Results,
                                           SelectionDAG &DAG) const {
  LLVM_DEBUG(dbgs() << "ReplaceNodeResults: "; N->dump(&DAG));
}

// Legalize Helpers

SDValue Z80TargetLowering::LowerAddSub(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  unsigned Opc = Op.getOpcode();
  EVT VT = Op.getValueType();
  SDValue LHS = Op.getOperand(0), RHS = Op.getOperand(1);
  ConstantSDNode *ConstLHS = dyn_cast<ConstantSDNode>(LHS);
  bool OptSize = DAG.getMachineFunction().getFunction().getAttributes()
                 .hasAttribute(AttributeList::FunctionIndex, Attribute::OptimizeForSize);
  if (OptSize && Opc == ISD::SUB && VT.bitsGT(MVT::i8) && ConstLHS &&
      !ConstLHS->getZExtValue()) {
    assert(false && "Fix RTLIB");
  }
  return LowerLibCall(RTLIB::UNKNOWN_LIBCALL,
                      RTLIB::FPROUND_F128_F16 /*RTLIB::NEG_I16*/,
                      RTLIB::FPROUND_F128_F16, DAG.getMergeValues(RHS, DL), DAG);
  return SDValue();
}

SDValue Z80TargetLowering::LowerBitwise(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  unsigned Opc = Op.getOpcode();
  assert((Opc == ISD::AND || Opc == ISD::OR || Opc == ISD::XOR) &&
         "Unexpected opcode!");
  EVT VT = Op.getValueType();
  assert((VT == MVT::i16) && "Unexpected type!");
  SDValue LHS = Op.getOperand(0), RHS = Op.getOperand(1);
  ConstantSDNode *ConstRHS = dyn_cast<ConstantSDNode>(RHS);
  bool OptSize = DAG.getMachineFunction().getFunction().getAttributes()
                 .hasAttribute(AttributeList::FunctionIndex, Attribute::OptimizeForSize);
  // FIXME: this can be worse for constants
  if (Opc != ISD::AND && DAG.haveNoCommonBitsSet(LHS, RHS)) {
    return DAG.getNode(ISD::ADD, DL, VT, LHS, RHS);
  }
  if (OptSize && Opc == ISD::XOR && ConstRHS && ConstRHS->getSExtValue() == ~0) {
    assert(false && "Fix RTLIB");
    return LowerLibCall(RTLIB::UNKNOWN_LIBCALL,
                        RTLIB::FPROUND_F128_F16 /*RTLIB::NOT_I16*/,
                        RTLIB::FPROUND_F128_F16 /*RTLIB::NOT_I32*/, DAG.getMergeValues(LHS, DL), DAG);
  }
  KnownBits Known;
  DAG.computeKnownBits(Op, Known);
  SDValue ResHi, ResLo;
  //if (VT == MVT::i24 && ((Known.Zero | Known.One) & 0xFF0000) != 0xFF0000)
  //  return SDValue(); // fallback to libcall
  if (((Known.Zero | Known.One) & 0x00FF00) != 0x00FF00) {
    SDValue LHSHi = DAG.getTargetExtractSubreg(Z80::sub_high, DL, MVT::i8, LHS);
    SDValue RHSHi = DAG.getTargetExtractSubreg(Z80::sub_high, DL, MVT::i8, RHS);
    ResHi = DAG.getNode(Opc, DL, MVT::i8, LHSHi, RHSHi);
    Known.One &= ~0x00FF00;
  }
  if (((Known.Zero | Known.One) & 0x0000FF) != 0x0000FF) {
    SDValue LHSLo = DAG.getTargetExtractSubreg(Z80::sub_low, DL, MVT::i8, LHS);
    SDValue RHSLo = DAG.getTargetExtractSubreg(Z80::sub_low, DL, MVT::i8, RHS);
    ResLo = DAG.getNode(Opc, DL, MVT::i8, LHSLo, RHSLo);
    Known.One &= ~0x0000FF;
  }
  if (VT == MVT::i16 && ResHi && ResLo) {
    return EmitPair(DL, ResHi, ResLo, DAG);
  }
  SDValue Res = DAG.getConstant(Known.One, DL, VT);
  if (ResHi) {
    Res = DAG.getTargetInsertSubreg(Z80::sub_high, DL, VT, Res, ResHi);
  }
  if (ResLo) {
    Res = DAG.getTargetInsertSubreg(Z80::sub_low,  DL, VT, Res, ResLo);
  }
  return Res;
}

SDValue Z80TargetLowering::LowerShift(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  unsigned Opc = Op.getOpcode();
  assert((Opc == ISD::SHL || Opc == ISD::SRL || Opc == ISD::SRA ||
          Opc == ISD::ROTL || Opc == ISD::ROTR) && "Unexpected opcode!");
  EVT VT = Op.getValueType();
  assert((VT == MVT::i8 || (Opc != ISD::ROTL && Opc != ISD::ROTR)) &&
         "Unsupported operation!");
  //bool Is24Bit = Subtarget.is24Bit();
  bool OptSize = DAG.getMachineFunction().getFunction().getAttributes()
                 .hasAttribute(AttributeList::FunctionIndex, Attribute::OptimizeForSize);
  if (ConstantSDNode *Shift = dyn_cast<ConstantSDNode>(Op.getOperand(1))) {
    unsigned Amt = Shift->getZExtValue();
    SDValue Val = Op->getOperand(0);
    if (Opc == ISD::SRA && Amt == VT.getSizeInBits() - 1) {
      return DAG.getNode(Z80ISD::SEXT, DL, VT, EmitSignToCarry(Val, DAG));
    }
    SDVTList VTList = DAG.getVTList(MVT::i8, MVT::i8);
    switch (VT.getSizeInBits()) {
    case 8:
      if (Amt >= 4 && (Opc == ISD::ROTL || Opc == ISD::ROTR)) {
        Opc = Opc == ISD::ROTL ? ISD::ROTR : ISD::ROTL;
        Amt = 8 - Amt;
      }
      if (OptSize && Amt > 2 /*+ Is24Bit*/) {
        break;
      }
      switch (Opc) {
      default: llvm_unreachable("Unexpected opcode!");
      case ISD::SHL: Opc = Z80ISD::SLA; break;
      case ISD::SRA: Opc = Z80ISD::SRA; break;
      case ISD::SRL: Opc = Z80ISD::SRL; break;
      case ISD::ROTL: Opc = Z80ISD::RLC; break;
      case ISD::ROTR: Opc = Z80ISD::RRC; break;
      }
      while (Amt--) {
        Val = DAG.getNode(Opc, DL, VTList, Val);
      }
      return Val;
    case 16:
      if (Amt >= 8) {
        Val = DAG.getTargetExtractSubreg(
                Opc == ISD::SHL ? Z80::sub_low : Z80::sub_high, DL, MVT::i8, Val);
        Val = DAG.getNode(Opc, DL, MVT::i8, Val,
                          DAG.getConstant(Amt - 8, DL, MVT::i8));
        if (Opc == ISD::SHL) {
          return EmitPair(DL, Val, DAG.getConstant(0, DL, MVT::i8), DAG);
        }
        return DAG.getNode(Opc == ISD::SRL ? ISD::ZERO_EXTEND
                           : ISD::SIGN_EXTEND, DL, VT, Val);
      }
      if (Opc != ISD::SHL && (!OptSize || Amt == 1)) {
        unsigned Opc0 = Opc == ISD::SRA ? Z80ISD::SRA : Z80ISD::SRL;
        SDValue Val0 = DAG.getTargetExtractSubreg(Z80::sub_high, DL,
                                                  MVT::i8, Val);
        unsigned Opc1 = Z80ISD::RR;
        SDValue Val1 = DAG.getTargetExtractSubreg(Z80::sub_low, DL,
                                                  MVT::i8, Val);
        while (Amt--) {
          Val0 = DAG.getNode(Opc0, DL, VTList, Val0);
          Val1 = DAG.getNode(Opc1, DL, VTList, Val1, Val0.getValue(1));
        }
        return EmitPair(DL, Val0, Val1, DAG);
      }
      LLVM_FALLTHROUGH;
#if 0
    case 24:
      if (Opc == ISD::SHL) {
        if (OptSize && Amt > 5 + Is24Bit) {
          break;
        }
        MVT PtrVT = getPointerTy(DAG.getDataLayout());
        Val = DAG.getNode(ISD::ANY_EXTEND, DL, PtrVT, Val);
        while (Amt--)
          Val = DAG.getNode(Z80ISD::ADD, DL,
                            DAG.getVTList(PtrVT, MVT::i8), Val, Val);
        return DAG.getNode(ISD::TRUNCATE, DL, VT, Val);
      }
      if (Amt >= 8) {
        SDValue Ptr, Chain;
        MachinePointerInfo MPI;
        if (ISD::isNormalLoad(Val.getNode())) {
          auto Load = cast<LoadSDNode>(Val);
          Ptr = Load->getBasePtr();
          MPI = Load->getPointerInfo();
          Chain = Load->getChain();
        } else {
          Ptr = DAG.CreateStackTemporary(MVT::i24);
          MPI = MachinePointerInfo::getFixedStack(
                  DAG.getMachineFunction(),
                  cast<FrameIndexSDNode>(Ptr)->getIndex());
          Chain = DAG.getStore(DAG.getEntryNode(), DL, Val, Ptr, MPI);
        }
        unsigned Off = Amt / 8;
        MVT NewVT = Amt >= 16 ? MVT::i8 : MVT::i16;
        Val = DAG.getLoad(NewVT, DL, Chain,
                          DAG.getMemBasePlusOffset(Ptr, Off, DL),
                          MPI.getWithOffset(Off));
        Val = DAG.getNode(Opc, DL, NewVT, Val,
                          DAG.getConstant(Amt - Off * 8, DL, NewVT));
        return DAG.getNode(Opc == ISD::SRL ? ISD::ZERO_EXTEND
                           : ISD::SIGN_EXTEND, DL, VT, Val);
      }
      break;
#endif // 0
    }
  }
  return SDValue();
}

SDValue Z80TargetLowering::LowerAnyExtend(SDValue Op, SelectionDAG &DAG) const {
  assert(Op.getOpcode() == ISD::ANY_EXTEND && "Unexpected opcode");
  SDLoc DL(Op);
  EVT VT = Op.getValueType();
  SDValue Val = Op.getOperand(0);
  EVT ValVT = Val.getValueType();
  assert(((VT == MVT::i16 && ValVT == MVT::i8)) &&
         "Unexpected any extend");
  return DAG.getTargetInsertSubreg(
           ValVT == MVT::i16 ? Z80::sub_short : Z80::sub_low, DL, VT,
           DAG.getUNDEF(VT), Val);
}

SDValue Z80TargetLowering::LowerZeroExtend(SDValue Op,
                                           SelectionDAG &DAG) const {
  assert(Op.getOpcode() == ISD::ZERO_EXTEND && "Unexpected opcode");
  SDLoc DL(Op);
  EVT VT = Op.getValueType();
  SDValue Val = Op.getOperand(0);
  EVT ValVT = Val.getValueType();
  dbgs() << "Zero extending: ";
  Val.dump();
  if (VT == MVT::i16) {
    assert(ValVT == MVT::i8 && "Unexpected zero extend");
    return EmitPair(DL, DAG.getConstant(0, DL, MVT::i8), Val, DAG);
  }
  //assert(VT == MVT::i24 && (ValVT == MVT::i16 || ValVT == MVT::i8) &&
  //       "Unexpected zero extend");
  return DAG.getTargetInsertSubreg(
           ValVT == MVT::i16 ? Z80::sub_short : Z80::sub_low, DL, VT,
           DAG.getConstant(0, DL, VT), Val);
}

SDValue Z80TargetLowering::LowerSignExtend(SDValue Op,
                                           SelectionDAG &DAG) const {
  assert(Op.getOpcode() == ISD::SIGN_EXTEND && "Unexpected opcode");
  SDLoc DL(Op);
  EVT VT = Op.getValueType();
  SDValue Val = Op.getOperand(0);
  EVT ValVT = Val.getValueType();
  SDValue Sign = EmitSignToCarry(Val, DAG);
  if (VT == MVT::i16) {
    assert(ValVT == MVT::i8 && "Unexpected sign extend");
    return DAG.getTargetInsertSubreg(
             Z80::sub_high, DL, VT, DAG.getNode(ISD::ANY_EXTEND, DL, VT, Val),
             DAG.getNode(Z80ISD::SEXT, DL, ValVT, Sign));
  }
  //assert(VT == MVT::i24 && (ValVT == MVT::i16 || ValVT == MVT::i8) &&
  //       "Unexpected sign extend");
  return DAG.getTargetInsertSubreg(
           ValVT == MVT::i16 ? Z80::sub_short : Z80::sub_low, DL, VT,
           DAG.getNode(Z80ISD::SEXT, DL, VT, Sign), Val);
}

SDValue Z80TargetLowering::LowerMul(SDValue Op, SelectionDAG &DAG) const {
  assert(Op.getOpcode() == ISD::MUL && "Unexpected opcode");
  SDLoc DL(Op);
  EVT VT = Op.getValueType();
  unsigned Bits = VT.getSizeInBits();
  KnownBits Known;
  APInt LoMask = APInt::getLowBitsSet(Bits, 8), HiMask = ~LoMask;
  SDValue InOps[] = { Op.getOperand(0), Op.getOperand(1) };
  bool NegRes = false;
  if (VT != MVT::i8)
    for (SDValue &Op : InOps) {
      DAG.computeKnownBits(Op, Known);
      Op = DAG.getNode(ISD::TRUNCATE, DL, MVT::i8, Op);
      if ((Known.One & HiMask) == HiMask && Known.One.intersects(LoMask)) {
        Op = DAG.getNode(ISD::SUB, DL, MVT::i8,
                         DAG.getConstant(0, DL, MVT::i8), Op);
        NegRes = !NegRes;
      } else if ((Known.Zero & HiMask) != HiMask) {
        return SDValue();
      }
    }
  SDValue Res = DAG.getZExtOrTrunc(
                  DAG.getNode(Z80ISD::MLT, DL, MVT::i16,
                              EmitPair(DL, InOps[0], InOps[1], DAG)), DL, VT);
  if (NegRes) {
    Res = DAG.getNode(ISD::SUB, DL, VT, DAG.getConstant(0, DL, VT), Res);
  }
  return Res;
}

SDValue Z80TargetLowering::LowerGlobalAddress(GlobalAddressSDNode *Node,
                                              SelectionDAG &DAG) const {
  SDLoc DL(Node);
  auto PtrVT = getPointerTy(DAG.getDataLayout());
  SDValue Res = DAG.getTargetGlobalAddress(Node->getGlobal(), DL, PtrVT,
                                           Node->getOffset());
  return DAG.getNode(Z80ISD::Wrapper, DL, PtrVT, Res);
}

SDValue Z80TargetLowering::LowerExternalSymbol(ExternalSymbolSDNode *Node,
                                               SelectionDAG &DAG) const {
  SDLoc DL(Node);
  auto PtrVT = getPointerTy(DAG.getDataLayout());
  SDValue Res = DAG.getTargetExternalSymbol(Node->getSymbol(), PtrVT);
  return DAG.getNode(Z80ISD::Wrapper, DL, PtrVT, Res);
}

SDValue Z80TargetLowering::LowerBlockAddress(BlockAddressSDNode *Node,
                                             SelectionDAG &DAG) const {
  SDLoc DL(Node);
  auto PtrVT = getPointerTy(DAG.getDataLayout());
  SDValue Res = DAG.getTargetBlockAddress(Node->getBlockAddress(), PtrVT);
  return DAG.getNode(Z80ISD::Wrapper, DL, PtrVT, Res);
}

SDValue Z80TargetLowering::LowerLoad(LoadSDNode *Node,
                                     SelectionDAG &DAG) const {
  SDLoc DL(Node);
  SDValue Ch = Node->getChain();
  SDValue Ptr = Node->getBasePtr();
  const MachinePointerInfo &MPI = Node->getPointerInfo();
  unsigned Alignment = Node->getAlignment();
  MachineMemOperand *MMO = Node->getMemOperand();
  AAMDNodes AAInfo = Node->getAAInfo();
  if (Node->getExtensionType() == ISD::EXTLOAD)
    return DAG.getLoad(Node->getValueType(0), DL, Ch, Ptr, MPI, Alignment,
                       MMO->getFlags(), AAInfo);
  unsigned RC, LoTy, LoIdx, HiTy, HiIdx, HiOff;
  bool Split = Z80::splitReg(MMO->getSize(), MVT::i8, // MVT::i16,
                             RC, LoTy, LoIdx, HiTy, HiIdx, HiOff
                             /*Subtarget.hasEZ80Ops()*/);
  assert(Split && "Can only custom lower splittable loads");
  SDValue Lo = DAG.getLoad(MVT::SimpleValueType(LoTy), DL, Ch, Ptr, MPI,
                           Alignment, MMO->getFlags(), AAInfo);
  Ptr = DAG.getMemBasePlusOffset(Ptr, HiOff, DL);
  SDValue Hi = DAG.getLoad(MVT::SimpleValueType(HiTy), DL, Ch, Ptr,
                           MPI.getWithOffset(HiOff), MinAlign(Alignment, HiOff),
                           MMO->getFlags(), AAInfo);
  Ch = DAG.getNode(ISD::TokenFactor, DL, MVT::Other,
                   Lo.getValue(1), Hi.getValue(1));
  SDValue Res = EmitPair(DL, Hi, Lo, DAG);
  return DAG.getMergeValues({ Res, Ch }, DL);
}

SDValue Z80TargetLowering::LowerStore(StoreSDNode *Node,
                                      SelectionDAG &DAG) const {
  SDLoc DL(Node);
  SDValue Ch = Node->getChain();
  SDValue Ptr = Node->getBasePtr();
  SDValue Val = Node->getValue();
  const MachinePointerInfo &MPI = Node->getPointerInfo();
  unsigned Alignment = Node->getAlignment();
  MachineMemOperand *MMO = Node->getMemOperand();
  AAMDNodes AAInfo = Node->getAAInfo();
  unsigned RC, LoTy, LoIdx, HiTy, HiIdx, HiOff;
  bool Split = Z80::splitReg(MMO->getSize(), MVT::i8, // MVT::i16,
                             RC, LoTy, LoIdx, HiTy, HiIdx, HiOff);
  //Subtarget.has16BitEZ80Ops());
  assert(Split && "Can only custom lower splittable stores");
  SDValue Lo = EmitExtractSubreg(LoIdx, DL, Val, DAG);
  Lo = DAG.getStore(Ch, DL, Lo, Ptr, MPI, Alignment, MMO->getFlags(), AAInfo);
  Ptr = DAG.getMemBasePlusOffset(Ptr, HiOff, DL);
  SDValue Hi = EmitExtractSubreg(HiIdx, DL, Val, DAG);
  Hi = DAG.getStore(Ch, DL, Hi, Ptr, MPI.getWithOffset(HiOff),
                    MinAlign(Alignment, HiOff), MMO->getFlags(), AAInfo);
  Ch = DAG.getNode(ISD::TokenFactor, DL, MVT::Other,
                   Lo.getValue(0), Hi.getValue(0));
  return Ch;
}

SDValue Z80TargetLowering::LowerVAStart(SDValue Op, SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  Z80MachineFunctionInfo *FuncInfo = MF.getInfo<Z80MachineFunctionInfo>();
  SDLoc DL(Op);
  EVT PtrVT = getPointerTy(MF.getDataLayout());
  SDValue FR = DAG.getFrameIndex(FuncInfo->getVarArgsFrameIndex(), PtrVT);
  const Value *SV = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  return DAG.getStore(Op.getOperand(0), DL, FR, Op.getOperand(1),
                      MachinePointerInfo(SV));
}

SDValue Z80TargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const {
  LLVM_DEBUG(dbgs() << "LowerOperation: "; Op->dump(&DAG));
  assert(Op.getResNo() == 0);
  LLVM_DEBUG(dbgs() << "Opcode = " << Op.getOpcode());
  switch (Op.getOpcode()) {
  default: llvm_unreachable("Don't know how to lower this operation.");
  case ISD::BR_CC:          return LowerBR_CC(Op, DAG);
//case ISD::SETCC:          return LowerSETCC(Op, DAG);
  case ISD::SELECT_CC:      return LowerSELECT_CC(Op, DAG);
//case ISD::ADD:
//case ISD::SUB:            return LowerAddSub(Op, DAG);
  case ISD::AND:
  case ISD::OR:
  case ISD::XOR:            return LowerBitwise(Op, DAG);
  case ISD::SHL:
  case ISD::SRA:
  case ISD::SRL:
  case ISD::ROTL:
  case ISD::ROTR:           return LowerShift(Op, DAG);
  case ISD::ANY_EXTEND:     return LowerAnyExtend(Op, DAG);
  case ISD::ZERO_EXTEND:    return LowerZeroExtend(Op, DAG);
  case ISD::SIGN_EXTEND:    return LowerSignExtend(Op, DAG);
  case ISD::MUL:            return LowerMul(Op, DAG);
  case ISD::GlobalAddress:  return LowerGlobalAddress(
                                       cast<GlobalAddressSDNode>(Op), DAG);
  case ISD::ExternalSymbol: return LowerExternalSymbol(
                                       cast<ExternalSymbolSDNode>(Op), DAG);
  case ISD::BlockAddress:   return LowerBlockAddress(
                                       cast<BlockAddressSDNode>(Op), DAG);
  case ISD::LOAD:           return LowerLoad(cast<LoadSDNode>(Op), DAG);
  case ISD::STORE:          return LowerStore(cast<StoreSDNode>(Op), DAG);
  case ISD::VASTART:        return LowerVAStart(Op, DAG);
  }
}

// Old stuff

SDValue Z80TargetLowering::EmitCmp(SDValue LHS, SDValue RHS, SDValue &TargetCC,
                                   ISD::CondCode CC, const SDLoc &DL,
                                   SelectionDAG &DAG) const {
  EVT VT = LHS.getValueType();
  assert(VT == RHS.getValueType() && "Types should match");
  assert(VT.isScalarInteger() && "Unhandled type");
  if (isa<ConstantSDNode>(LHS)) {
    std::swap(LHS, RHS);
    CC = getSetCCSwappedOperands(CC);
  }
  ConstantSDNode *Const = dyn_cast<ConstantSDNode>(RHS);
  int32_t SignVal = 1 << (VT.getSizeInBits() - 1), ConstVal;
  if (Const) {
    ConstVal = Const->getSExtValue();
  }
  Z80::CondCode TCC = Z80::COND_INVALID;
  unsigned Opc = Z80ISD::SUB;
  switch (CC) {
  default: llvm_unreachable("Invalid integer condition");
  case ISD::SETEQ:
    TCC = Z80::COND_Z;
    break;
  case ISD::SETNE:
    TCC = Z80::COND_NZ;
    break;
  case ISD::SETULE:
    if (Const) {
      assert(ConstVal != -1 && "Unexpected always true condition");
      ++ConstVal;
      TCC = Z80::COND_C;
      break;
    }
    Const = nullptr;
    std::swap(LHS, RHS);
    LLVM_FALLTHROUGH;
  case ISD::SETUGE:
    TCC = Z80::COND_NC;
    break;
  case ISD::SETUGT:
    if (Const) {
      assert(ConstVal != -1 && "Unexpected always false condition");
      ++ConstVal;
      TCC = Z80::COND_NC;
      break;
    }
    Const = nullptr;
    std::swap(LHS, RHS);
    LLVM_FALLTHROUGH;
  case ISD::SETULT:
    TCC = Z80::COND_C;
    break;
  case ISD::SETLE:
    Const = nullptr;
    std::swap(LHS, RHS);
    LLVM_FALLTHROUGH;
  case ISD::SETGE:
    LHS = EmitFlipSign(DL, LHS, DAG);
    if (Const) {
      ConstVal ^= SignVal;
    } else {
      RHS = EmitFlipSign(DL, RHS, DAG);
    }
    TCC = Z80::COND_NC;
    break;
  case ISD::SETGT:
    Const = nullptr;
    std::swap(LHS, RHS);
    LLVM_FALLTHROUGH;
  case ISD::SETLT:
    LHS = EmitFlipSign(DL, LHS, DAG);
    if (Const) {
      ConstVal ^= SignVal;
    } else {
      RHS = EmitFlipSign(DL, RHS, DAG);
    }
    TCC = Z80::COND_C;
    break;
  }
  switch (TCC) {
  default: llvm_unreachable("Invalid target condition");
  case Z80::COND_Z:
  case Z80::COND_NZ:
    break;
  case Z80::COND_C:
  case Z80::COND_NC:
    // For word compares with constants, adding the negative is more optimal.
    if (VT != MVT::i8 && Const) {
      Opc = Z80ISD::ADD;
      TCC = Z80::GetOppositeBranchCondition(TCC);
      ConstVal = -ConstVal;
      if (ConstVal == SignVal) {
        RHS = LHS;
        Const = nullptr;
      }
    }
    break;
  }
  if (Const) {
    RHS = DAG.getConstant(ConstVal, DL, VT);
  }
  TargetCC = DAG.getConstant(TCC, DL, MVT::i8);
  return DAG.getNode(Opc, DL, DAG.getVTList(VT, MVT::i8), LHS, RHS).getValue(1);
}

// Old SelectionDAG helpers
SDValue Z80TargetLowering::EmitExtractSubreg(unsigned Idx, const SDLoc &DL,
                                             SDValue Op,
                                             SelectionDAG &DAG) const {
  //if (Op.getOpcode() != ISD::Constant)
  //  return DAG.getTargetExtractSubreg(Idx, DL, VT, Op);
  const TargetRegisterInfo *TRI = Subtarget.getRegisterInfo();
  return DAG.getNode(ISD::TRUNCATE, DL,
                     MVT::getIntegerVT(TRI->getSubRegIdxSize(Idx)),
                     DAG.getNode(ISD::SRL, DL, Op.getValueType(), Op,
                                 DAG.getConstant(TRI->getSubRegIdxOffset(Idx),
                                                 DL, MVT::i8)));
}
SDValue Z80TargetLowering::EmitInsertSubreg(unsigned Idx, const SDLoc &DL,
                                            MVT VT, SDValue Op,
                                            SelectionDAG &DAG) const {
  const TargetRegisterInfo *TRI = Subtarget.getRegisterInfo();
  assert(Op.getValueSizeInBits() == TRI->getSubRegIdxSize(Idx));
  return DAG.getNode(ISD::SHL, DL, VT,
                     DAG.getNode(ISD::ZERO_EXTEND, DL, VT, Op),
                     DAG.getConstant(TRI->getSubRegIdxOffset(Idx), DL,
                                     MVT::i8));
}

SDValue Z80TargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG) const {
  SDValue Chain = Op.getOperand(0);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
  SDValue LHS   = Op.getOperand(2);
  SDValue RHS   = Op.getOperand(3);
  SDValue Dest  = Op.getOperand(4);
  SDLoc DL(Op);

  SDValue TargetCC;
  SDValue Flags = EmitCmp(LHS, RHS, TargetCC, CC, DL, DAG);

  return DAG.getNode(Z80ISD::BRCOND, DL, MVT::Other,
                     Chain, Dest, TargetCC, Flags);
}

SDValue Z80TargetLowering::LowerSETCC(SDValue Op, SelectionDAG &DAG) const {
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDLoc DL(Op);

  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(2))->get();
  SDValue TargetCC;
  SDValue Flags = EmitCmp(LHS, RHS, TargetCC, CC, DL, DAG);

  EVT VT = Op.getValueType();
  return DAG.getNode(Z80ISD::SELECT, DL, DAG.getVTList(VT, MVT::Glue),
                     DAG.getConstant(1, DL, VT), DAG.getConstant(0, DL, VT),
                     TargetCC, Flags);
}

SDValue Z80TargetLowering::LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const {
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue TV  = Op.getOperand(2);
  SDValue FV  = Op.getOperand(3);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(4))->get();
  SDLoc DL(Op);

  SDValue TargetCC;
  SDValue Flag = EmitCmp(LHS, RHS, TargetCC, CC, DL, DAG);

  return DAG.getNode(Z80ISD::SELECT, DL,
                     DAG.getVTList(Op.getValueType(), MVT::Glue), TV, FV,
                     TargetCC, Flag);
}

SDValue Z80TargetLowering::LowerLibCall(
  RTLIB::Libcall LC8, RTLIB::Libcall LC16,
  RTLIB::Libcall LC32, SDValue Op, SelectionDAG &DAG) const {
  RTLIB::Libcall LC;
  assert(Op.getSimpleValueType().isInteger() && "Can not lower this type");
  switch (Op.getValueSizeInBits()) {
  default: llvm_unreachable("Can not lower this type");
  case 8:  LC = LC8;  break;
  case 16: LC = LC16; break;
  //case 24: LC = LC24; break;
  case 32: LC = LC32; break;
  }

  SmallVector<SDValue, 2> Ops;
  Ops.reserve(Op.getNumOperands());
  for (unsigned I = 0, E = Op.getNumOperands(); I != E; ++I) {
    Ops.push_back(Op.getOperand(I));
  }
  return makeLibCall(DAG, LC, Op.getValueType(), Ops, false, SDLoc(Op)).first;
}

bool Z80TargetLowering::isOffsetFoldingLegal(
  const GlobalAddressSDNode * /*GA*/) const {
  return true;
}

/// Return true if the addressing mode represented by AM is legal for this
/// target, for a load/store of the specified type.
bool Z80TargetLowering::isLegalAddressingMode(const DataLayout &DL,
                                              const AddrMode &AM, Type *Ty,
                                              unsigned AS,
                                              Instruction *I) const {
  if (AM.Scale) {
    return false;
  }

  if (AM.BaseGV && AM.HasBaseReg) {
    return false;
  }

  if (AM.BaseGV) {
    return AM.BaseOffs == 0;
  }

  if (AM.HasBaseReg) {
    return isInt<8>(AM.BaseOffs);
  }

  return false;
}

bool Z80TargetLowering::isLegalICmpImmediate(int64_t Imm) const {
  return isInt<8>(Imm);
}
bool Z80TargetLowering::isLegalAddImmediate(int64_t Imm) const {
  return isInt<8>(Imm);
}

bool Z80TargetLowering::isTruncateFree(Type *Ty1, Type *Ty2) const {
  if (!Ty1->isIntegerTy() || !Ty2->isIntegerTy()) {
    return false;
  }
  return Ty1->getPrimitiveSizeInBits() > Ty2->getPrimitiveSizeInBits();
}
bool Z80TargetLowering::isTruncateFree(EVT VT1, EVT VT2) const {
  if (!VT1.isInteger() || !VT2.isInteger()) {
    return false;
  }
  return VT1.getSizeInBits() > VT2.getSizeInBits();
}

#if 0
bool Z80TargetLowering::isZExtFree(Type *Ty1, Type *Ty2) const {
  // ez80 implicitly zero-extends 16-bit results in 24-bit registers.
  return Ty1->isIntegerTy(16) && Ty2->isIntegerTy(24) && Subtarget.is24Bit();
}
bool Z80TargetLowering::isZExtFree(EVT VT1, EVT VT2) const {
  // ez80 implicitly zero-extends 16-bit results in 24-bit registers.
  return VT1 == MVT::i16 && VT2 == MVT::i24 && Subtarget.is24Bit();
}

bool Z80TargetLowering::isNarrowingProfitable(EVT VT1, EVT VT2) const {
  // i16 instructions are longer (prefixed) and slower.
  return !(VT1 == MVT::i24 && VT2 == MVT::i16);
}
#endif // 0

/// \brief Returns true if it is beneficial to convert a load of a constant
/// to just the constant itself.
bool Z80TargetLowering::shouldConvertConstantLoadToIntImm(const APInt &Imm,
                                                          Type *Ty) const {
  assert(Ty->isIntegerTy());
  // Immediate loads are always the same size (or smaller) and faster than
  // a constant load (from an immediate address).
  return true;
}

SDValue Z80TargetLowering::combineCopyFromReg(SDNode *N,
                                              DAGCombinerInfo &DCI) const {
  SDLoc DL(N);
  SelectionDAG &DAG = DCI.DAG;
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  RegisterSDNode *Reg = cast<RegisterSDNode>(N->getOperand(1).getNode());
  if (Reg->getReg() == getStackPointerRegisterToSaveRestore() &&
      !MFI.hasVarSizedObjects()) {
    SDValue Ops[] = { DAG.getTargetIndex(Reg->getReg(), N->getValueType(0)),
                      N->getOperand(0)
                    };
    return DAG.getMergeValues(Ops, DL);
  }
  return SDValue();
}

SDValue Z80TargetLowering::combineStore(StoreSDNode *N,
                                        DAGCombinerInfo &DCI) const {
  SelectionDAG &DAG = DCI.DAG;
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineMemOperand *MMO = N->getMemOperand();
  const PseudoSourceValue *PSV = MMO->getPseudoValue();
  if (PSV && PSV->isStack()) {
    dbgs() << DCI.getDAGCombineLevel() << ':' << MFI.hasVarSizedObjects() << '\n';
  }
  return SDValue();
}

SDValue Z80TargetLowering::combineINSERT_SUBREG(SDNode *N,
                                                DAGCombinerInfo &DCI) const {
  return SDValue();
}

// This code is from DAGCombiner::visitADDC
SDValue Z80TargetLowering::combineADD(SDNode *N, DAGCombinerInfo &DCI) const {
  SelectionDAG &DAG = DCI.DAG;
  EVT VT = N->getValueType(0);
  SDValue N0 = N->getOperand(0);
  SDValue N1 = N->getOperand(1);

  // If the flag result is dead, turn this into an ADD.
  if (!N->hasAnyUseOfValue(1))
    return DCI.CombineTo(N, DAG.getNode(ISD::ADD, SDLoc(N), VT, N0, N1),
                         DAG.getNode(ISD::CARRY_FALSE, SDLoc(N), MVT::Glue));

  // canonicalize constant to RHS.
  ConstantSDNode *N0C = dyn_cast<ConstantSDNode>(N0);
  ConstantSDNode *N1C = dyn_cast<ConstantSDNode>(N1);
  if (N0C && !N1C) {
    return DAG.getNode(N->getOpcode(), SDLoc(N), N->getVTList(), N1, N0);
  }

  // fold (addc x, 0) -> x + no carry out
  if (isNullConstant(N1))
    return DCI.CombineTo(N, N0, DAG.getNode(ISD::CARRY_FALSE,
                                            SDLoc(N), MVT::Glue));
  return SDValue();
}

// This code is from DAGCombiner::visitSUBC
SDValue Z80TargetLowering::combineSUB(SDNode *N, DAGCombinerInfo &DCI) const {
  SelectionDAG &DAG = DCI.DAG;
  EVT VT = N->getValueType(0);
  SDValue N0 = N->getOperand(0);
  SDValue N1 = N->getOperand(1);

  // If the flag result is dead, turn this into an ADD.
  if (!N->hasAnyUseOfValue(1))
    return DCI.CombineTo(N, DAG.getNode(ISD::SUB, SDLoc(N), VT, N0, N1),
                         DAG.getNode(ISD::CARRY_FALSE, SDLoc(N), MVT::Glue));

  // fold (subc x, x) -> 0 + no borrow
  if (N0 == N1)
    return DCI.CombineTo(N, DAG.getConstant(0, SDLoc(N), VT),
                         DAG.getNode(ISD::CARRY_FALSE, SDLoc(N), MVT::Glue));

  // fold (subc x, 0) -> x + no borrow
  if (isNullConstant(N1))
    return DCI.CombineTo(N, N0, DAG.getNode(ISD::CARRY_FALSE, SDLoc(N),
                                            MVT::Glue));

  return SDValue();
}

static SDValue combineExtractSubreg(SDNode *N, SelectionDAG &DAG,
                                    const Z80Subtarget &Subtarget) {
  const TargetRegisterInfo *TRI = Subtarget.getRegisterInfo();
  EVT VT = N->getValueType(0);
  SDValue N0 = N->getOperand(0);
  EVT N0VT = N0.getValueType();
  unsigned Off = TRI->getSubRegIdxOffset(N->getConstantOperandVal(1));
  SDLoc DL(N);
  if (N0->getOpcode() == ISD::Constant)
    return DAG.getNode(ISD::TRUNCATE, DL, VT,
                       DAG.getNode(ISD::SRL, DL, N0VT, N0,
                                   DAG.getConstant(Off, DL, N0VT)));
  if (!N0.hasOneUse()) {
    return SDValue();
  }
  if (auto Load = dyn_cast<LoadSDNode>(N0))
    return DAG.getLoad(VT, DL, Load->getChain(),
                       DAG.getMemBasePlusOffset(Load->getBasePtr(), Off, DL),
                       Load->getPointerInfo().getWithOffset(Off),
                       Load->getAlignment(), Load->getMemOperand()->getFlags(),
                       Load->getAAInfo());
  return SDValue();
}

static SDValue combineMul(SDNode *N, SelectionDAG &DAG,
                          const Z80Subtarget &Subtarget) {
  EVT VT = N->getValueType(0);
  SDValue N0 = N->getOperand(0);
  SDValue N1 = N->getOperand(1);
  SDLoc DL(N);
  bool OptSize = DAG.getMachineFunction().getFunction().getAttributes()
                 .hasAttribute(AttributeList::FunctionIndex, Attribute::OptimizeForSize);
  if (OptSize) {
    return SDValue();
  }
  //if (VT == MVT::i8 && Subtarget.hasZ180Ops())
  //  return SDValue();
  if (auto C1 = dyn_cast<ConstantSDNode>(N1)) {
    APInt Const = C1->getAPIntValue();
    bool Odd = Const[0];
    Const.lshrInPlace(1);
    SDValue Half = DAG.getNode(ISD::MUL, DL, VT, N0,
                               DAG.getConstant(Const, DL, VT));
    SDValue Double = DAG.getNode(ISD::ADD, DL, VT, Half, Half);
    if (Odd) {
      Double = DAG.getNode(Z80ISD::ADD, DL, VT, Double, N0);
    }
    return Double;
  }
  return SDValue();
}

#if 0
static SDValue implicitlyClearTop(SDValue &Op, SelectionDAG &DAG) {
  SDLoc DL(Op);
  unsigned NewOpc;
  switch (Op.getOpcode()) {
  default: return SDValue();
  case ISD::ADD: NewOpc = Z80ISD::ADD; break;
  case ISD::SUB: NewOpc = Z80ISD::SUB; break;
  }
  Op = DAG.getNode(NewOpc, DL, DAG.getVTList(MVT::i16, MVT::i8),
                   DAG.getNode(ISD::TRUNCATE, DL, MVT::i16, Op.getOperand(0)),
                   DAG.getNode(ISD::TRUNCATE, DL, MVT::i16, Op.getOperand(1)));
  return SDValue(DAG.getMachineNode(TargetOpcode::SUBREG_TO_REG, DL,
                                    MVT::i16, // MVT::i24,
                                    DAG.getTargetConstant(0, DL, MVT::i16 /*MVT::i24*/), Op,
                                    DAG.getTargetConstant(Z80::sub_short,
                                                          DL, MVT::i32)), 0);
}
#endif // 0

static SDValue combineAnd(SDNode *N, SelectionDAG &DAG) {
  SDLoc DL(N);
  EVT VT = N->getValueType(0);
  SDValue Val = N->getOperand(0);
  //if (VT == MVT::i24 && Val.hasOneUse())
  //  if (auto Const = dyn_cast<ConstantSDNode>(N->getOperand(1)))
  //    if ((Const->getZExtValue() & 0xFF0000) == 0)
  //      return implicitlyClearTop(Val, DAG);
  return SDValue();
}

static SDValue combineZeroExtend(SDNode *N,
                                 TargetLowering::DAGCombinerInfo &DCI) {
  //SelectionDAG &DAG = DCI.DAG;
  SDLoc DL(N);
  EVT VT = N->getValueType(0);
  SDValue Val = N->getOperand(0);
  EVT ValVT = Val.getValueType();
  //if (VT == MVT::i24 && ValVT == MVT::i16) {
  //  if (SDValue Res = implicitlyClearTop(Val, DAG)) {
  //    DCI.CombineTo(N->getOperand(0).getNode(), Val);
  //    return Res;
  //  }
  //}
  return SDValue();
}

static SDValue combineTruncate(SDNode *N, SelectionDAG &DAG) {
  SDLoc DL(N);
  EVT VT = N->getValueType(0);
  SDValue Val = N->getOperand(0);
  EVT ValVT = Val.getValueType();
  if (VT == MVT::i8 && Val.hasOneUse())
    switch (Val.getOpcode()) {
    case ISD::SRL:
    case ISD::SRA:
#if 0
      if (auto SA = dyn_cast<ConstantSDNode>(Val.getOperand(1)))
        if (ValVT == MVT::i24 && SA->getZExtValue() <= 8)
          return DAG.getNode(ISD::TRUNCATE, DL, VT,
                             DAG.getNode(ISD::SRL, DL, MVT::i16,
                                         DAG.getNode(ISD::TRUNCATE, DL,
                                                     MVT::i16,
                                                     Val.getOperand(0)),
                                         Val.getOperand(1)));
#endif // 0
      break;
    case Z80ISD::SEXT:
      return DAG.getNode(Z80ISD::SEXT, DL, VT, Val.getOperand(0));
    }
  return SDValue();
}

static SDValue combineSub(SDNode *N, TargetLowering::DAGCombinerInfo &DCI) {
  SelectionDAG &DAG = DCI.DAG;
  SDLoc DL(N);
  SDValue Val0(N, 0);
  if (Val0.use_empty())
    return DCI.CombineTo(N, Val0,
                         DAG.getNode(Z80ISD::CP, DL, N->getValueType(1),
                                     N->getOperand(0), N->getOperand(1)));
  return SDValue();
}

static SDValue combineSExt(SDNode *N, SelectionDAG &DAG,
                           const Z80Subtarget &Subtarget) {
  EVT VT = N->getValueType(0);
  SDValue N0 = N->getOperand(0);
  SDLoc DL(N);
  //if (VT == MVT::i16 && Subtarget.is24Bit())
  //  return DAG.getNode(ISD::TRUNCATE, DL, VT,
  //                     DAG.getNode(Z80ISD::SEXT, DL, MVT::i24, N0));
  return SDValue();
}

SDValue Z80TargetLowering::PerformDAGCombine(SDNode *N,
                                             DAGCombinerInfo &DCI) const {
  SelectionDAG &DAG = DCI.DAG;
  if (N->isMachineOpcode())
    switch (N->getMachineOpcode()) {
    default: return SDValue();
    case TargetOpcode::EXTRACT_SUBREG:
      return combineExtractSubreg(N, DCI.DAG, Subtarget);
    }
  switch (N->getOpcode()) {
  default:               break;
  case ISD::MUL:         return combineMul(N, DCI.DAG, Subtarget);
  case ISD::AND:         return combineAnd(N, DCI.DAG);
  case ISD::ZERO_EXTEND: return combineZeroExtend(N, DCI);
  case ISD::TRUNCATE:    return combineTruncate(N, DCI.DAG);
  case Z80ISD::SUB:      return combineSub(N, DCI);
  case Z80ISD::SEXT:     return combineSExt(N, DCI.DAG, Subtarget);
  }
//switch (N->getOpcode()) {
//case ISD::MUL:         return combineMul(N, DCI);
//case ISD::CopyFromReg: return combineCopyFromReg(N, DCI);
//case ISD::STORE:       return combineStore(cast<StoreSDNode>(N), DCI);
//case TargetOpcode::INSERT_SUBREG:  return combineINSERT_SUBREG(N, DCI);
//case Z80ISD::ADD:                  return combineADD(N, DCI);
//case Z80ISD::SUB:                  return combineSUB(N, DCI);
//}
  return SDValue();
}

/// Return true if the target has native support for the specified value type
/// and it is 'desirable' to use the type for the given node type. e.g. On ez80
/// i16 is legal, but undesirable since i16 instruction encodings are longer and
/// slower.
bool Z80TargetLowering::isTypeDesirableForOp(unsigned Opc, EVT VT) const {
  if (!isTypeLegal(VT)) {
    return false;
  }
#if 0
  if (Subtarget.is16Bit()) {
    return true;
  }

  switch (Opc) {
  default:
  case ISD::SIGN_EXTEND:
  case ISD::ZERO_EXTEND:
  case ISD::ANY_EXTEND:
    return true;
  case ISD::LOAD:
  case ISD::STORE:
  case ISD::ADD:
  case ISD::SUB:
    return VT != MVT::i16;
  case ISD::MUL:
  case ISD::AND:
  case ISD::OR:
  case ISD::XOR:
  case ISD::SHL:
  case ISD::SRA:
  case ISD::SRL:
    return VT != MVT::i24;
  }
#else
  return true;
#endif // 0
}

/// Return true if x op y -> (SrcVT)((DstVT)x op (DstVT)y) is beneficial.
bool Z80TargetLowering::isDesirableToShrinkOp(unsigned Opc, EVT SrcVT,
                                              EVT DstVT) const {
  if (!isTypeLegal(DstVT)) {
    return false;
  }
  switch (Opc) {
  default:
    return false;
  case ISD::ADD:
  case ISD::SUB:
  case ISD::ADDC:
  case ISD::SUBC:
  case ISD::ADDE:
  case ISD::SUBE:
    // These require a .sis suffix for i24 -> i16
    // $TODO: Are these for Z80 as well?
    return true; // DstVT != MVT::i16 || Subtarget.is16Bit();
  case ISD::MUL:
  case ISD::AND:
  case ISD::OR:
  case ISD::XOR:
  case ISD::SHL:
  case ISD::SRA:
  case ISD::SRL:
  case ISD::ROTL:
  case ISD::ROTR:
    // These are more expensive on larger types, so always shrink.
    return true;
  }
}

/// This method query the target whether it is beneficial for dag combiner to
/// promote the specified node. If true, it should return the desired promotion
/// type by reference.
bool Z80TargetLowering::IsDesirableToPromoteOp(SDValue Op, EVT &PVT) const {
#if 0
  if (isDesirableToShrinkOp(Op.getOpcode(), MVT::i24, Op.getValueType())) {
    return false;
  }
  PVT = MVT::i24;
  return true;
#else
  return false;
#endif // 0
}

MachineBasicBlock *
Z80TargetLowering::EmitInstrWithCustomInserter(MachineInstr &MI,
                                               MachineBasicBlock *BB) const {
  switch (MI.getOpcode()) {
  default: llvm_unreachable("Unexpected instr type to insert");
  /*case Z80::Sub016:
  case Z80::Sub024:
    return EmitLoweredSub0(MI, BB);
  case Z80::Sub16:
  case Z80::Sub24:
    return EmitLoweredSub(MI, BB);
  case Z80::Cp16a0:
  case Z80::Cp24a0:
    return EmitLoweredCmp0(MI, BB);
  case Z80::Cp16ao:
  case Z80::Cp24ao:
  return EmitLoweredCmp(MI, BB);*/
  case Z80::Select8:
  case Z80::Select16:
    //case Z80::Select24:
    return EmitLoweredSelect(MI, BB);
  case Z80::SExt8:
  case Z80::SExt16:
    //case Z80::SExt24:
    return EmitLoweredSExt(MI, BB);
  }
}

#if 1
void Z80TargetLowering::AdjustInstrPostInstrSelection(MachineInstr &MI,
                                                      SDNode *Node) const {
  switch (MI.getOpcode()) {
  default: llvm_unreachable("Unexpected instr type to insert");
  case Z80::ADJCALLSTACKUP16:
  //case Z80::ADJCALLSTACKUP24:
  case Z80::ADJCALLSTACKDOWN16:
    //case Z80::ADJCALLSTACKDOWN24:
    return AdjustAdjCallStack(MI);
  }
}

void Z80TargetLowering::AdjustAdjCallStack(MachineInstr &MI) const {
  //bool Is24Bit = MI.getOpcode() == Z80::ADJCALLSTACKUP24 ||
  //               MI.getOpcode() == Z80::ADJCALLSTACKDOWN24;
  assert((/*Is24Bit ||*/ MI.getOpcode() == Z80::ADJCALLSTACKUP16 ||
                         MI.getOpcode() == Z80::ADJCALLSTACKDOWN16) &&
         "Unexpected opcode");
  MachineRegisterInfo &MRI = MI.getParent()->getParent()->getRegInfo();
  unsigned Reg = MRI.createVirtualRegister(&Z80::AIR16RegClass);
  MachineInstrBuilder(*MI.getParent()->getParent(), MI)
  .addReg(Reg, RegState::ImplicitDefine | RegState::Dead);
  LLVM_DEBUG(MI.dump());
}
#endif // 0

MachineBasicBlock *
Z80TargetLowering::EmitLoweredSub(MachineInstr &MI,
                                  MachineBasicBlock *BB) const {
  //bool Is24Bit = MI.getOpcode() == Z80::SUB24ao;
  assert((/*Is24Bit ||*/ MI.getOpcode() == Z80::SUB16ao) && "Unexpected opcode");
  const TargetInstrInfo *TII = Subtarget.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  LLVM_DEBUG(BB->dump());
  BuildMI(*BB, MI, DL, TII->get(Z80::RCF));
  BuildMI(*BB, MI, DL, TII->get(Z80::SBC16ao))
  .addReg(MI.getOperand(0).getReg());
  MI.eraseFromParent();
  LLVM_DEBUG(BB->dump());
  return BB;
}

MachineBasicBlock *
Z80TargetLowering::EmitLoweredCmp(MachineInstr &MI,
                                  MachineBasicBlock *BB) const {
  //bool Is24Bit = MI.getOpcode() == Z80::CP24ao;
  assert((/*Is24Bit ||*/ MI.getOpcode() == Z80::CP16ao) && "Unexpected opcode");
  const TargetInstrInfo *TII = Subtarget.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  BuildMI(*BB, MI, DL, TII->get(Z80::RCF));
  BuildMI(*BB, MI, DL, TII->get(Z80::SBC16ao))
  .addReg(MI.getOperand(0).getReg());
  BuildMI(*BB, MI, DL, TII->get(Z80::ADD16ao),
          Z80::HL).addReg(Z80::HL)
  .addReg(MI.getOperand(0).getReg());
  MI.eraseFromParent();
  LLVM_DEBUG(BB->dump());
  return BB;
}

MachineBasicBlock *
Z80TargetLowering::EmitLoweredCmp0(MachineInstr &MI,
                                   MachineBasicBlock *BB) const {
  //bool Is24Bit = MI.getOpcode() == Z80::CP24a0;
  assert((/*Is24Bit ||*/ MI.getOpcode() == Z80::CP16a0) && "Unexpected opcode");
  const TargetInstrInfo *TII = Subtarget.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  BuildMI(*BB, MI, DL, TII->get(Z80::ADD16ao),
          Z80::HL).addReg(Z80::HL)
  .addReg(MI.getOperand(0).getReg());
  BuildMI(*BB, MI, DL, TII->get(Z80::RCF));
  BuildMI(*BB, MI, DL, TII->get(Z80::SBC16ao))
  .addReg(MI.getOperand(0).getReg());
  MI.eraseFromParent();
  LLVM_DEBUG(BB->dump());
  return BB;
}

MachineBasicBlock *
Z80TargetLowering::EmitLoweredSelect(MachineInstr &MI,
                                     MachineBasicBlock *BB) const {
  const TargetInstrInfo *TII = Subtarget.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();

  // To "insert" a SELECT_CC instruction, we actually have to insert the
  // diamond control-flow pattern.  The incoming instruction knows the
  // destination vreg to set, the condition code register to branch on, the
  // true/false values to select between, and a branch opcode to use.
  const BasicBlock *LLVM_BB = BB->getBasicBlock();
  MachineFunction::iterator I = ++BB->getIterator();

  //  thisMBB:
  //  ...
  //   %FalseVal = ...
  //   cmpTY ccX, r1, r2
  //   bCC copy1MBB
  //   fallthrough --> copy0MBB
  MachineBasicBlock *thisMBB = BB;
  MachineFunction *F = BB->getParent();
  MachineBasicBlock *copy0MBB = F->CreateMachineBasicBlock(LLVM_BB);
  MachineBasicBlock *copy1MBB = F->CreateMachineBasicBlock(LLVM_BB);
  F->insert(I, copy0MBB);
  F->insert(I, copy1MBB);

  // Update machine-CFG edges by transferring all successors of the current
  // block to the new block which will contain the Phi node for the select.
  copy1MBB->splice(copy1MBB->begin(), BB,
                   std::next(MachineBasicBlock::iterator(MI)), BB->end());
  copy1MBB->transferSuccessorsAndUpdatePHIs(BB);
  // Next, add the true and fallthrough blocks as its successors.
  BB->addSuccessor(copy0MBB);
  BB->addSuccessor(copy1MBB);

  BuildMI(BB, DL, TII->get(Z80::JQCC)).addMBB(copy1MBB)
  .addImm(MI.getOperand(3).getImm());

  //  copy0MBB:
  //   %TrueVal = ...
  //   # fallthrough to copy1MBB
  BB = copy0MBB;

  // Update machine-CFG edges
  BB->addSuccessor(copy1MBB);

  //  copy1MBB:
  //   %Result = phi [ %FalseValue, copy0MBB ], [ %TrueValue, thisMBB ]
  //  ...
  BB = copy1MBB;
  BuildMI(*BB, BB->begin(), DL, TII->get(Z80::PHI),
          MI.getOperand(0).getReg())
  .addReg(MI.getOperand(1).getReg()).addMBB(thisMBB)
  .addReg(MI.getOperand(2).getReg()).addMBB(copy0MBB);

  MI.eraseFromParent();   // The pseudo instruction is gone now.
  LLVM_DEBUG(F->dump());
  return BB;
}

MachineBasicBlock *Z80TargetLowering::EmitLoweredSExt(
  MachineInstr &MI, MachineBasicBlock *BB) const {
  const TargetInstrInfo *TII = Subtarget.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  unsigned Opc, Reg;
  switch (MI.getOpcode()) {
  default: llvm_unreachable("Unexpected opcode");
  case Z80::SExt8:
    Opc = Z80::SBC8ar;
    Reg = Z80::A;
    break;
  case Z80::SExt16:
    Opc = Z80::SBC16aa;
    Reg = Z80::HL;
    break;
    //case Z80::SExt24:
    //  Opc = Z80::SBC24aa;
    //  Reg = Z80::UHL;
    //  break;
  }
  MachineInstrBuilder MIB = BuildMI(*BB, MI, DL, TII->get(Opc));
  MIB->findRegisterUseOperand(Reg)->setIsUndef();
  if (Reg == Z80::A) {
    MIB.addReg(Reg, RegState::Undef);
  }
  MI.eraseFromParent();
  return BB;
}

//===----------------------------------------------------------------------===//
//               Return Value Calling Convention Implementation
//===----------------------------------------------------------------------===//

#include "Z80GenCallingConv.inc"

CCAssignFn *Z80TargetLowering::getCCAssignFn(CallingConv::ID CallConv) const {
  //bool Is24Bit = Subtarget.is24Bit();
  switch (CallConv) {
  default: llvm_unreachable("Unsupported calling convention!");
  case CallingConv::C:
  case CallingConv::Fast:
  case CallingConv::PreserveAll:
    return CC_Z80_C;
    //case CallingConv::Z80_LibCall:
    //  return CC_EZ80_LC_AB;
    //case CallingConv::Z80_LibCall_AC:
    //  return CC_EZ80_LC_AC;
    //case CallingConv::Z80_LibCall_BC:
    //  return CC_EZ80_LC_BC;
    //case CallingConv::Z80_LibCall_C:
    //  return CC_EZ80_LC_C;
    //case CallingConv::Z80_LibCall_L:
    //  return CC_EZ80_LC_L;
  }
}
CCAssignFn *Z80TargetLowering::getRetCCAssignFn(CallingConv::ID CallConv)
const {
  //bool Is24Bit = Subtarget.is24Bit();
  switch (CallConv) {
  default: llvm_unreachable("Unsupported calling convention!");
  case CallingConv::C:
  case CallingConv::Fast:
  case CallingConv::PreserveAll:
  case CallingConv::Z80_LibCall:
  case CallingConv::Z80_LibCall_AC:
  case CallingConv::Z80_LibCall_BC:
  case CallingConv::Z80_LibCall_C:
    return RetCC_Z80_C;
    //case CallingConv::Z80_LibCall_L:
    //  return RetCC_EZ80_LC_L;
  }
}

SDValue Z80TargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                                     SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG                     = CLI.DAG;
  SDLoc &DL                             = CLI.DL;
  SmallVectorImpl<ISD::OutputArg> &Outs = CLI.Outs;
  SmallVectorImpl<SDValue> &OutVals     = CLI.OutVals;
  SmallVectorImpl<ISD::InputArg> &Ins   = CLI.Ins;
  SDValue Chain                         = CLI.Chain;
  SDValue Callee                        = CLI.Callee;
  CallingConv::ID CallConv              = CLI.CallConv;
  bool &IsTailCall                      = CLI.IsTailCall;
  bool IsVarArg                         = CLI.IsVarArg;

  const TargetRegisterInfo *TRI = Subtarget.getRegisterInfo();
  MachineFunction &MF = DAG.getMachineFunction();
  if (MF.getFunction().getFnAttribute("disable-tail-calls")
      .getValueAsString() == "true") {
    IsTailCall = false;
  }

  if (IsTailCall)
    IsTailCall = IsEligibleForTailCallOptimization(
                   Callee, CallConv, IsVarArg, CLI.RetTy, Outs, OutVals, Ins, DAG);

  // Analyze operands of the call, assigning locations to each operand.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeCallOperands(Outs, getCCAssignFn(CallConv));

  // Get a count of how many bytes are to be pushed on the stack.
  unsigned NumBytes = CCInfo.getAlignedCallFrameSize();
  MVT PtrVT = getPointerTy(DAG.getDataLayout());

  if (!IsTailCall) {
    Chain = DAG.getCALLSEQ_START(Chain, NumBytes, 0, DL);
  }

  SmallVector<std::pair<unsigned, SDValue>, 2> RegsToPass;
  const TargetRegisterInfo *RegInfo = Subtarget.getRegisterInfo();

  // Walk the register/memloc assignments, inserting copies/loads.
  for (unsigned I = ArgLocs.size(); I; --I) {
    ISD::OutputArg &OA = Outs[I - 1];
    CCValAssign &VA = ArgLocs[I - 1];
    unsigned Reg = VA.isRegLoc() ? VA.getLocReg() : Z80::NoRegister;
    EVT LocVT = VA.getLocVT();
    SDValue Val = OutVals[I - 1];
    // Don't copy to the full final register if any extending
    if (OA.Flags.isSplitEnd() && !OA.Flags.isZExt() && !OA.Flags.isSExt()) {
      unsigned RegBytes = OA.ArgVT.getStoreSize() - OA.PartOffset;
      if (LocVT.getStoreSize() != RegBytes) {
        unsigned Idx;
        switch (RegBytes) {
        default: llvm_unreachable("Unexpected final size");
        case 1: Idx = Z80::sub_low;   break;
        case 2: Idx = Z80::sub_short; break;
        }
        Reg = TRI->getSubReg(Reg, Idx);
        Val = DAG.getNode(ISD::ANY_EXTEND, DL, LocVT,
                          DAG.getNode(ISD::TRUNCATE, DL,
                                      MVT::getIntegerVT(8 * RegBytes), Val));
      }
    }
    // Promote values to the appropriate types.
    else if (VA.getLocInfo() == CCValAssign::AExt) {
      Val = DAG.getNode(ISD::ANY_EXTEND, DL, LocVT, Val);
    } else if (VA.getLocInfo() == CCValAssign::SExt) {
      Val = DAG.getNode(ISD::SIGN_EXTEND, DL, LocVT, Val);
    } else if (VA.getLocInfo() == CCValAssign::ZExt) {
      Val = DAG.getNode(ISD::ZERO_EXTEND, DL, LocVT, Val);
    } else if (VA.getLocInfo() == CCValAssign::BCvt) {
      Val = DAG.getBitcast(LocVT, Val);
    } else {
      assert(VA.getLocInfo() == CCValAssign::Full && "Unknown loc info!");
    }

    if (VA.isRegLoc()) {
      RegsToPass.push_back(std::make_pair(Reg, Val));
    } else if (!IsTailCall) {
      assert(VA.isMemLoc());
#if 1
      // $TODO: Check if this is correct
      Chain = DAG.getMemIntrinsicNode(
                Z80ISD::PUSH, DL, DAG.getVTList(MVT::Other),
      { Chain, Val, DAG.getConstant(VA.getLocMemOffset(), DL, PtrVT) },
      LocVT, MachinePointerInfo::getStack(DAG.getMachineFunction(),
                                          VA.getLocMemOffset()),
      //    /*Align=*/0, /*Vol=*/false, /*ReadMem=*/false);
      /*Align=*/0, MachineMemOperand::MOStore);

#else
      Chain = DAG.getStore(
                Chain, DL, Val,
                DAG.getTargetIndex(VA.getLocMemOffset() / PtrBits,
                                   PtrVT, VA.getLocMemOffset() % PtrBits),
                MachinePointerInfo::getStack(DAG.getMachineFunction(),
                                             VA.getLocMemOffset()));
#endif
    }
  }

  // Build a sequence of copy-to-reg nodes chained together with a token chain
  // and flag operands with copy the outgoing args into registers.
  SDValue InFlag;
  for (unsigned I = 0, E = RegsToPass.size(); I != E; ++I) {
    Chain = DAG.getCopyToReg(Chain, DL, RegsToPass[I].first,
                             RegsToPass[I].second, InFlag);
    InFlag = Chain.getValue(1);
  }
  if (IsTailCall) {
    InFlag = SDValue();
  }

  // If the callee is a GlobalAddress node (quite common, every direct call is)
  // turn it into a TargetGlobalAddress node so that legalize doesn't hack it.
  // Likewise ExternalSymbol -> TargetExternalSymbol.
  if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee)) {
    Callee = DAG.getTargetGlobalAddress(G->getGlobal(), DL, PtrVT);
  } else if (ExternalSymbolSDNode *E = dyn_cast<ExternalSymbolSDNode>(Callee)) {
    Callee = DAG.getTargetExternalSymbol(E->getSymbol(), PtrVT);
  }

  // Returns a chain and a flag for retval copy to use.
  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Chain);
  Ops.push_back(Callee);

  // Add argument registers to the end of the list so that they are known live
  // into the call.
  for (unsigned I = 0, E = RegsToPass.size(); I != E; ++I)
    Ops.push_back(DAG.getRegister(RegsToPass[I].first,
                                  RegsToPass[I].second.getValueType()));

  // Add a register mask operand representing the call-preserved registers.
  const uint32_t *Mask = RegInfo->getCallPreservedMask(MF, CallConv);
  assert(Mask && "Missing call preserved mask for calling convention");

  Ops.push_back(DAG.getRegisterMask(Mask));
  if (InFlag.getNode()) {
    Ops.push_back(InFlag);
  }

  if (IsTailCall) {
    MF.getFrameInfo().setHasTailCall();
    return DAG.getNode(Z80ISD::TC_RETURN, DL, NodeTys, Ops);
  }

  // Returns a chain and a flag for retval copy to use.
  Chain = DAG.getNode(Z80ISD::CALL, DL, NodeTys, Ops);
  InFlag = Chain.getValue(1);

  // Create the CALLSEQ_END node.
  Chain = DAG.getCALLSEQ_END(Chain, DAG.getIntPtrConstant(NumBytes, DL, true),
                             DAG.getIntPtrConstant(0, DL, true), InFlag, DL);
  InFlag = Chain.getValue(1);

  // Handle result values, copying them out of physregs into vregs that we
  // return.
  return LowerCallResult(Chain, InFlag, CallConv, IsVarArg, Ins, DL, DAG,
                         InVals);
}

/// MatchingStackOffset - Return true if the given stack call argument is
/// already available in the same position (relatively) of the caller's
/// incoming argument stack.
static bool MatchingStackOffset(SDValue Arg, unsigned Offset,
                                ISD::ArgFlagsTy Flags, MachineFrameInfo &MFI,
                                const MachineRegisterInfo *MRI,
                                const TargetInstrInfo *TII) {
  unsigned Bytes = Arg.getValueType().getStoreSize();
  unsigned NumElements = 0, Elements = 0;
  while (true) {
    switch (Arg.getOpcode()) {
    case ISD::BUILD_PAIR:
      if (NumElements--) {
        Arg = Arg.getOperand(Elements & 1);
        Elements >>= 1;
        continue;
      }
      break;
    case ISD::EXTRACT_ELEMENT:
      assert(NumElements < 8 * sizeof(Elements) && "Overflowed Elements");
      Elements <<= 1;
      Elements |= Arg.getConstantOperandVal(1);
      ++NumElements;
      LLVM_FALLTHROUGH;
    case ISD::SIGN_EXTEND:
    case ISD::ZERO_EXTEND:
    case ISD::ANY_EXTEND:
    case ISD::BITCAST:
      Arg = Arg.getOperand(0);
      continue;
    case ISD::SRL:
    case ISD::SRA:
      if (ConstantSDNode *Amt = dyn_cast<ConstantSDNode>(Arg.getOperand(1))) {
        SDValue Val = Arg.getOperand(0);
        if (Val.getOpcode() == ISD::TRUNCATE) {
          Val = Val.getOperand(0);
        }
        if (Val.getOpcode() == ISD::BUILD_PAIR &&
            Val.getOperand(0).getValueSizeInBits() == Amt->getZExtValue()) {
          Arg = Val.getOperand(1);
          continue;
        }
      }
      break;
    case ISD::TRUNCATE:
      EVT TruncVT = Arg.getValueType();
      Arg = Arg.getOperand(0);
      switch (Arg.getOpcode()) {
      case ISD::BUILD_PAIR:
        if (TruncVT.bitsLE(Arg.getOperand(0).getValueType())) {
          Arg = Arg.getOperand(0);
        }
        break;
      case ISD::AssertZext:
      case ISD::AssertSext:
        Arg = Arg.getOperand(0);
        break;
      }
      continue;
    }
    break;
  }

  int FI = INT_MAX;
  if (Arg.getOpcode() == ISD::CopyFromReg) {
    unsigned VR = cast<RegisterSDNode>(Arg.getOperand(1))->getReg();
    if (!TargetRegisterInfo::isVirtualRegister(VR)) {
      return false;
    }
    MachineInstr *Def = MRI->getVRegDef(VR);
    if (!Def) {
      return false;
    }
    if (Flags.isByVal() || !TII->isLoadFromStackSlot(*Def, FI)) {
      return false;
    }
  } else if (LoadSDNode *Ld = dyn_cast<LoadSDNode>(Arg)) {
    if (Flags.isByVal()) {
      return false;
    }
    SDValue Ptr = Ld->getBasePtr();
    if (FrameIndexSDNode *FINode = dyn_cast<FrameIndexSDNode>(Ptr)) {
      FI = FINode->getIndex();
    } else {
      return false;
    }
  } else if (Arg.getOpcode() == ISD::FrameIndex && Flags.isByVal()) {
    FrameIndexSDNode *FINode = cast<FrameIndexSDNode>(Arg);
    FI = FINode->getIndex();
    Bytes = Flags.getByValSize();
  } else {
    return false;
  }

  assert(FI != INT_MAX);
  return MFI.isFixedObjectIndex(FI) && Offset == MFI.getObjectOffset(FI) &&
         Bytes <= MFI.getObjectSize(FI);
}

/// Check whether the call is eligible for tail call optimization. Targets
/// that want to do tail call optimization should implement this function.
bool Z80TargetLowering::IsEligibleForTailCallOptimization(
  SDValue Callee, CallingConv::ID CalleeCC, bool isVarArg, Type *RetTy,
  const SmallVectorImpl<ISD::OutputArg> &Outs,
  const SmallVectorImpl<SDValue> &OutVals,
  const SmallVectorImpl<ISD::InputArg> &Ins, SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  const Function &CallerF = MF.getFunction();
  CallingConv::ID CallerCC = CallerF.getCallingConv();
  // TODO: Handle other calling conventions when they exist
  if (CalleeCC != CallingConv::C || CallerCC != CallingConv::C) {
    return false;
  }
  LLVMContext &C = *DAG.getContext();
  if (!CCState::resultsCompatible(CalleeCC, CallerCC, MF, C, Ins,
                                  RetCC_Z80_C, RetCC_Z80_C)) {
    return false;
  }
  // If the callee takes no arguments then go on to check the results of the
  // call.
  if (!Outs.empty()) {
    // Check if stack adjustment is needed. For now, do not do this if any
    // argument is passed on the stack.
    SmallVector<CCValAssign, 16> ArgLocs;
    CCState CCInfo(CalleeCC, isVarArg, MF, ArgLocs, C);
    CCInfo.AnalyzeCallOperands(Outs, getCCAssignFn(CalleeCC));
    if (CCInfo.getNextStackOffset()) {
      // Check if the arguments are already laid out in the right way as
      // the caller's fixed stack objects.
      MachineFrameInfo &MFI = MF.getFrameInfo();
      const MachineRegisterInfo *MRI = &MF.getRegInfo();
      const TargetInstrInfo *TII = Subtarget.getInstrInfo();
      for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
        CCValAssign &VA = ArgLocs[i];
        SDValue Arg = OutVals[i];
        ISD::ArgFlagsTy Flags = Outs[i].Flags;
        if (VA.getLocInfo() == CCValAssign::Indirect) {
          return false;
        }
        if (VA.isMemLoc() && !MatchingStackOffset(Arg, VA.getLocMemOffset(),
                                                  Flags, MFI, MRI, TII/*, VA*/)) {
          return false;
        }
      }
    }
  }
  return true;
}

llvm::EVT llvm::Z80TargetLowering::getTypeForExtReturn(
  LLVMContext &Context, EVT VT, ISD::NodeType ExtendKind) const {
  return getRegisterType(Context, MVT::i8);
}

SDValue Z80TargetLowering::LowerReturn(SDValue Chain,
                                       CallingConv::ID CallConv, bool IsVarArg,
                                       const SmallVectorImpl<ISD::OutputArg> &Outs,
                                       const SmallVectorImpl<SDValue> &OutVals,
                                       const SDLoc &DL, SelectionDAG &DAG) const {
  const TargetRegisterInfo *TRI = Subtarget.getRegisterInfo();
  MachineFunction &MF = DAG.getMachineFunction();
  const Function &F = MF.getFunction();

  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, RVLocs, *DAG.getContext());
  CCInfo.AnalyzeReturn(Outs, getRetCCAssignFn(CallConv));

  SDValue Flag;
  SmallVector<SDValue, 6> RetOps;
  RetOps.push_back(Chain);

  // Copy the result values into the output registers.
  assert(Outs.size() == RVLocs.size());
  for (unsigned I = 0, E = RVLocs.size(); I != E; ++I) {
    const ISD::OutputArg &OA = Outs[I];
    CCValAssign &VA = RVLocs[I];
    assert(VA.isRegLoc() && "Can only return in registers!");
    unsigned Reg = VA.getLocReg();
    EVT LocVT = VA.getLocVT();
    SDValue Val = OutVals[I];
    // Don't copy to the full final register if any extending
    if (OA.Flags.isSplitEnd() && !OA.Flags.isZExt() && !OA.Flags.isSExt()) {
      unsigned RegBytes = OA.ArgVT.getStoreSize() - OA.PartOffset;
      if (LocVT.getStoreSize() != RegBytes) {
        unsigned Idx;
        switch (RegBytes) {
        default: llvm_unreachable("Unexpected final size");
        case 1: Idx = Z80::sub_low;   break;
        case 2: Idx = Z80::sub_short; break;
        }
        Reg = TRI->getSubReg(Reg, Idx);
        LocVT = MVT::getIntegerVT(8 * RegBytes);
        Val = DAG.getNode(ISD::TRUNCATE, DL, LocVT, Val);
      }
    }
    // Promote values to the appropriate types.
    else if (VA.getLocInfo() == CCValAssign::AExt) {
      Val = DAG.getNode(ISD::ANY_EXTEND, DL, LocVT, Val);
    } else if (VA.getLocInfo() == CCValAssign::SExt) {
      Val = DAG.getNode(ISD::SIGN_EXTEND, DL, LocVT, Val);
    } else if (VA.getLocInfo() == CCValAssign::ZExt) {
      Val = DAG.getNode(ISD::ZERO_EXTEND, DL, LocVT, Val);
    } else if (VA.getLocInfo() == CCValAssign::BCvt) {
      Val = DAG.getBitcast(LocVT, Val);
    } else {
      assert(VA.getLocInfo() == CCValAssign::Full && "Unknown loc info!");
    }

    Chain = DAG.getCopyToReg(Chain, DL, Reg, Val, Flag);
    Flag = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(Reg, LocVT));
  }

  RetOps[0] = Chain; // Update chain.

  // Add the flag if we have it.
  if (Flag.getNode()) {
    RetOps.push_back(Flag);
  }

  unsigned Opc = Z80ISD::RET_FLAG;
  if (F.hasFnAttribute("interrupt"))
    Opc = F.getFnAttribute("interrupt").getValueAsString() == "NMI" ?
          Z80ISD::RETN_FLAG : Z80ISD::RETI_FLAG;
  return DAG.getNode(Opc, DL, MVT::Other, RetOps);
}

/// Lower the result values of a call into the appropriate copies out of
/// appropriate physical registers.
///
SDValue
Z80TargetLowering::LowerCallResult(SDValue Chain, SDValue InFlag,
                                   CallingConv::ID CallConv, bool IsVarArg,
                                   const SmallVectorImpl<ISD::InputArg> &Ins,
                                   SDLoc DL, SelectionDAG &DAG,
                                   SmallVectorImpl<SDValue> &InVals) const {
  // Assign locations to each value returned by this call.
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeCallResult(Ins, getRetCCAssignFn(CallConv));

  // Copy all of the result registers out of their specified physreg.
  for (unsigned I = 0, E = RVLocs.size(); I != E; ++I) {
    Chain = DAG.getCopyFromReg(Chain, DL, RVLocs[I].getLocReg(),
                               RVLocs[I].getValVT(), InFlag).getValue(1);
    InFlag = Chain.getValue(2);
    InVals.push_back(Chain.getValue(0));
  }

  return Chain;
}

SDValue
Z80TargetLowering::LowerMemArgument(SDValue Chain, CallingConv::ID CallConv,
                                    const SmallVectorImpl<ISD::InputArg> &Ins,
                                    const SDLoc &dl, SelectionDAG &DAG,
                                    const CCValAssign &VA,
                                    MachineFrameInfo &MFI, unsigned i) const {
  // Create the nodes corresponding to a load from this parameter slot.
  ISD::ArgFlagsTy Flags = Ins[i].Flags;
  bool AlwaysUseMutable = shouldGuaranteeTCO(
                            CallConv, DAG.getTarget().Options.GuaranteedTailCallOpt);
  bool isImmutable = !AlwaysUseMutable && !Flags.isByVal();
  EVT ValVT;
  MVT PtrVT = getPointerTy(DAG.getDataLayout());

  // If value is passed by pointer we have address passed instead of the value
  // itself. No need to extend if the mask value and location share the same
  // absolute size.
  bool ExtendedInMem =
    VA.isExtInLoc() && VA.getValVT().getScalarType() == MVT::i1 &&
    VA.getValVT().getSizeInBits() != VA.getLocVT().getSizeInBits();

  if (VA.getLocInfo() == CCValAssign::Indirect || ExtendedInMem) {
    ValVT = VA.getLocVT();
  } else {
    ValVT = VA.getValVT();
  }

  // Calculate SP offset of interrupt parameter, re-arrange the slot normally
  // taken by a return address.
  int Offset = 0;
#if 0
  if (CallConv == CallingConv::X86_INTR) {
    // X86 interrupts may take one or two arguments.
    // On the stack there will be no return address as in regular call.
    // Offset of last argument need to be set to -4/-8 bytes.
    // Where offset of the first argument out of two, should be set to 0 bytes.
    Offset = (Subtarget.is64Bit() ? 8 : 4) * ((i + 1) % Ins.size() - 1);
    if (Subtarget.is64Bit() && Ins.size() == 2) {
      // The stack pointer needs to be realigned for 64 bit handlers with error
      // code, so the argument offset changes by 8 bytes.
      Offset += 8;
    }
  }
#endif // 0

  // FIXME: For now, all byval parameter objects are marked mutable. This can be
  // changed with more analysis.
  // In case of tail call optimization mark all arguments mutable. Since they
  // could be overwritten by lowering of arguments in case of a tail call.
  if (Flags.isByVal()) {
    unsigned Bytes = Flags.getByValSize();
    if (Bytes == 0) { Bytes = 1; } // Don't create zero-sized stack objects.

    // FIXME: For now, all byval parameter objects are marked as aliasing. This
    // can be improved with deeper analysis.
    int FI = MFI.CreateFixedObject(Bytes, VA.getLocMemOffset(), isImmutable,
                                   /*isAliased=*/true);
    // Adjust SP offset of interrupt parameter.
#if 0
    if (CallConv == CallingConv::X86_INTR) {
      MFI.setObjectOffset(FI, Offset);
    }
#endif // 0
    return DAG.getFrameIndex(FI, PtrVT);
  }

  // This is an argument in memory. We might be able to perform copy elision.
  if (Flags.isCopyElisionCandidate()) {
    EVT ArgVT = Ins[i].ArgVT;
    SDValue PartAddr;
    if (Ins[i].PartOffset == 0) {
      // If this is a one-part value or the first part of a multi-part value,
      // create a stack object for the entire argument value type and return a
      // load from our portion of it. This assumes that if the first part of an
      // argument is in memory, the rest will also be in memory.
      int FI = MFI.CreateFixedObject(ArgVT.getStoreSize(), VA.getLocMemOffset(),
                                     /*Immutable=*/false);
      PartAddr = DAG.getFrameIndex(FI, PtrVT);
      return DAG.getLoad(
               ValVT, dl, Chain, PartAddr,
               MachinePointerInfo::getFixedStack(DAG.getMachineFunction(), FI));
    } else {
      // This is not the first piece of an argument in memory. See if there is
      // already a fixed stack object including this offset. If so, assume it
      // was created by the PartOffset == 0 branch above and create a load from
      // the appropriate offset into it.
      int64_t PartBegin = VA.getLocMemOffset();
      int64_t PartEnd = PartBegin + ValVT.getSizeInBits() / 8;
      int FI = MFI.getObjectIndexBegin();
      for (; MFI.isFixedObjectIndex(FI); ++FI) {
        int64_t ObjBegin = MFI.getObjectOffset(FI);
        int64_t ObjEnd = ObjBegin + MFI.getObjectSize(FI);
        if (ObjBegin <= PartBegin && PartEnd <= ObjEnd) {
          break;
        }
      }
      if (MFI.isFixedObjectIndex(FI)) {
        SDValue Addr =
          DAG.getNode(ISD::ADD, dl, PtrVT, DAG.getFrameIndex(FI, PtrVT),
                      DAG.getIntPtrConstant(Ins[i].PartOffset, dl));
        return DAG.getLoad(
                 ValVT, dl, Chain, Addr,
                 MachinePointerInfo::getFixedStack(DAG.getMachineFunction(), FI,
                                                   Ins[i].PartOffset));
      }
    }
  }

  int FI = MFI.CreateFixedObject(ValVT.getSizeInBits() / 8,
                                 VA.getLocMemOffset(), isImmutable);

  // Set SExt or ZExt flag.
  if (VA.getLocInfo() == CCValAssign::ZExt) {
    MFI.setObjectZExt(FI, true);
  } else if (VA.getLocInfo() == CCValAssign::SExt) {
    MFI.setObjectSExt(FI, true);
  }

  // Adjust SP offset of interrupt parameter.
  if (CallConv == CallingConv::X86_INTR) {
    MFI.setObjectOffset(FI, Offset);
  }

  SDValue FIN = DAG.getFrameIndex(FI, PtrVT);
  SDValue Val = DAG.getLoad(
                  ValVT, dl, Chain, FIN,
                  MachinePointerInfo::getFixedStack(DAG.getMachineFunction(), FI));
  return ExtendedInMem
         ? (VA.getValVT().isVector()
            ? DAG.getNode(ISD::SCALAR_TO_VECTOR, dl, VA.getValVT(), Val)
            : DAG.getNode(ISD::TRUNCATE, dl, VA.getValVT(), Val))
         : Val;
}

SDValue Z80TargetLowering::LowerFormalArguments(
  SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
  const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &dl,
  SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  MachineFunction &MF = DAG.getMachineFunction();
  Z80MachineFunctionInfo *FuncInfo = MF.getInfo<Z80MachineFunctionInfo>();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  //MachineRegisterInfo &MRI = MF.getRegInfo();

  const Function &F = MF.getFunction();

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, getCCAssignFn(CallConv));

  // If the function takes variable number of arguments, make a frame index for
  // the start of the first vararg value... for expansion of llvm.va_start. We
  // can skip this if there are no va_start calls.
  if (MFI.hasVAStart())
    FuncInfo->setVarArgsFrameIndex(
      MFI.CreateFixedObject(1, CCInfo.getNextStackOffset(), true));

  SDValue ArgValue;
  for (unsigned I = 0, InsIndex = 0, E = ArgLocs.size(); I != E;
       ++I, ++InsIndex) {
    assert(InsIndex < Ins.size() && "Invalid Ins index");
    CCValAssign &VA = ArgLocs[I];

    if (VA.isRegLoc()) {
      EVT RegVT = VA.getLocVT();
      if (VA.needsCustom()) {
#if 0
        assert(
          VA.getValVT() == MVT::v64i1 &&
          "Currently the only custom case is when we split v64i1 to 2 regs");

        // v64i1 values, in regcall calling convention, that are
        // compiled to 32 bit arch, are split up into two registers.
        ArgValue =
          getv64i1Argument(VA, ArgLocs[++I], Chain, DAG, dl, Subtarget);
#else
        assert(false); // $MS
#endif // 0
      } else {
        const TargetRegisterClass *RC;
        if (RegVT == MVT::i8) {
          RC = &Z80::GR8RegClass;
        } else if (RegVT == MVT::i16)
          RC = &Z80::GR16RegClass;
#if 0
        else if (RegVT == MVT::i32) {
          RC = &X86::GR32RegClass;
        } else if (Is64Bit && RegVT == MVT::i64) {
          RC = &X86::GR64RegClass;
        } else if (RegVT == MVT::f32) {
          RC = Subtarget.hasAVX512() ? &X86::FR32XRegClass : &X86::FR32RegClass;
        } else if (RegVT == MVT::f64) {
          RC = Subtarget.hasAVX512() ? &X86::FR64XRegClass : &X86::FR64RegClass;
        } else if (RegVT == MVT::f80) {
          RC = &X86::RFP80RegClass;
        } else if (RegVT == MVT::f128) {
          RC = &X86::FR128RegClass;
        } else if (RegVT.is512BitVector()) {
          RC = &X86::VR512RegClass;
        } else if (RegVT.is256BitVector()) {
          RC = Subtarget.hasVLX() ? &X86::VR256XRegClass : &X86::VR256RegClass;
        } else if (RegVT.is128BitVector()) {
          RC = Subtarget.hasVLX() ? &X86::VR128XRegClass : &X86::VR128RegClass;
        } else if (RegVT == MVT::x86mmx) {
          RC = &X86::VR64RegClass;
        } else if (RegVT == MVT::v1i1) {
          RC = &X86::VK1RegClass;
        } else if (RegVT == MVT::v8i1) {
          RC = &X86::VK8RegClass;
        } else if (RegVT == MVT::v16i1) {
          RC = &X86::VK16RegClass;
        } else if (RegVT == MVT::v32i1) {
          RC = &X86::VK32RegClass;
        } else if (RegVT == MVT::v64i1) {
          RC = &X86::VK64RegClass;
        }
#endif // 0
        else
          llvm_unreachable("Unknown argument type!");

        unsigned Reg = MF.addLiveIn(VA.getLocReg(), RC);
        ArgValue = DAG.getCopyFromReg(Chain, dl, Reg, RegVT);
      }

#if 0 // $MS: 8 and 16 bit values are passed in the correct size already
      // If this is an 8 or 16-bit value, it is really passed promoted to 32
      // bits.  Insert an assert[sz]ext to capture this, then truncate to the
      // right size.
      if (VA.getLocInfo() == CCValAssign::SExt)
        ArgValue = DAG.getNode(ISD::AssertSext, dl, RegVT, ArgValue,
                               DAG.getValueType(VA.getValVT()));
      else if (VA.getLocInfo() == CCValAssign::ZExt)
        ArgValue = DAG.getNode(ISD::AssertZext, dl, RegVT, ArgValue,
                               DAG.getValueType(VA.getValVT()));
      else if (VA.getLocInfo() == CCValAssign::BCvt) {
        ArgValue = DAG.getBitcast(VA.getValVT(), ArgValue);
      }
#endif // 0

      if (VA.isExtInLoc()) {
#if 0
        // Handle MMX values passed in XMM regs.
        if (RegVT.isVector() && VA.getValVT().getScalarType() != MVT::i1) {
          ArgValue = DAG.getNode(X86ISD::MOVDQ2Q, dl, VA.getValVT(), ArgValue);
        } else if (VA.getValVT().isVector() &&
                   VA.getValVT().getScalarType() == MVT::i1 &&
                   ((VA.getLocVT() == MVT::i64) || (VA.getLocVT() == MVT::i32) ||
                    (VA.getLocVT() == MVT::i16) || (VA.getLocVT() == MVT::i8))) {
          // Promoting a mask type (v*i1) into a register of type i64/i32/i16/i8
          ArgValue = lowerRegToMasks(ArgValue, VA.getValVT(), RegVT, dl, DAG);
        } else
#endif // 0
          ArgValue = DAG.getNode(ISD::TRUNCATE, dl, VA.getValVT(), ArgValue);
      }
    } else {
      assert(VA.isMemLoc());
      ArgValue =
        LowerMemArgument(Chain, CallConv, Ins, dl, DAG, VA, MFI, InsIndex);
    }

    // If value is passed via pointer - do a load.
    if (VA.getLocInfo() == CCValAssign::Indirect)
      ArgValue =
        DAG.getLoad(VA.getValVT(), dl, Chain, ArgValue, MachinePointerInfo());

    InVals.push_back(ArgValue);
  }

  for (unsigned I = 0, E = Ins.size(); I != E; ++I) {
    // Swift calling convention does not require we copy the sret argument
    // into %rax/%eax for the return. We don't set SRetReturnReg for Swift.
    if (CallConv == CallingConv::Swift) {
      continue;
    }

    // All x86 ABIs require that for returning structs by value we copy the
    // sret argument into %rax/%eax (depending on ABI) for the return. Save
    // the argument into a virtual register so that we can access it from the
    // return points.
    if (Ins[I].Flags.isSRet()) {
      unsigned Reg = FuncInfo->getSRetReturnReg();
      if (!Reg) {
        MVT PtrTy = getPointerTy(DAG.getDataLayout());
        Reg = MF.getRegInfo().createVirtualRegister(getRegClassFor(PtrTy));
        FuncInfo->setSRetReturnReg(Reg);
      }
      SDValue Copy = DAG.getCopyToReg(DAG.getEntryNode(), dl, Reg, InVals[I]);
      Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, Copy, Chain);
      break;
    }
  }

  unsigned StackSize = CCInfo.getNextStackOffset();
#if 0
  // Align stack specially for tail calls.
  if (shouldGuaranteeTCO(CallConv,
                         MF.getTarget().Options.GuaranteedTailCallOpt)) {
    StackSize = GetAlignedArgumentStackSize(StackSize, DAG);
  }
#endif // 0

  // If the function takes variable number of arguments, make a frame index for
  // the start of the first vararg value... for expansion of llvm.va_start. We
  // can skip this if there are no va_start calls.
  if (MFI.hasVAStart() &&
      (/*Is64Bit ||*/ (CallConv != CallingConv::X86_FastCall &&
                       CallConv != CallingConv::X86_ThisCall))) {
    FuncInfo->setVarArgsFrameIndex(MFI.CreateFixedObject(1, StackSize, true));
  }

#if 0
  // Figure out if XMM registers are in use.
  assert(!(Subtarget.useSoftFloat() &&
           F.hasFnAttribute(Attribute::NoImplicitFloat)) &&
         "SSE register cannot be used when SSE is disabled!");

  // 64-bit calling conventions support varargs and register parameters, so we
  // have to do extra work to spill them in the prologue.
  if (Is64Bit && isVarArg && MFI.hasVAStart()) {
    // Find the first unallocated argument registers.
    ArrayRef<MCPhysReg> ArgGPRs = get64BitArgumentGPRs(CallConv, Subtarget);
    ArrayRef<MCPhysReg> ArgXMMs = get64BitArgumentXMMs(MF, CallConv, Subtarget);
    unsigned NumIntRegs = CCInfo.getFirstUnallocated(ArgGPRs);
    unsigned NumXMMRegs = CCInfo.getFirstUnallocated(ArgXMMs);
    assert(!(NumXMMRegs && !Subtarget.hasSSE1()) &&
           "SSE register cannot be used when SSE is disabled!");

    // Gather all the live in physical registers.
    SmallVector<SDValue, 6> LiveGPRs;
    SmallVector<SDValue, 8> LiveXMMRegs;
    SDValue ALVal;
    for (MCPhysReg Reg : ArgGPRs.slice(NumIntRegs)) {
      unsigned GPR = MF.addLiveIn(Reg, &X86::GR64RegClass);
      LiveGPRs.push_back(
        DAG.getCopyFromReg(Chain, dl, GPR, MVT::i64));
    }
    if (!ArgXMMs.empty()) {
      unsigned AL = MF.addLiveIn(X86::AL, &X86::GR8RegClass);
      ALVal = DAG.getCopyFromReg(Chain, dl, AL, MVT::i8);
      for (MCPhysReg Reg : ArgXMMs.slice(NumXMMRegs)) {
        unsigned XMMReg = MF.addLiveIn(Reg, &X86::VR128RegClass);
        LiveXMMRegs.push_back(
          DAG.getCopyFromReg(Chain, dl, XMMReg, MVT::v4f32));
      }
    }

    if (IsWin64) {
      // Get to the caller-allocated home save location.  Add 8 to account
      // for the return address.
      int HomeOffset = TFI.getOffsetOfLocalArea() + 8;
      FuncInfo->setRegSaveFrameIndex(
        MFI.CreateFixedObject(1, NumIntRegs * 8 + HomeOffset, false));
      // Fixup to set vararg frame on shadow area (4 x i64).
      if (NumIntRegs < 4) {
        FuncInfo->setVarArgsFrameIndex(FuncInfo->getRegSaveFrameIndex());
      }
    } else {
      // For X86-64, if there are vararg parameters that are passed via
      // registers, then we must store them to their spots on the stack so
      // they may be loaded by dereferencing the result of va_next.
      FuncInfo->setVarArgsGPOffset(NumIntRegs * 8);
      FuncInfo->setVarArgsFPOffset(ArgGPRs.size() * 8 + NumXMMRegs * 16);
      FuncInfo->setRegSaveFrameIndex(MFI.CreateStackObject(
                                       ArgGPRs.size() * 8 + ArgXMMs.size() * 16, 16, false));
    }

    // Store the integer parameter registers.
    SmallVector<SDValue, 8> MemOps;
    SDValue RSFIN = DAG.getFrameIndex(FuncInfo->getRegSaveFrameIndex(),
                                      getPointerTy(DAG.getDataLayout()));
    unsigned Offset = FuncInfo->getVarArgsGPOffset();
    for (SDValue Val : LiveGPRs) {
      SDValue FIN = DAG.getNode(ISD::ADD, dl, getPointerTy(DAG.getDataLayout()),
                                RSFIN, DAG.getIntPtrConstant(Offset, dl));
      SDValue Store =
        DAG.getStore(Val.getValue(1), dl, Val, FIN,
                     MachinePointerInfo::getFixedStack(
                       DAG.getMachineFunction(),
                       FuncInfo->getRegSaveFrameIndex(), Offset));
      MemOps.push_back(Store);
      Offset += 8;
    }

    if (!ArgXMMs.empty() && NumXMMRegs != ArgXMMs.size()) {
      // Now store the XMM (fp + vector) parameter registers.
      SmallVector<SDValue, 12> SaveXMMOps;
      SaveXMMOps.push_back(Chain);
      SaveXMMOps.push_back(ALVal);
      SaveXMMOps.push_back(DAG.getIntPtrConstant(
                             FuncInfo->getRegSaveFrameIndex(), dl));
      SaveXMMOps.push_back(DAG.getIntPtrConstant(
                             FuncInfo->getVarArgsFPOffset(), dl));
      SaveXMMOps.insert(SaveXMMOps.end(), LiveXMMRegs.begin(),
                        LiveXMMRegs.end());
      MemOps.push_back(DAG.getNode(X86ISD::VASTART_SAVE_XMM_REGS, dl,
                                   MVT::Other, SaveXMMOps));
    }

    if (!MemOps.empty()) {
      Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, MemOps);
    }
  }

  if (isVarArg && MFI.hasMustTailInVarArgFunc()) {
    // Find the largest legal vector type.
    MVT VecVT = MVT::Other;
    // FIXME: Only some x86_32 calling conventions support AVX512.
    if (Subtarget.hasAVX512() &&
        (Is64Bit || (CallConv == CallingConv::X86_VectorCall ||
                     CallConv == CallingConv::Intel_OCL_BI))) {
      VecVT = MVT::v16f32;
    } else if (Subtarget.hasAVX()) {
      VecVT = MVT::v8f32;
    } else if (Subtarget.hasSSE2()) {
      VecVT = MVT::v4f32;
    }

    // We forward some GPRs and some vector types.
    SmallVector<MVT, 2> RegParmTypes;
    MVT IntVT = Is64Bit ? MVT::i64 : MVT::i32;
    RegParmTypes.push_back(IntVT);
    if (VecVT != MVT::Other) {
      RegParmTypes.push_back(VecVT);
    }

    // Compute the set of forwarded registers. The rest are scratch.
    SmallVectorImpl<ForwardedRegister> &Forwards =
      FuncInfo->getForwardedMustTailRegParms();
    CCInfo.analyzeMustTailForwardedRegisters(Forwards, RegParmTypes, CC_X86);

    // Conservatively forward AL on x86_64, since it might be used for varargs.
    if (Is64Bit && !CCInfo.isAllocated(X86::AL)) {
      unsigned ALVReg = MF.addLiveIn(X86::AL, &X86::GR8RegClass);
      Forwards.push_back(ForwardedRegister(ALVReg, X86::AL, MVT::i8));
    }

    // Copy all forwards from physical to virtual registers.
    for (ForwardedRegister &F : Forwards) {
      // FIXME: Can we use a less constrained schedule?
      SDValue RegVal = DAG.getCopyFromReg(Chain, dl, F.VReg, F.VT);
      F.VReg = MF.getRegInfo().createVirtualRegister(getRegClassFor(F.VT));
      Chain = DAG.getCopyToReg(Chain, dl, F.VReg, RegVal);
    }
  }
#endif // 0

  // Some CCs need callee pop.
  if (Z80::isCalleePop(CallConv, /*Is64Bit,*/ isVarArg,
                       MF.getTarget().Options.GuaranteedTailCallOpt)) {
    FuncInfo->setBytesToPopOnReturn(StackSize); // Callee pops everything.
  } else if (CallConv == CallingConv::X86_INTR && Ins.size() == 2) {
    // X86 interrupts must pop the error code (and the alignment padding) if
    // present.
    FuncInfo->setBytesToPopOnReturn(/*Is64Bit ? 16 : 4*/ 2);
  } else {
    FuncInfo->setBytesToPopOnReturn(0); // Callee pops nothing.
    // If this is an sret function, the return should pop the hidden pointer.
    if (/*!Is64Bit &&*/ !canGuaranteeTCO(CallConv) &&
                        /*!Subtarget.getTargetTriple().isOSMSVCRT() &&*/
                        argsAreStructReturn(Ins /*, Subtarget.isTargetMCU()*/) == StackStructReturn) {
      FuncInfo->setBytesToPopOnReturn(/*4*/ 2);
    }
  }

  //if (!Is64Bit)
  {
    // RegSaveFrameIndex is X86-64 only.
    FuncInfo->setRegSaveFrameIndex(0xAAAAAAA);
    if (CallConv == CallingConv::X86_FastCall ||
        CallConv == CallingConv::X86_ThisCall)
      // fastcc functions can't have varargs.
    {
      FuncInfo->setVarArgsFrameIndex(0xAAAAAAA);
    }
  }

  FuncInfo->setArgumentStackSize(StackSize);

#if 0
  if (WinEHFuncInfo *EHInfo = MF.getWinEHFuncInfo()) {
    EHPersonality Personality = classifyEHPersonality(F.getPersonalityFn());
    if (Personality == EHPersonality::CoreCLR) {
      assert(Is64Bit);
      // TODO: Add a mechanism to frame lowering that will allow us to indicate
      // that we'd prefer this slot be allocated towards the bottom of the frame
      // (i.e. near the stack pointer after allocating the frame).  Every
      // funclet needs a copy of this slot in its (mostly empty) frame, and the
      // offset from the bottom of this and each funclet's frame must be the
      // same, so the size of funclets' (mostly empty) frames is dictated by
      // how far this slot is from the bottom (since they allocate just enough
      // space to accommodate holding this slot at the correct offset).
      int PSPSymFI = MFI.CreateStackObject(8, 8, /*isSS=*/false);
      EHInfo->PSPSymFrameIdx = PSPSymFI;
    }
  }
#endif // 0

  if (CallConv == CallingConv::X86_RegCall ||
      F.hasFnAttribute("no_caller_saved_registers")) {
    MachineRegisterInfo &MRI = MF.getRegInfo();
    for (std::pair<unsigned, unsigned> Pair : MRI.liveins()) {
      MRI.disableCalleeSavedRegister(Pair.first);
    }
  }

  return Chain;
}

const char *Z80TargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch ((Z80ISD::NodeType)Opcode) {
  case Z80ISD::FIRST_NUMBER: break;
  case Z80ISD::Wrapper:      return "Z80ISD::Wrapper";
  case Z80ISD::RLC:          return "Z80ISD::RLC";
  case Z80ISD::RRC:          return "Z80ISD::RRC";
  case Z80ISD::RL:           return "Z80ISD::RL";
  case Z80ISD::RR:           return "Z80ISD::RR";
  case Z80ISD::SLA:          return "Z80ISD::SLA";
  case Z80ISD::SRA:          return "Z80ISD::SRA";
  case Z80ISD::SRL:          return "Z80ISD::SRL";
  case Z80ISD::INC:          return "Z80ISD::INC";
  case Z80ISD::DEC:          return "Z80ISD::DEC";
  case Z80ISD::ADD:          return "Z80ISD::ADD";
  case Z80ISD::ADC:          return "Z80ISD::ADC";
  case Z80ISD::SUB:          return "Z80ISD::SUB";
  case Z80ISD::SBC:          return "Z80ISD::SBC";
  case Z80ISD::AND:          return "Z80ISD::AND";
  case Z80ISD::XOR:          return "Z80ISD::XOR";
  case Z80ISD::OR:           return "Z80ISD::OR";
  case Z80ISD::CP:           return "Z80ISD::CP";
  case Z80ISD::TST:          return "Z80ISD::TST";
  case Z80ISD::MLT:          return "Z80ISD::MLT";
  case Z80ISD::SEXT:         return "Z80ISD::SEXT";
  case Z80ISD::CALL:         return "Z80ISD::CALL";
  case Z80ISD::RET_FLAG:     return "Z80ISD::RET_FLAG";
  case Z80ISD::RETN_FLAG:    return "Z80ISD::RETN_FLAG";
  case Z80ISD::RETI_FLAG:    return "Z80ISD::RETI_FLAG";
  case Z80ISD::TC_RETURN:    return "Z80ISD::TC_RETURN";
  case Z80ISD::BRCOND:       return "Z80ISD::BRCOND";
  case Z80ISD::SELECT:       return "Z80ISD::SELECT";
  case Z80ISD::POP:          return "Z80ISD::POP";
  case Z80ISD::PUSH:         return "Z80ISD::PUSH";
  }
  return nullptr;
}

EVT Z80TargetLowering::getSetCCResultType(const DataLayout &DL,
                                          LLVMContext &Context,
                                          EVT VT) const {
  assert(!VT.isVector() && "No default SetCC type for vectors!");
  return MVT::i8;
}
MVT::SimpleValueType Z80TargetLowering::getCmpLibcallReturnType() const {
  return MVT::Other;
}
