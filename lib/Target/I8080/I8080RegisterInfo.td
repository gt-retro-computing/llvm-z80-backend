//===- I8080RegisterInfo.td - Describe the I8080 Register File --*- tablegen -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the I8080 Register file, defining the registers themselves,
// aliases between the registers, and the register classes built out of the
// registers.
//
//===----------------------------------------------------------------------===//

class I8080Reg<string n, bits<16> Enc = -1, list<Register> subregs = []>
  : Register<n> {
  let Namespace = "I8080";
  let HWEncoding = Enc;
  let SubRegs = subregs;
}

// Subregister indices.
let Namespace = "I8080" in {
  def sub_low   : SubRegIndex<8>;
  def sub_high  : SubRegIndex<8, 8>;
  def sub_short : SubRegIndex<16>;
}

//===----------------------------------------------------------------------===//
//  Register definitions...
//

// 8-bit registers
def A : I8080Reg<"a", 7>;
def F : I8080Reg<"f">;
def B : I8080Reg<"b", 0>;
def C : I8080Reg<"c", 1>;
def D : I8080Reg<"d", 2>;
def E : I8080Reg<"e", 3>;
def H : I8080Reg<"h", 4>;
def L : I8080Reg<"l", 5>;

// 8-bit index registers
let CostPerUse = 1 in {
def IXH : I8080Reg<"ixh", 4>;
def IXL : I8080Reg<"ixl", 5>;
def IYH : I8080Reg<"iyh", 4>;
def IYL : I8080Reg<"iyl", 5>;
}

let SubRegIndices = [sub_high, sub_low], CoveredBySubRegs = 1 in {
// 16-bit registers
def AF : I8080Reg<"af", 3, [A,F]>;
def BC : I8080Reg<"bc", 0, [B,C]>;
def DE : I8080Reg<"de", 1, [D,E]>;
def HL : I8080Reg<"hl", 2, [H,L]>;
// 16-bit index registers
let CostPerUse = 1 in {
def IX : I8080Reg<"ix", 2, [IXH,IXL]>;
def IY : I8080Reg<"iy", 2, [IYH,IYL]>;
}
}
def SPS : I8080Reg<"sp", 3>;

def PC  : I8080Reg<"pc">;

//===----------------------------------------------------------------------===//
//  Register Class Definitions...
//

class I8080RC8 <dag regList> : RegisterClass<"I8080", [i8],  8, regList>;
class I8080RC16<dag regList> : RegisterClass<"I8080", [i16], 8, regList>;

// Status flags registers.
// Don't allow copying of status registers.
let CopyCost = -1, isAllocatable = 0 in
def FR8  : I8080RC8 <(add F)>;

def GR8  : I8080RC8 <(add A, L, E, C, H, D, B)>;
def OR8  : I8080RC8 <(add A, E, C, D, B)>;

def GR8L  : I8080RC8 <(add A, L, E, C)>; // pushable 8 bit registers

//def Y8  : I8080RC8 <(add OR8, IYL, IYH)>;
//def X8  : I8080RC8 <(add OR8, IXL, IXH)>;
//def I8  : I8080RC8 <(add IYL, IYH, IXL, IXH)>;

//def R8  : I8080RC8 <(add GR8, I8)>;
def RR8  : I8080RC8 <(add GR8)>;

def OR16 : I8080RC16<(add DE, BC)>;
def GR16 : I8080RC16<(add HL, OR16)>;
//def XR16 : I8080RC16<(add IX, OR16)>;
//def YR16 : I8080RC16<(add IY, OR16)>;
//def IR16 : I8080RC16<(add IX)>;
def IR16 : I8080RC16<(add IY, IX)>;
def AR16 : I8080RC16<(add HL)>;
def AIR16 : I8080RC16<(add HL, IR16)>;
def R16 : I8080RC16<(add GR16, IR16)>;

def SR16 : I8080RC16<(add SPS)>;
def HR16 : I8080RC16<(add HL)>;

// Do not comment!
// Add all registers from that are not contained in any other register class
// to avoid the "Not enough results" assertion from table generator.
def Z16 : I8080RC16<(add AF)>;
