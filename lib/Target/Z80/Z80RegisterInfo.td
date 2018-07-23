//===- Z80RegisterInfo.td - Describe the Z80 Register File --*- tablegen -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the Z80 Register file, defining the registers themselves,
// aliases between the registers, and the register classes built out of the
// registers.
//
//===----------------------------------------------------------------------===//

class Z80Reg<string n, bits<16> Enc = -1, list<Register> subregs = []>
  : Register<n> {
  let Namespace = "Z80";
  let HWEncoding = Enc;
  let SubRegs = subregs;
}

// Subregister indices.
let Namespace = "Z80" in {
  def sub_low   : SubRegIndex<8>;
  def sub_high  : SubRegIndex<8, 8>;
  def sub_short : SubRegIndex<16>;
}

//===----------------------------------------------------------------------===//
//  Register definitions...
//

// 8-bit registers
def A : Z80Reg<"a", 7>;
def F : Z80Reg<"f">;
def B : Z80Reg<"b", 0>;
def C : Z80Reg<"c", 1>;
def D : Z80Reg<"d", 2>;
def E : Z80Reg<"e", 3>;
def H : Z80Reg<"h", 4>;
def L : Z80Reg<"l", 5>;

// 8-bit index registers
let CostPerUse = 1 in {
def IXH : Z80Reg<"ixh", 4>;
def IXL : Z80Reg<"ixl", 5>;
def IYH : Z80Reg<"iyh", 4>;
def IYL : Z80Reg<"iyl", 5>;
}

let SubRegIndices = [sub_high, sub_low], CoveredBySubRegs = 1 in {
// 16-bit registers
def AF : Z80Reg<"af", 3, [A,F]>;
def BC : Z80Reg<"bc", 0, [B,C]>;
def DE : Z80Reg<"de", 1, [D,E]>;
def HL : Z80Reg<"hl", 2, [H,L]>;
// 16-bit index registers
let CostPerUse = 1 in {
def IX : Z80Reg<"ix", 2, [IXH,IXL]>;
def IY : Z80Reg<"iy", 2, [IYH,IYL]>;
}
}
def SPS : Z80Reg<"sp", 3>;

def PC  : Z80Reg<"pc">;

//===----------------------------------------------------------------------===//
//  Register Class Definitions...
//

class Z80RC8 <dag regList> : RegisterClass<"Z80", [i8],  8, regList>;
class Z80RC16<dag regList> : RegisterClass<"Z80", [i16], 8, regList>;

// Status flags registers.
// Don't allow copying of status registers.
let CopyCost = -1, isAllocatable = 0 in
def FR8  : Z80RC8 <(add F)>;

def GR8  : Z80RC8 <(add A, L, E, C, H, D, B)>;
def OR8  : Z80RC8 <(add A, E, C, D, B)>;

def GR8L  : Z80RC8 <(add A, L, E, C)>; // pushable 8 bit registers

//def Y8  : Z80RC8 <(add OR8, IYL, IYH)>;
//def X8  : Z80RC8 <(add OR8, IXL, IXH)>;
//def I8  : Z80RC8 <(add IYL, IYH, IXL, IXH)>;

//def R8  : Z80RC8 <(add GR8, I8)>;
def RR8  : Z80RC8 <(add GR8)>;

def OR16 : Z80RC16<(add DE, BC)>;
def GR16 : Z80RC16<(add HL, OR16)>;
//def XR16 : Z80RC16<(add IX, OR16)>;
//def YR16 : Z80RC16<(add IY, OR16)>;
//def IR16 : Z80RC16<(add IX)>;
def IR16 : Z80RC16<(add IY, IX)>;
def AR16 : Z80RC16<(add HL)>;
def AIR16 : Z80RC16<(add HL, IR16)>;
def R16 : Z80RC16<(add GR16, IR16)>;

def SR16 : Z80RC16<(add SPS)>;
def HR16 : Z80RC16<(add HL)>;

// Do not comment!
// Add all registers from that are not contained in any other register class
// to avoid the "Not enough results" assertion from table generator.
def Z16 : Z80RC16<(add AF)>;
