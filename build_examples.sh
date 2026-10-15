#!/bin/bash

Compiler="clang"

CompileFlags=" \
    -g \
    -O0 \
    -ffreestanding \
    -fpie \
    -fno-stack-protector \
    -std=c11 \
    -nostdlib \
    -mavx \
    -Wall -Wextra -Wpedantic -Werror \
    -Wno-unused-parameter \
    -Wno-unused-variable \
    -Wno-unused-but-set-variable \
    -Wno-unused-function"

LinkFlags=" \
    -fuse-ld=lld \
    -Wl,-nostdlib \
    -Wl,--entry,trEntryPoint \
    -Wl,-lc \
    -Wl,-lwayland-client \
    -Wl,-lxkbcommon"

for SourceFile in example*.c; do
    OutputFile=${SourceFile%.*}
    $Compiler $CompileFlags -o $OutputFile $SourceFile $LinkFlags
done

