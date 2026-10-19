#!/bin/bash
# Verifica que el texto real se cifra/descifra y se encuentra con una llave pequena
cd "$(dirname "$0")/.."
mpirun --oversubscribe -np 4 build/bruteforce_fix 2000000 data/texto.txt "memoria distribuida" cyclic | grep -E "CSV|Key|PROC"
