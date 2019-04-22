
set(LLVM_TARGET_DEFINITIONS I8080.td)

# Generate I8080GenRegisterInfo.inc and I8080GenInstrInfo.inc which included by
#  your hand code C++ files. 
# I8080GenRegisterInfo.inc came from I8080RegisterInfo.td, I8080GenInstrInfo.inc
#  came from I8080InstrInfo.td.
tablegen(LLVM I8080GenRegisterInfo.inc -gen-register-info)
tablegen(LLVM I8080GenInstrInfo.inc -gen-instr-info)
tablegen(LLVM I8080GenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM I8080GenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM I8080GenDAGISel.inc -gen-dag-isel)
tablegen(LLVM I8080GenCallingConv.inc -gen-callingconv)
tablegen(LLVM I8080GenSubtargetInfo.inc -gen-subtarget)

# I8080CommonTableGen must be defined
add_public_tablegen_target(I8080CommonTableGen)

# I8080CodeGen should match with LLVMBuild.txt I8080CodeGen
add_llvm_target(
I8080CodeGen
I8080AsmPrinter.cpp
I8080CallFrameOptimization.cpp
I8080ExpandPseudo.cpp
I8080FrameLowering.cpp
I8080InstrInfo.cpp
I8080ISelDAGToDAG.cpp
I8080ISelLowering.cpp
I8080MachineFunctionInfo.cpp
I8080MachineLateOptimization.cpp
I8080MCInstLower.cpp
I8080RegisterInfo.cpp
I8080Subtarget.cpp
I8080TargetMachine.cpp
I8080TargetObjectFile.cpp
  )

# Should match with "subdirectories =  MCTargetDesc TargetInfo" in LLVMBuild.txt
add_subdirectory(InstPrinter)
add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)

