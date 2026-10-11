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
    -nostdlib \
    -std=c11 \
    -Wall -Wextra -Wpedantic -Werror \
    -Wno-unused-parameter \
    -Wno-unused-variable \
    -Wno-unused-but-set-variable \
    -Wno-unused-function \
    -o $OutputFile"

LinkFlags=" \
    -fuse-ld=lld \
    -Wl,-nostdlib \
    -Wl,--entry,trEntryPoint"

$Compiler $CompileFlags $SourceFile $LinkFlags

