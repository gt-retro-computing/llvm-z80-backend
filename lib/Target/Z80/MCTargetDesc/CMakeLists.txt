# MCTargetDesc/CMakeLists.txt
add_llvm_library(LLVMZ80Desc
        I8080MCAsmInfo.cpp
        I8080MCTargetDesc.cpp
  Z80TargetStreamer.cpp
  )
