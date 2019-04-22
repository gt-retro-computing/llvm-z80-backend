//===-- I8080CallingConv.td - Calling Conventions I8080 ---*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This describes the calling conventions for the I8080 architectures.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// I8080 Argument Calling Conventions
//===----------------------------------------------------------------------===//

def CC_I8080_C : CallingConv<[
  CCIfByVal<CCPassByVal<2, 1>>,
  CCIfType<[i1], CCPromoteToType<i8>>,

  // The first 3 integer arguments, if the call is not
  // a vararg call, are passed in integer registers.
  //CCIfNotVarArg<CCIfType<[i8], CCAssignToReg<[A, E, C]>>>,
  //CCIfNotVarArg<CCIfType<[i8], CCAssignToReg<[H, L, E, C]>>>,
  //CCIfNotVarArg<CCIfType<[i16], CCAssignToReg<[HL, DE, BC]>>>,

  CCIfType<[i8, i16], CCAssignToStack<2, 1>>
//  CCIfType<[i32, f32], CCAssignToStack<4, 1>>,
//  CCIfType<[i64, f64], CCAssignToStack<8, 1>>
]>;
//def CC_I8080_C : CallingConv<[
//  CCIfByVal<CCPassByVal<2, 1>>,
//  CCIfType<[i1, i8], CCPromoteToType<i16>>,
//  CCIfType<[i16], CCAssignToStack<2, 1>>,
//  CCIfType<[i32, f32], CCAssignToStack<4, 1>>,
//  CCIfType<[i64, f64], CCAssignToStack<8, 1>>
//]>;

//def CC_EI8080_C : CallingConv<[
//  CCIfByVal<CCPassByVal<3, 1>>,
//  CCIfType<[i1, i8, i16], CCPromoteToType<i24>>,
//  CCIfType<[i24], CCAssignToStack<3, 1>>,
//  CCIfType<[i32, f32, i48], CCAssignToStack<6, 1>>,
//  CCIfType<[i64, f64], CCAssignToStack<9, 1>>
//]>;

//def CC_EI8080_LC : CallingConv<[
////  CCIfType<[i24], CCIfSplit<CCAssignToReg<[UHL, UBC]>>>,
////  CCIfType<[i24], CCIfSplitEnd<CCAssignToReg<[UDE, UIY]>>>,
////  CCIfType<[i24], CCAssignToRegWithShadow<[UHL, UBC], [UDE, UIY]>>,
//  CCIfType<[i16], CCIfSplit<CCAssignToReg<[HL, BC]>>>,
//  CCIfType<[i16], CCIfSplitEnd<CCAssignToReg<[DE, IY]>>>,
//  CCIfType<[i16], CCAssignToRegWithShadow<[HL, BC], [DE, IY]>>
//]>;
//def CC_EI8080_LC_AB : CallingConv<[
//  CCDelegateTo<CC_EI8080_LC>,
//  CCIfType<[i8], CCAssignToReg<[A, B]>>
//]>;
//def CC_EI8080_LC_AC : CallingConv<[
//  CCDelegateTo<CC_EI8080_LC>,
//  CCIfType<[i8], CCAssignToReg<[A, C]>>
//]>;
//def CC_EI8080_LC_BC : CallingConv<[
//  CCDelegateTo<CC_EI8080_LC>,
//  CCIfType<[i8], CCAssignToReg<[B, C]>>
//]>;
//def CC_EI8080_LC_C  : CallingConv<[
//  CCDelegateTo<CC_EI8080_LC>,
//  CCIfType<[i8], CCAssignToReg<[C]>>
//]>;
//def CC_EI8080_LC_L  : CallingConv<[
//  CCIfType<[i24], CCAssignToReg<[UBC]>>,
//  CCDelegateTo<CC_EI8080_LC>,
//  CCIfType<[i8], CCAssignToReg<[A, L]>>
//]>;

//===----------------------------------------------------------------------===//
// Return Value Calling Conventions
//===----------------------------------------------------------------------===//
def RetCC_I8080_C : CallingConv<[
//  CCIfType<[i16], CCAssignToReg<[HL, DE, BC, IY]>>,
//  CCIfType<[i8], CCAssignToReg<[A, L, H, E, D, C, B, IYL, IYH]>>
  CCIfType<[i8], CCAssignToReg<[A]>>,
  CCIfType<[i16], CCAssignToReg<[HL]>>
]>;
//def RetCC_EI8080_C : CallingConv<[
//  CCDelegateTo<CC_EI8080_LC>,
//  CCIfType<[i8], CCAssignToReg<[A, E]>>
//]>;
//def RetCC_EI8080_LC_L : CallingConv<[
//  CCIfType<[i24], CCAssignToReg<[UBC]>>,
//  CCIfType<[i8], CCAssignToReg<[A]>>
//]>;

//===----------------------------------------------------------------------===//
// Callee-saved Registers.
//===----------------------------------------------------------------------===//

def CSR_NoRegs       : CalleeSavedRegs<(add)>;
def CSR_I8080_C : CalleeSavedRegs<(add IX)>;
//def CSR_EI8080_C       : CalleeSavedRegs<(add UIX)>;
def CSR_I8080_AllRegs  : CalleeSavedRegs<(add R16, AF)>;
//def CSR_EI8080_AllRegs : CalleeSavedRegs<(add R24, AF)>;
