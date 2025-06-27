#!/usr/bin/env sh

#Compile using 16 threads (my home-machine is 8-core)
N_THREADS=16

#Build type. Mostly irrelevant
type=Release

# Generate/update cmake build directory
cmake -Htool -Bbuild/$type -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -DCMAKE_BUILD_TYPE=$type -DCMAKE_EXE_LINKER_FLAGS=-latomic || { echo "CMAKE failed"; exit 1; }

# Build project
cmake --build build/$type --parallel $N_THREADS --config $type "$@" || { echo "Build failed"; exit 1; }

#Link compile_commands.json
# for ccls (C language server, IDE shennanigans)
ln -fs build/$type/compile_commands.json .

#Link executable
#ln -fs build/$type/runner .
