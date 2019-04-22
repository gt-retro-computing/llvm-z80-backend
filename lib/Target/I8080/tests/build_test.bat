clang --target=z80 -S -emit-llvm %1.c
llc -filetype=asm -march=I8080 %1.ll
