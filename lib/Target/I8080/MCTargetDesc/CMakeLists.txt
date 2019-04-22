# MCTargetDesc/CMakeLists.txt
add_llvm_library(LLVMI8080Desc
  I8080MCAsmInfo.cpp
  I8080MCTargetDesc.cpp
  I8080TargetStreamer.cpp
  )
