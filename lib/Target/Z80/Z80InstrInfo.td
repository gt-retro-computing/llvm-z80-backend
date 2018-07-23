//===-- Z80InstrInfo.td - Main Z80 Instruction Definition --*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the Z80 instruction set, defining the instructions, and
// properties of the instructions which are needed for code generation, machine
// code emission, and analysis.
//
//===----------------------------------------------------------------------===//

include "Z80RegisterInfo.td"

//===----------------------------------------------------------------------===//
// Type Constraints.
//===----------------------------------------------------------------------===//
class SDTCisChain<int OpNum> : SDTCisVT<OpNum, OtherVT>;
class SDTCisI8   <int OpNum> : SDTCisVT<OpNum, i8>;
class SDTCisFlag <int OpNum> : SDTCisI8<OpNum>;
class SDTCisI16  <int OpNum> : SDTCisVT<OpNum, i16>;
class SDTCisPtr  <int OpNum> : SDTCisVT<OpNum, iPTR>;

//===----------------------------------------------------------------------===//
// Type Profiles. <Results, Operants, Constraints>
//===----------------------------------------------------------------------===//
def SDTUnOpRF   : SDTypeProfile<2, 1, [SDTCisInt<0>,
                                       SDTCisFlag<1>,
                                       SDTCisSameAs<2, 0>]>;
def SDTUnOpRFF  : SDTypeProfile<2, 2, [SDTCisInt<0>,
                                       SDTCisFlag<1>,
                                       SDTCisSameAs<2, 0>,
                                       SDTCisFlag<3>]>;
def SDTBinOpRF  : SDTypeProfile<2, 2, [SDTCisInt<0>,
                                       SDTCisFlag<1>,
                                       SDTCisSameAs<2, 0>,
                                       SDTCisSameAs<3, 0>]>;
def SDTBinOpRFF : SDTypeProfile<2, 3, [SDTCisInt<0>,
                                       SDTCisFlag<1>,
                                       SDTCisSameAs<2, 0>,
                                       SDTCisSameAs<3, 0>,
                                       SDTCisFlag<4>]>;
def SDTBinOpF   : SDTypeProfile<1, 2, [SDTCisFlag<0>,
                                       SDTCisInt<1>,
                                       SDTCisSameAs<2, 1>]>;

def SDTZ80Wrapper       : SDTypeProfile<1, 1, [SDTCisPtrTy<0>,
                                               SDTCisSameAs<1, 0>]>;
def SDT_Z80mlt          : SDTypeProfile<1, 1, [SDTCisI16<0>, SDTCisI16<1>]>;
def SDT_Z80sext         : SDTypeProfile<1, 1, [SDTCisInt<0>, SDTCisFlag<1>]>;
def SDT_Z80TCRet        : SDTypeProfile<0, 1, [SDTCisPtrTy<0>]>;
def SDT_Z80Call         : SDTypeProfile<0, -1, [SDTCisPtr<0>]>;
def SDT_Z80CallSeqStart : SDCallSeqStart<[SDTCisPtr<0>, SDTCisPtr<1>]>;
def SDT_Z80CallSeqEnd   : SDCallSeqEnd<[SDTCisPtr<0>, SDTCisPtr<1>]>;
def SDT_Z80BrCond       : SDTypeProfile<0, 3, [SDTCisChain<0>,
                                               SDTCisI8<1>,
                                               SDTCisFlag<2>]>;
def SDT_Z80Select       : SDTypeProfile<1, 4, [SDTCisInt<0>,
                                               SDTCisSameAs<1, 0>,
                                               SDTCisSameAs<2, 0>,
                                               SDTCisI8<3>,
                                               SDTCisI8<4>]>;
def SDT_Z80Pop          : SDTypeProfile<1, 0, [SDTCisPtr<0>]>;
def SDT_Z80Push         : SDTypeProfile<0, 1, [SDTCisPtr<0>]>;
def SDT_Z80Push8        : SDTypeProfile<0, 1, [SDTCisI8<0>]>;

//===----------------------------------------------------------------------===//
// Z80 specific DAG Nodes.
//

def Z80Wrapper       : SDNode<"Z80ISD::Wrapper", SDTZ80Wrapper>;
def Z80rlc_flag      : SDNode<"Z80ISD::RLC",     SDTUnOpRF>;
def Z80rrc_flag      : SDNode<"Z80ISD::RRC",     SDTUnOpRF>;
def Z80rl_flag       : SDNode<"Z80ISD::RL",      SDTUnOpRFF>;
def Z80rr_flag       : SDNode<"Z80ISD::RR",      SDTUnOpRFF>;
def Z80sla_flag      : SDNode<"Z80ISD::SLA",     SDTUnOpRF>;
def Z80sra_flag      : SDNode<"Z80ISD::SRA",     SDTUnOpRF>;
def Z80srl_flag      : SDNode<"Z80ISD::SRL",     SDTUnOpRF>;
def Z80inc_flag      : SDNode<"Z80ISD::INC",     SDTUnOpRF>;
def Z80dec_flag      : SDNode<"Z80ISD::DEC",     SDTUnOpRF>;
def Z80add_flag      : SDNode<"Z80ISD::ADD",     SDTBinOpRF, [SDNPCommutative]>;
def Z80adc_flag      : SDNode<"Z80ISD::ADC",     SDTBinOpRFF>;
def Z80sub_flag      : SDNode<"Z80ISD::SUB",     SDTBinOpRF>;
def Z80sbc_flag      : SDNode<"Z80ISD::SBC",     SDTBinOpRFF>;
def Z80and_flag      : SDNode<"Z80ISD::AND",     SDTBinOpRF, [SDNPCommutative]>;
def Z80xor_flag      : SDNode<"Z80ISD::XOR",     SDTBinOpRF, [SDNPCommutative]>;
def Z80or_flag       : SDNode<"Z80ISD::OR",      SDTBinOpRF, [SDNPCommutative]>;
def Z80cp_flag       : SDNode<"Z80ISD::CP",      SDTBinOpF>;
def Z80tst_flag      : SDNode<"Z80ISD::TST",     SDTBinOpF,  [SDNPCommutative]>;
def Z80mlt           : SDNode<"Z80ISD::MLT",     SDT_Z80mlt>;
def Z80sext          : SDNode<"Z80ISD::SEXT",    SDT_Z80sext>;
def Z80retflag       : SDNode<"Z80ISD::RET_FLAG", SDTNone,
                              [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def Z80retnflag      : SDNode<"Z80ISD::RETN_FLAG", SDTNone,
                              [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def Z80retiflag      : SDNode<"Z80ISD::RETI_FLAG", SDTNone,
                              [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def Z80tcret         : SDNode<"Z80ISD::TC_RETURN", SDT_Z80TCRet,
                              [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def Z80call          : SDNode<"Z80ISD::CALL", SDT_Z80Call,
                              [SDNPHasChain, SDNPOutGlue, SDNPOptInGlue,
                               SDNPVariadic]>;
def Z80callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_Z80CallSeqStart,
                              [SDNPHasChain, SDNPOutGlue]>;
def Z80callseq_end   : SDNode<"ISD::CALLSEQ_END", SDT_Z80CallSeqEnd,
                              [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;
def Z80brcond        : SDNode<"Z80ISD::BRCOND", SDT_Z80BrCond, [SDNPHasChain]>;
def Z80select        : SDNode<"Z80ISD::SELECT", SDT_Z80Select>;
def Z80pop           : SDNode<"Z80ISD::POP", SDT_Z80Pop,
                              [SDNPHasChain, SDNPMayLoad]>;
def Z80push          : SDNode<"Z80ISD::PUSH", SDT_Z80Push,
                              [SDNPHasChain, SDNPMayStore]>;
def Z80push8         : SDNode<"Z80ISD::PUSH", SDT_Z80Push8,
                              [SDNPHasChain, SDNPMayStore]>;

//===----------------------------------------------------------------------===//
// Z80 Instruction Predicate Definitions.
//
def HaveUndocOps : Predicate<"Subtarget->hasUndocOps()">,
                   AssemblerPredicate<"FeatureUndoc", "undocumented ops">;
def HaveIdxHalf  : Predicate<"Subtarget->hasIndexHalfRegs()">,
                   AssemblerPredicate<"FeatureIdxHalf", "index half regs">;

//===----------------------------------------------------------------------===//
// Z80 Instruction Format Definitions.
//

include "Z80InstrFormats.td"

//===----------------------------------------------------------------------===//
// Pattern fragments.
//

// Z80 specific condition code. These correspond to CondCode in
// Z80InstrInfo.h. They must be kept in synch.
def Z80_COND_NZ : PatLeaf<(i8 0)>;
def Z80_COND_Z  : PatLeaf<(i8 1)>;
def Z80_COND_NC : PatLeaf<(i8 2)>;
def Z80_COND_C  : PatLeaf<(i8 3)>;
def Z80_COND_PO : PatLeaf<(i8 4)>;
def Z80_COND_PE : PatLeaf<(i8 5)>;
def Z80_COND_P  : PatLeaf<(i8 6)>;
def Z80_COND_M  : PatLeaf<(i8 7)>;

//===----------------------------------------------------------------------===//
// Z80 Operand Definitions.
//

def aptr_rc : PointerLikeRegClass<1>;
def iptr_rc : PointerLikeRegClass<2>;

def mem : Operand<iPTR> {
  let PrintMethod = "printMem";
  let MIOperandInfo = (ops imm);
  let OperandType = "OPERAND_MEMORY";
}
def ptr : Operand<iPTR> {
  let PrintMethod = "printPtr";
  let MIOperandInfo = (ops aptr_rc);
  let OperandType = "OPERAND_MEMORY";
}
def off : Operand<iPTR> {
  let PrintMethod = "printOff";
  let MIOperandInfo = (ops iptr_rc, i8imm);
  let OperandType = "OPERAND_MEMORY";
}
def off16 : Operand<i16> {
  let PrintMethod = "printAddr";
  let MIOperandInfo = (ops IR16, i8imm);
}

def jmptarget : Operand<OtherVT>;
def jmptargetoff : Operand<OtherVT>;

def cc : Operand<i8> {
  let PrintMethod = "printCCOperand";
}

//===----------------------------------------------------------------------===//
// Pattern Fragments.
//

//===----------------------------------------------------------------------===//
// Z80 Complex Pattern Definitions.
//
def mempat : ComplexPattern<iPTR, 1, "SelectMem",
                            [imm, globaladdr, externalsym]>;
def offpat : ComplexPattern<iPTR, 2, "SelectOff",
                            [add, frameindex]>;
def fipat  : ComplexPattern<iPTR, 2, "SelectFI",
                            [add, frameindex]>;

//===----------------------------------------------------------------------===//
// Instruction list.
//

let hasPostISelHook = 1 in {
  let Defs = [SPS, F], Uses = [SPS] in {
  def ADJCALLSTACKDOWN16 : P<(outs), (ins i16imm:$amt1, i16imm:$amt2),
                             [(Z80callseq_start timm:$amt1, timm:$amt2)]>;
  def ADJCALLSTACKUP16   : P<(outs), (ins i16imm:$amt1, i16imm:$amt2),
                             [(Z80callseq_end timm:$amt1, timm:$amt2)]>;
  }
}


let usesCustomInserter = 1 in {
  let Uses = [F] in {
    def Select8  : P<(outs  RR8:$dst), (ins  RR8:$true,  RR8:$false, i8imm:$cc),
                     [(set  RR8:$dst, (Z80select  RR8:$true,  RR8:$false, imm:$cc,
                                      F))]>;
    def Select16 : P<(outs R16:$dst), (ins R16:$true, R16:$false, i8imm:$cc),
                     [(set R16:$dst, (Z80select R16:$true, R16:$false, imm:$cc,
                                      F))]>;
  }
  let Uses = [F] in {
    let Defs = [A]   in def SExt8  : P<(outs), (ins), [(set A,   (Z80sext F))]>;
    let Defs = [HL]  in def SExt16 : P<(outs), (ins), [(set HL,  (Z80sext F))]>;
  }
}

let hasSideEffects = 0 in
def NOP : I<NoPre, 0x00, "nop">;
def DI  : I<NoPre, 0xF3, "di">;
def EI  : I<NoPre, 0xFB, "ei">;

//===----------------------------------------------------------------------===//
//  Control Flow Instructions.
//

// All calls clobber the non-callee saved registers.  SP is marked as a use to
// prevent stack-pointer assignments that appear immediately before calls from
// potentially appearing dead.  Uses for argument registers are added manually.
let isCall = 1 in {
  let Uses = [SPS] in {
    def CALL16i : I16i<NoPre, 0xCD, "call", "\t$tgt", "",
                       (outs), (ins i16imm:$tgt), [(Z80call mempat:$tgt)]>;
    def CALL16r : P   <(outs), (ins    AIR16:$tgt), [(Z80call    AIR16:$tgt)]>;
  }
}

let isTerminator = 1, isReturn = 1, isBarrier = 1 in {
  def RET  : I<NoPre, 0xC9, "ret",  "", "", (outs), (ins), [(Z80retflag)]>;
  def RETN : I<EDPre, 0x45, "retn", "", "", (outs), (ins), [(Z80retnflag)]>;
  def RETI : I<EDPre, 0x4D, "reti", "", "">;
  def EI_RETI : P<(outs), (ins), [(Z80retiflag)]>;
}
let isCall = 1, isTerminator = 1, isReturn = 1, isBarrier = 1 in {
  let Uses = [SPS] in {
    def TCRETURN16i : P<(outs), (ins i16imm:$tgt), [(Z80tcret mempat:$tgt)]>;
    def TCRETURN16r : P<(outs), (ins    AIR16:$tgt), [(Z80tcret    AIR16:$tgt)]>;
  }
}

let isBranch = 1, isTerminator = 1 in {
  let isBarrier = 1 in {
    def JQ : Pseudo<"jp", "\t$tgt", "", (outs), (ins jmptarget:$tgt),
                    [(br bb:$tgt)]>;
    def JR   : I8i <NoPre, 0x18, "jr", "\t$tgt", "",
                    (outs), (ins jmptargetoff:$tgt)>;
    def JP16 : I16i<NoPre, 0xC3, "jp", "\t$tgt", "",
                    (outs), (ins jmptarget:$tgt)>;
    let isIndirectBranch = 1 in {
      def JP16r : I16<NoPre, 0xE9, "jp", "\t($tgt)", "",
                      (outs), (ins AIR16:$tgt), [(brind AIR16:$tgt)]>;
    }
  }
  let Uses = [F] in {
    def JQCC : Pseudo<"jp", "\t$cc, $tgt", "",
                      (outs), (ins jmptarget:$tgt, cc:$cc),
                      [(Z80brcond bb:$tgt, imm:$cc, F)]>;
    def JRCC   : I8i <NoPre, 0x18, "jr", "\t$cc, $tgt", "",
                      (outs), (ins jmptargetoff:$tgt, cc:$cc)>;
    def JP16CC : I16i<NoPre, 0xC3, "jp", "\t$cc, $tgt", "",
                      (outs), (ins jmptarget:$tgt, cc:$cc)>;
  }
}

//===----------------------------------------------------------------------===//
//  Load Instructions.
//

def LD8gg : I8<NoPre, 0x40, "ld", "\t$dst, $src", "",
               (outs GR8:$dst), (ins GR8:$src)>;
//def LD8xx : I8<DDPre, 0x40, "ld", "\t$dst, $src", "",
//               (outs X8:$dst), (ins X8:$src)>, Requires<[HaveIdxHalf]>;
//def LD8yy : I8<FDPre, 0x40, "ld", "\t$dst, $src", "",
//               (outs Y8:$dst), (ins Y8:$src)>, Requires<[HaveIdxHalf]>;

let isMoveImm = 1, isReMaterializable = 1 in {
  let Defs = [F] in {
    def LD8r0   : Pseudo<"ld", "\t$dst, 0",  "",
                         (outs  RR8:$dst), (ins), [(set  RR8:$dst,  0)]>;
  }
  def LD8ri  : I8i <Idx0Pre, 0x06, "ld", "\t$dst, $src", "",
                    (outs  RR8:$dst), (ins  i8imm:$src),
                    [(set  RR8:$dst, imm:$src)]>;
  def LD16ri : I16i<Idx0Pre, 0x01, "ld", "\t$dst, $src", "",
                    (outs R16:$dst), (ins i16imm:$src),
                    [(set R16:$dst, imm:$src)]>;
}

let mayLoad = 1, canFoldAsLoad = 1, isReMaterializable = 1 in {
  let Defs = [A] in
  def LD8am  : Ii    <NoPre,   0x3A, "ld", "\ta, $src",    "",
                      (outs), (ins mem:$src), [(set A, (load mempat:$src))]>;
  def LD16rm : Pseudo<               "ld", "\t$dst, $src", "",
                      (outs R16:$dst), (ins mem:$src),
                      [(set R16:$dst, (load mempat:$src))]>;
  def LD16am : I16i  <Idx0Pre, 0x2A, "ld", "\t$dst, $src", "",
                    (outs AIR16:$dst), (ins mem:$src)>;
  def LD16om : I16i  <EDPre,   0x4B, "ld", "\t$dst, $src", "",
                    (outs OR16:$dst), (ins mem:$src)>;

  def LD8rp  : Pseudo<               "ld", "\t$dst, $src", "",
                     (outs RR8:$dst), (ins ptr:$src)>;
  def LD8gp  : I8o   <Idx1Pre, 0x46, "ld", "\t$dst, $src", "",
                     (outs GR8:$dst), (ins ptr:$src),
                     [(set GR8:$dst, (load iPTR:$src))]>;
  def LD88rp : Pseudo<               "ld", "\t$dst, $src", "",
                      (outs R16:$dst), (ins ptr:$src),
                      [(set R16:$dst, (load iPTR:$src))]>;

  def LD8ro  : Pseudo<               "ld", "\t$dst, $src", "",
                     (outs RR8:$dst), (ins off:$src)>;
  def LD8go  : I8o    <Idx1Pre, 0x46, "ld", "\t$dst, $src", "",
                     (outs GR8:$dst), (ins off:$src),
                     [(set GR8:$dst, (load offpat:$src))]>;
  def LD88ro : Pseudo<               "ld", "\t$dst, $src", "",
                      (outs R16:$dst), (ins off:$src),
                      [(set R16:$dst, (load offpat:$src))]>;
}
def : Pat<(i16 (extloadi8  mempat:$src)), (LD16rm mem:$src)>;
def : Pat<(i16 (extloadi8    iPTR:$src)),
          (INSERT_SUBREG (IMPLICIT_DEF), (LD8rp ptr:$src), sub_low)>;
def : Pat<(i16 (extloadi8  offpat:$src)),
          (INSERT_SUBREG (IMPLICIT_DEF), (LD8ro off:$src), sub_low)>;

let mayStore = 1 in {
  let Uses = [A] in
  def LD8ma  : Ii    <  NoPre, 0x32, "ld", "\t$dst, a",    "",
                      (outs), (ins mem:$dst), [(store A, mempat:$dst)]>;
  def LD16mr : Pseudo<               "ld", "\t$dst, $src", "",
                      (outs), (ins mem:$dst, R16:$src),
                      [(store R16:$src, mempat:$dst)]>;
  def LD16ma : I16i  <Idx1Pre, 0x22, "ld", "\t$dst, $src", "",
                      (outs), (ins mem:$dst, AIR16:$src)>;
  def LD16mo : I16i  <  EDPre, 0x43, "ld", "\t$dst, $src", "",
                      (outs), (ins mem:$dst, OR16:$src)>;

  def LD8pr  : Pseudo<               "ld", "\t$dst, $src", "",
                      (outs), (ins ptr:$dst, RR8:$src)>;
  def LD8pg  : I8o   <Idx0Pre, 0x70, "ld", "\t$dst, $src", "",
                      (outs), (ins ptr:$dst, GR8:$src),
                      [(store GR8:$src, iPTR:$dst)]>;
  def LD88pr : Pseudo<               "ld", "\t$dst, $src", "",
                      (outs), (ins ptr:$dst, R16:$src),
                      [(store R16:$src, iPTR:$dst)]>;

  def LD8or  : Pseudo<               "ld", "\t$dst, $src", "",
                      (outs), (ins off:$dst, RR8:$src)>;
  def LD8og  : I8o   <Idx0Pre, 0x70, "ld", "\t$dst, $src", "",
                      (outs), (ins off:$dst, GR8:$src),
                      [(store GR8:$src, offpat:$dst)]>;
}
let mayStore = 1 in {
  def LD88or : Pseudo<               "ld", "\t$dst, $src", "",
                      (outs), (ins off:$dst, R16:$src),
                      [(store R16:$src, offpat:$dst)]>;
}
// Same as above.

let mayStore = 1 in {
  def LD8pi  : I8oi <Idx0Pre, 0x36, "ld", "\t$dst, $src", "",
                     (outs), (ins ptr:$dst, i8imm:$src),
                     [(store (i8 imm:$src),   iPTR:$dst)]>;
  def LD8oi  : I8oi <Idx0Pre, 0x36, "ld", "\t$dst, $src", "",
                     (outs), (ins off:$dst, i8imm:$src),
                     [(store (i8 imm:$src), offpat:$dst)]>;
}

let Defs = [SPS] in
def LD16SP : I16<Idx0Pre, 0xF9, "ld", "\tsp, $src", "", (outs), (ins AIR16:$src)>;

def EXAF : I<NoPre, 0x08, "ex", "\taf, af'">;
def EXX  : I<NoPre, 0xD9, "exx">;

let Defs = [DE, HL], Uses = [DE, HL] in
def EX16DE : I16<NoPre, 0xEB, "ex", "\tde, hl">;

let Constraints = "$imp = $dst" in {
let Uses = [SPS] in
def EX16SP : I16<Idx0Pre, 0xE3, "ex", "\t(sp), $dst", "",
                 (outs AIR16:$dst), (ins AIR16:$imp)>;
}

let Defs = [SPS], Uses = [SPS] in {
  let mayLoad = 1 in
    def POP16r  : I16 <Idx0Pre, 0xC1, "pop", "\t$dst", "",
                       (outs R16:$dst), (ins), [(set R16:$dst, Z80pop)]>;
  let mayStore = 1 in
    def PUSH16r : I16 <Idx0Pre, 0xC5, "push", "\t$src", "",
                       (outs), (ins R16:$src), [(Z80push R16:$src)]>;

  // pushes and pops for 8 bit values (mainly for calling parameter pushes)
  // Theye are in front to be selected before their 16 bit counterpart
  let mayStore = 1 in
    def PUSH8r : Pseudo<"push8", "\t$src", "",
                       (outs), (ins GR8L:$src), [(Z80push8 GR8L:$src)]>;
}
let Defs = [AF, SPS], Uses = [SPS], mayLoad = 1 in
def POP16AF  : I16<NoPre, 0xF1, "pop", "\taf", "",
                   (outs), (ins), [(set AF, Z80pop)]>;
let Defs = [SPS], Uses = [AF, SPS], mayStore = 1 in
def PUSH16AF : I16<NoPre, 0xF5, "push", "\taf", "",
                   (outs), (ins), [(Z80push AF)]>;

let isReMaterializable = 1, Defs = [F] in {
  def RCF : P;
  def SCF : I<NoPre, 0x37, "scf">;
  def CCF : I<NoPre, 0x3F, "ccf">;
}

//===----------------------------------------------------------------------===//
//  Arithmetic Instructions.
//

let Defs = [A, F], Uses = [A] in {
  def CPL8 : I8<NoPre, 0x2F, "cpl", "", "", (outs), (ins), [(set A, ( not A))]>;
  def NEG8 : I8<EDPre, 0x44, "neg", "", "", (outs), (ins), [(set A, (ineg A))]>;
}

let Defs = [F] in
multiclass UnOp8RF<Prefix prefix, bits<8> opcode, string mnemonic,
                   Z80RC8 rc8 = !if(!eq(prefix.Value, CBPre.Value), GR8, RR8)> {
  def 8r : I8 <prefix, opcode, mnemonic, "\t$dst", "$imp = $dst",
               (outs rc8:$dst), (ins rc8:$imp),
               [(set rc8:$dst, F,
                       (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                           rc8:$imp))]>;
  def 8p : I8 <prefix, opcode, mnemonic, "\t$adr", "", (outs), (ins ptr:$adr),
               [(store (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                           (i8 (load iPTR:$adr))), iPTR:$adr),
                (implicit F)]>;
  def 8o : I8o<prefix, opcode, mnemonic, "\t$adr", "", (outs), (ins off:$adr),
               [(store (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                           (i8 (load offpat:$adr))), offpat:$adr),
                (implicit F)]>;
}
let Defs = [F], Uses = [F] in
multiclass UnOp8RFF<Prefix prefix, bits<8> opcode, string mnemonic,
                    Z80RC8 rc8 = !if(!eq(prefix.Value, CBPre.Value), GR8, RR8)> {
  def 8r : I8 <prefix, opcode, mnemonic, "\t$dst", "$imp = $dst",
               (outs rc8:$dst), (ins rc8:$imp),
               [(set rc8:$dst, F,
                       (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                           rc8:$imp, F))]>;
  def 8p : I8 <prefix, opcode, mnemonic, "\t$adr", "", (outs), (ins ptr:$adr),
               [(store (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                           (i8 (load iPTR:$adr)), F), iPTR:$adr),
                (implicit F)]>;
  def 8o : I8o<prefix, opcode, mnemonic, "\t$adr", "", (outs), (ins off:$adr),
               [(store (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                           (i8 (load offpat:$adr)), F), offpat:$adr),
                (implicit F)]>;
}
multiclass BinOp8RF<Prefix prefix, bits<3> opcode, string mnemonic,
                    bit compare = 0> {
  let isCompare = compare, Defs = [A, F], Uses = [A] in {
    def 8ar : I8 <prefix, {0b10, opcode, 0b000}, mnemonic, "\ta, $src", "",
                  (outs FR8:$dst), (ins    RR8:$src),
                  [(set FR8:$dst, F,
                        (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                            A, RR8:$src))]>;
    def 8ai : I8i<prefix, {0b11, opcode, 0b110}, mnemonic, "\ta, $src", "",
                  (outs FR8:$dst), (ins i8imm:$src),
                  [(set FR8:$dst, F,
                        (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                            A, imm:$src))]>;
    def 8ap : I8 <prefix, {0b10, opcode, 0b110}, mnemonic, "\ta, $src", "",
                  (outs FR8:$dst), (ins   ptr:$src),
                  [(set FR8:$dst, F,
                        (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                            A, (i8 (load   iPTR:$src))))]>;
    def 8ao : I8o<prefix, {0b10, opcode, 0b110}, mnemonic, "\ta, $src", "",
                  (outs FR8:$dst), (ins   off:$src),
                  [(set FR8:$dst, F,
                        (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                            A, (i8 (load offpat:$src))))]>;
  }
  def : Pat<(!cast<SDNode>(mnemonic) A,  RR8:$src),
            (!cast<Instruction>(!strconcat(NAME, "8ar"))  RR8:$src)>;
  def : Pat<(!cast<SDNode>(mnemonic) A, imm:$src),
            (!cast<Instruction>(!strconcat(NAME, "8ai")) imm:$src)>;
  def : Pat<(!cast<SDNode>(mnemonic) A, (load iPTR:$src)),
            (!cast<Instruction>(!strconcat(NAME, "8ap")) ptr:$src)>;
  def : Pat<(!cast<SDNode>(mnemonic) A, (load offpat:$src)),
            (!cast<Instruction>(!strconcat(NAME, "8ao")) off:$src)>;
}
multiclass BinOp8RFF<Prefix prefix, bits<3> opcode, string mnemonic,
                     SDNode node, bit compare = 0> {
  let isCompare = compare, Defs = [A, F], Uses = [A, F] in {
    def 8ar : I8 <prefix,  {0b10, opcode, 0b000}, mnemonic, "\ta, $src", "",
                  (outs FR8:$dst), (ins    RR8:$src),
                  [(set FR8:$dst, F,
                        (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                            A, RR8:$src, F))]>;
    def 8ai : I8i<prefix,  {0b11, opcode, 0b110}, mnemonic, "\ta, $src", "",
                  (outs FR8:$dst), (ins i8imm:$src),
                  [(set FR8:$dst, F,
                        (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                            A, imm:$src, F))]>;
    def 8ap : I8 <prefix,  {0b10, opcode, 0b110}, mnemonic, "\ta, $src", "",
                  (outs FR8:$dst), (ins   ptr:$src),
                  [(set FR8:$dst, F,
                        (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                            A, (i8 (load iPTR:$src)), F))]>;
    def 8ao : I8o<Idx1Pre, {0b10, opcode, 0b110}, mnemonic, "\ta, $src", "",
                  (outs FR8:$dst), (ins   off:$src),
                  [(set FR8:$dst, F,
                        (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                            A, (i8 (load offpat:$src)), F))]>;
  }
  def : Pat<(node A,  RR8:$src),
            (!cast<Instruction>(!strconcat(NAME, "8ar"))  RR8:$src)>;
  def : Pat<(node A, imm:$src),
            (!cast<Instruction>(!strconcat(NAME, "8ai")) imm:$src)>;
  def : Pat<(node A, (load iPTR:$src)),
            (!cast<Instruction>(!strconcat(NAME, "8ap")) ptr:$src)>;
  def : Pat<(node A, (load offpat:$src)),
            (!cast<Instruction>(!strconcat(NAME, "8ao")) off:$src)>;
}
multiclass BinOp8F<Prefix prefix, bits<3> opcode, string mnemonic,
                   bit compare = 0> {
  let isCompare = compare, Defs = [F], Uses = [A] in {
    def 8ar : I8 <prefix,  {0b10, opcode, 0b000}, mnemonic, "\ta, $src", "",
                  (outs), (ins    RR8:$src),
                  [(set F,
                        (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                            A, RR8:$src))]>;
    def 8ai : I8i<prefix,  {0b11, opcode, 0b110}, mnemonic, "\ta, $src", "",
                  (outs), (ins i8imm:$src),
                  [(set F,
                        (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                            A, imm:$src))]>;
    def 8ap : I8 <prefix,  {0b10, opcode, 0b110}, mnemonic, "\ta, $src", "",
                  (outs), (ins   ptr:$src),
                  [(set F,
                        (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                            A, (i8 (load iPTR:$src))))]>;
    def 8ao : I8o<Idx0Pre, {0b10, opcode, 0b110}, mnemonic, "\ta, $src", "",
                 (outs), (ins   off:$src),
                 [(set F,
                       (!cast<SDNode>(!strconcat("Z80", mnemonic, "_flag"))
                           A, (i8 (load offpat:$src))))]>;
  }
}
defm RLC : UnOp8RF  <CBPre, 0, "rlc">;
defm RRC : UnOp8RF  <CBPre, 1, "rrc">;
defm RL  : UnOp8RFF <CBPre, 2, "rl">;
defm RR  : UnOp8RFF <CBPre, 3, "rr">;
defm SLA : UnOp8RF  <CBPre, 4, "sla">;
defm SRA : UnOp8RF  <CBPre, 5, "sra">;
defm SRL : UnOp8RF  <CBPre, 7, "srl">;
defm INC : UnOp8RF  <NoPre, 4, "inc">;
def : Pat<(add RR8:$reg, 1), (INC8r RR8:$reg)>;
defm DEC : UnOp8RF  <NoPre, 5, "dec">;
def : Pat<(add RR8:$reg, -1), (DEC8r RR8:$reg)>;
defm ADD : BinOp8RF <NoPre, 0, "add">;
defm ADC : BinOp8RFF<NoPre, 1, "adc", adde>;
defm SUB : BinOp8RF <NoPre, 2, "sub", 1>;
defm SBC : BinOp8RFF<NoPre, 3, "sbc", sube>;
defm AND : BinOp8RF <NoPre, 4, "and">;
defm XOR : BinOp8RF <NoPre, 5, "xor">;
defm OR  : BinOp8RF <NoPre, 6, "or">;
defm CP  : BinOp8F  <NoPre, 7, "cp",  1>;

let Defs = [F] in {
def INC16r : I16<Idx0Pre, 0x03, "inc", "\t$dst", "$imp = $dst",
                 (outs R16:$dst), (ins R16:$imp),
                 [(set R16:$dst, F, (Z80inc_flag R16:$imp))]>;
def DEC16r : I16<Idx0Pre, 0x0B, "dec", "\t$dst", "$imp = $dst",
                 (outs R16:$dst), (ins R16:$imp),
                 [(set R16:$dst, F, (Z80dec_flag R16:$imp))]>;

}
let Defs = [SPS, F], Uses = [SPS] in {
def INC16SP : I16<NoPre, 0x33, "inc", "\tsp", "",
                  (outs SR16:$dst), (ins), [(set SR16:$dst, F, (Z80inc_flag SPS))]>;
def DEC16SP : I16<NoPre, 0x3B, "dec", "\tsp", "",
                  (outs SR16:$dst), (ins), [(set SR16:$dst, F, (Z80dec_flag SPS))]>;
}
def : Pat<(add R16:$imp,  1), (INC16r R16:$imp)>;
def : Pat<(add R16:$imp, -1), (DEC16r R16:$imp)>;

let Defs = [F] in {
  def ADD16aa : I16<Idx0Pre, 0x29, "add", "\t$dst, $imp", "$imp = $dst",
                    (outs AR16:$dst), (ins AR16:$imp),
                    [(set AR16:$dst, F, (Z80add_flag AR16:$imp, AR16:$imp))]>;
  def ADD16ao : I16<Idx0Pre, 0x09, "add", "\t$dst, $src", "$imp = $dst",
                    (outs AR16:$dst), (ins AR16:$imp, OR16:$src),
                    [(set AR16:$dst, F, (Z80add_flag AR16:$imp, OR16:$src))]>;
  let Uses = [SPS] in
  def ADD16SP : I16<Idx0Pre, 0x39, "add", "\t$dst, sp",   "$imp = $dst",
                   (outs AIR16:$dst), (ins AIR16:$imp),
                   [(set AIR16:$dst, F, (Z80add_flag AIR16:$imp, SPS))]>;
}
def : Pat<(add  AR16:$imp, AR16:$imp), (ADD16aa AR16:$imp)>;
def : Pat<(addc AR16:$imp, AR16:$imp), (ADD16aa AR16:$imp)>;
def : Pat<(add  AR16:$imp, OR16:$src), (ADD16ao AR16:$imp, OR16:$src)>;
def : Pat<(addc AR16:$imp, OR16:$src), (ADD16ao AR16:$imp, OR16:$src)>;

let Defs = [HL, F] in {
  let Uses = [HL, F] in {
    def SBC16aa : I16<EDPre, 0x42, "sbc", "\thl, hl", "", (outs HR16:$dst), (ins),
                      [(set  HR16:$dst, F, (Z80sbc_flag  HL, HL,       F))]>;
    def ADC16aa : I16<EDPre, 0x4A, "adc", "\thl, hl", "", (outs HR16:$dst), (ins),
                      [(set  HR16:$dst, F, (Z80adc_flag  HL, HL,       F))]>;
    def SBC16ao : I16<EDPre, 0x42, "sbc", "\thl, $src", "", (outs HR16:$dst), (ins OR16:$src),
                      [(set  HR16:$dst, F, (Z80sbc_flag  HL, OR16:$src, F))]>;
    def ADC16ao : I16<EDPre, 0x4A, "adc", "\thl, $src", "", (outs HR16:$dst), (ins OR16:$src),
                      [(set  HR16:$dst, F, (Z80adc_flag  HL, OR16:$src, F))]>;
  }
  let Uses = [HL, SPS, F] in {
    def SBC16SP : I16<EDPre, 0x72, "sbc", "\thl, sp", "", (outs HR16:$dst), (ins),
                      [(set  HR16:$dst, F, (Z80sbc_flag HL, SPS, F))]>;
    def ADC16SP : I16<EDPre, 0x7A, "adc", "\thl, sp", "", (outs HR16:$dst), (ins),
                      [(set  HR16:$dst, F, (Z80adc_flag HL, SPS, F))]>;
  }
}
def : Pat<(sube  HL, OR16:$src), (SBC16ao OR16:$src)>;
def : Pat<(adde  HL, OR16:$src), (ADC16ao OR16:$src)>;

let Defs = [HL, F], Uses = [HL] in {
  def SUB16ao : P<(outs HR16:$dst), (ins OR16:$src),
                  [(set  HR16:$dst, F, (Z80sub_flag  HL, OR16:$src))]>;
}
let Defs = [F] in {
  let Uses = [HL] in {
    def CP16a0 : P<(outs), (ins), [(set F, (Z80cp_flag HL, 0))]>;
    def CP16ao : P<(outs), (ins OR16:$src),
                   [(set F, (Z80cp_flag HL, OR16:$src))]>;
  }
}
def : Pat<(sub   HL, OR16:$src), (SUB16ao OR16:$src)>;
def : Pat<(subc  HL, OR16:$src), (SUB16ao OR16:$src)>;

def LD16rfi: P<(outs R16:$dst), (ins off16:$src),
                [(set R16:$dst, fipat:$src)]>;

//def MLT8rr : I8<EDPre, 0x4C, "mlt", "\t$dst", "$imp = $dst",
//                (outs GR16:$dst), (ins GR16:$imp),
//                [(set GR16:$dst, (Z80mlt GR16:$imp))]>, Requires<[HaveZ180Ops]>; // $TODO: HaveR800Ops

//===----------------------------------------------------------------------===//
// Non-Instruction Patterns.
//===----------------------------------------------------------------------===//

// addresses
def : Pat<(i16 (Z80Wrapper tglobaladdr  :$src)), (LD16ri tglobaladdr  :$src)>;
def : Pat<(i16 (Z80Wrapper texternalsym :$src)), (LD16ri texternalsym :$src)>;
def : Pat<(i16 (Z80Wrapper tblockaddress:$src)), (LD16ri tblockaddress:$src)>;

// calls
def : Pat<(Z80call (tglobaladdr :$dst)), (CALL16i tglobaladdr :$dst)>;
def : Pat<(Z80call (texternalsym:$dst)), (CALL16i texternalsym:$dst)>;

def : Pat<(Z80tcret (tglobaladdr :$dst)), (TCRETURN16i tglobaladdr :$dst)>;
def : Pat<(Z80tcret (texternalsym:$dst)), (TCRETURN16i texternalsym:$dst)>;

//===----------------------------------------------------------------------===//
// Subsystems.
//===----------------------------------------------------------------------===//

// truncs
def : Pat<(i8  (trunc R16:$src)), (EXTRACT_SUBREG R16:$src, sub_low)>;
