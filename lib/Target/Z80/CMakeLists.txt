
set(LLVM_TARGET_DEFINITIONS Z80.td)

# Generate Z80GenRegisterInfo.inc and Z80GenInstrInfo.inc which included by 
#  your hand code C++ files. 
# Z80GenRegisterInfo.inc came from Z80RegisterInfo.td, Z80GenInstrInfo.inc 
#  came from Z80InstrInfo.td.
tablegen(LLVM Z80GenRegisterInfo.inc -gen-register-info)
tablegen(LLVM Z80GenInstrInfo.inc -gen-instr-info)
tablegen(LLVM Z80GenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM Z80GenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM Z80GenDAGISel.inc -gen-dag-isel)
tablegen(LLVM Z80GenCallingConv.inc -gen-callingconv)
tablegen(LLVM Z80GenSubtargetInfo.inc -gen-subtarget)

# Z80CommonTableGen must be defined
add_public_tablegen_target(Z80CommonTableGen)

# Z80CodeGen should match with LLVMBuild.txt Z80CodeGen
add_llvm_target(
Z80CodeGen
Z80AsmPrinter.cpp
Z80CallFrameOptimization.cpp
Z80ExpandPseudo.cpp
Z80FrameLowering.cpp
Z80InstrInfo.cpp
Z80ISelDAGToDAG.cpp
Z80ISelLowering.cpp
Z80MachineFunctionInfo.cpp
Z80MachineLateOptimization.cpp
Z80MCInstLower.cpp
Z80RegisterInfo.cpp
Z80Subtarget.cpp
Z80TargetMachine.cpp
Z80TargetObjectFile.cpp
  )

# Should match with "subdirectories =  MCTargetDesc TargetInfo" in LLVMBuild.txt
add_subdirectory(InstPrinter)
add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)

