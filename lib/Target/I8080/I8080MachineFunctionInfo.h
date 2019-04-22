//===-- I8080MachineFunctionInfo.h - I8080 machine function info ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares I8080-specific per-machine-function information.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_I8080_I8080MACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_I8080_I8080MACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {

/// I8080MachineFunctionInfo - This class is derived from MachineFunction and
/// contains private I8080 target-specific information for each MachineFunction.
class I8080MachineFunctionInfo : public MachineFunctionInfo {
  virtual void anchor();

  /// CalleeSavedFrameSize - Size of the callee-saved register portion of the
  /// stack frame in bytes.
  unsigned CalleeSavedFrameSize = 0;

  /// BytesToPopOnReturn - Number of bytes function pops on return (in addition
  /// to the space used by the return address).
  /// Used on windows platform for stdcall & fastcall name decoration
  unsigned BytesToPopOnReturn = 0;

  /// VarArgsFrameIndex - FrameIndex for start of varargs area.
  int VarArgsFrameIndex = 0;
  /// RegSaveFrameIndex - X86-64 vararg func register save area.
  int RegSaveFrameIndex = 0;
  /// ArgumentStackSize - The number of bytes on stack consumed by the arguments
  /// being passed on the stack.
  unsigned ArgumentStackSize = 0;

  /// SRetReturnReg - Some subtargets require that sret lowering includes
  /// returning the value of the returned struct in a register. This field
  /// holds the virtual register into which the sret argument is passed.
  unsigned SRetReturnReg = 0;

public:
  I8080MachineFunctionInfo() = default;

  explicit I8080MachineFunctionInfo(MachineFunction &MF) {}

  unsigned getCalleeSavedFrameSize() const { return CalleeSavedFrameSize; }
  void setCalleeSavedFrameSize(unsigned Bytes) { CalleeSavedFrameSize = Bytes; }

  int getVarArgsFrameIndex() const { return VarArgsFrameIndex; }
  void setVarArgsFrameIndex(int Idx) { VarArgsFrameIndex = Idx; }

  int getRegSaveFrameIndex() const { return RegSaveFrameIndex; }
  void setRegSaveFrameIndex(int Idx) { RegSaveFrameIndex = Idx; }

  unsigned getArgumentStackSize() const { return ArgumentStackSize; }
  void setArgumentStackSize(unsigned size) { ArgumentStackSize = size; }

  unsigned getSRetReturnReg() const { return SRetReturnReg; }
  void setSRetReturnReg(unsigned Reg) { SRetReturnReg = Reg; }

  unsigned getBytesToPopOnReturn() const { return BytesToPopOnReturn; }
  void setBytesToPopOnReturn(unsigned bytes) { BytesToPopOnReturn = bytes;}
};

} // End llvm namespace

#endif
