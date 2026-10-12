#!/bin/bash

GLSLCompiler="glslangValidator"
GLSLCompileFlags="--target-env vulkan1.4 -x"

for SourceFile in *.vert *.frag; do
    OutputFile="${SourceFile}.h"
    $GLSLCompiler $GLSLCompileFlags $SourceFile -o $OutputFile
done

