
set(LLVM_TARGET_DEFINITIONS Z80old.td)

# Generate Z80oldGenRegisterInfo.inc and Z80oldGenInstrInfo.inc which included by 
#  your hand code C++ files. 
# Z80oldGenRegisterInfo.inc came from Z80oldRegisterInfo.td, Z80oldGenInstrInfo.inc 
#  came from Z80oldInstrInfo.td.
tablegen(LLVM Z80oldGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM Z80oldGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM Z80oldGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM Z80oldGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM Z80oldGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM Z80oldGenCallingConv.inc -gen-callingconv)
tablegen(LLVM Z80oldGenSubtargetInfo.inc -gen-subtarget)

# Z80oldCommonTableGen must be defined
add_public_tablegen_target(Z80oldCommonTableGen)

# Z80oldCodeGen should match with LLVMBuild.txt Z80oldCodeGen
add_llvm_target(
Z80oldCodeGen
Z80oldAsmPrinter.cpp
Z80oldCallFrameOptimization.cpp
Z80oldExpandPseudo.cpp
Z80oldFrameLowering.cpp
Z80oldInstrInfo.cpp
Z80oldISelDAGToDAG.cpp
Z80oldISelLowering.cpp
Z80oldMachineFunctionInfo.cpp
Z80oldMachineLateOptimization.cpp
Z80oldMCInstLower.cpp
Z80oldRegisterInfo.cpp
Z80oldSubtarget.cpp
Z80oldTargetMachine.cpp
Z80oldTargetObjectFile.cpp
  )

# Should match with "subdirectories =  MCTargetDesc TargetInfo" in LLVMBuild.txt
add_subdirectory(InstPrinter)
add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)

