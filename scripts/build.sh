#!/bin/bash
# Compila las dos versiones en build/ (ejecutar desde WSL)
cd "$(dirname "$0")/.."
mkdir -p build
FLAGS="-Wall -Icompat -Wno-deprecated-declarations"
mpicc $FLAGS -o build/bruteforce_orig bruteforce_.c -lcrypto
mpicc $FLAGS -o build/bruteforce_fix bruteforce_fix.c -lcrypto
