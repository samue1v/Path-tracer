#!/bin/bash

mkdir -p compiled

for shader in *.vert *.frag; do
    if [[ -f "$shader" ]]; then
        output="compiled/$shader.spv"
        echo "Compiling $shader -> $output"
        glslc "$shader" -o "$output"
        if [[ $? -ne 0 ]]; then
            echo "Failed to compile $shader"
        fi
    fi
done

echo "Compilation finished."
