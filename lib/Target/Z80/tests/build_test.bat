clang --target=z80 -S -emit-llvm %1.c
llc -filetype=asm -march=Z80 %1.ll
