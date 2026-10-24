#!/bin/bash

for SourceFile in example*.c; do
    ExecutableName=${SourceFile%.*}
    if [ -f $ExecutableName ]; then
        rm $ExecutableName
    fi
done

