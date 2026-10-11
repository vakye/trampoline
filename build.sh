#!/bin/bash

Compiler="clang"

SourceFile="example.c"
OutputFile="example"

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
    -Wno-unused-function \
    -o $OutputFile"

LinkFlags=" \
    -fuse-ld=lld \
    -Wl,-nostdlib \
    -Wl,--entry,trEntryPoint \
    -Wl,-lc \
    -Wl,-lwayland-client"

$Compiler $CompileFlags $SourceFile $LinkFlags

