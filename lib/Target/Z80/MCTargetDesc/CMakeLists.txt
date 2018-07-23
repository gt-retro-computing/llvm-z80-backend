# MCTargetDesc/CMakeLists.txt
add_llvm_library(LLVMZ80Desc
  Z80MCAsmInfo.cpp
  Z80MCTargetDesc.cpp
  Z80TargetStreamer.cpp
  )
