//===- Z80oldRegisterInfo.td - Describe the Z80old Register File --*- tablegen -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the Z80old Register file, defining the registers themselves,
// aliases between the registers, and the register classes built out of the
// registers.
//
//===----------------------------------------------------------------------===//

class Z80oldReg<string n, bits<16> Enc = -1, list<Register> subregs = []>
  : Register<n> {
  let Namespace = "Z80old";
  let HWEncoding = Enc;
  let SubRegs = subregs;
}

// Subregister indices.
let Namespace = "Z80old" in {
  def sub_low   : SubRegIndex<8>;
  def sub_high  : SubRegIndex<8, 8>;
  def sub_short : SubRegIndex<16>;
}

//===----------------------------------------------------------------------===//
//  Register definitions...
//

// 8-bit registers
def A : Z80oldReg<"a", 7>;
def F : Z80oldReg<"f">;
def B : Z80oldReg<"b", 0>;
def C : Z80oldReg<"c", 1>;
def D : Z80oldReg<"d", 2>;
def E : Z80oldReg<"e", 3>;
def H : Z80oldReg<"h", 4>;
def L : Z80oldReg<"l", 5>;

// 8-bit index registers
let CostPerUse = 1 in {
def IXH : Z80oldReg<"ixh", 4>;
def IXL : Z80oldReg<"ixl", 5>;
def IYH : Z80oldReg<"iyh", 4>;
def IYL : Z80oldReg<"iyl", 5>;
}

let SubRegIndices = [sub_high, sub_low], CoveredBySubRegs = 1 in {
// 16-bit registers
def AF : Z80oldReg<"af", 3, [A,F]>;
def BC : Z80oldReg<"bc", 0, [B,C]>;
def DE : Z80oldReg<"de", 1, [D,E]>;
def HL : Z80oldReg<"hl", 2, [H,L]>;
// 16-bit index registers
let CostPerUse = 1 in {
def IX : Z80oldReg<"ix", 2, [IXH,IXL]>;
def IY : Z80oldReg<"iy", 2, [IYH,IYL]>;
}
}
def SPS : Z80oldReg<"sp", 3>;

def PC  : Z80oldReg<"pc">;

//===----------------------------------------------------------------------===//
//  Register Class Definitions...
//

class Z80oldRC8 <dag regList> : RegisterClass<"Z80old", [i8],  8, regList>;
class Z80oldRC16<dag regList> : RegisterClass<"Z80old", [i16], 8, regList>;

// Status flags registers.
// Don't allow copying of status registers.
let CopyCost = -1, isAllocatable = 0 in
def FR8  : Z80oldRC8 <(add F)>;

def GR8  : Z80oldRC8 <(add A, L, E, C, H, D, B)>;
def OR8  : Z80oldRC8 <(add A, E, C, D, B)>;

def GR8L  : Z80oldRC8 <(add A, L, E, C)>; // pushable 8 bit registers

//def Y8  : Z80oldRC8 <(add OR8, IYL, IYH)>;
//def X8  : Z80oldRC8 <(add OR8, IXL, IXH)>;
//def I8  : Z80oldRC8 <(add IYL, IYH, IXL, IXH)>;

//def R8  : Z80oldRC8 <(add GR8, I8)>;
def RR8  : Z80oldRC8 <(add GR8)>;

def OR16 : Z80oldRC16<(add DE, BC)>;
def GR16 : Z80oldRC16<(add HL, OR16)>;
//def XR16 : Z80oldRC16<(add IX, OR16)>;
//def YR16 : Z80oldRC16<(add IY, OR16)>;
//def IR16 : Z80oldRC16<(add IX)>;
def IR16 : Z80oldRC16<(add IY, IX)>;
def AR16 : Z80oldRC16<(add HL)>;
def AIR16 : Z80oldRC16<(add HL, IR16)>;
def R16 : Z80oldRC16<(add GR16, IR16)>;

def SR16 : Z80oldRC16<(add SPS)>;
def HR16 : Z80oldRC16<(add HL)>;

// Do not comment!
// Add all registers from that are not contained in any other register class
// to avoid the "Not enough results" assertion from table generator.
def Z16 : Z80oldRC16<(add AF)>;
