#!/bin/bash
cd "$(dirname "$0")/.."
for n in 1 4; do
  for m in block cyclic; do
    echo "== N=$n $m"
    timeout 120 mpirun --oversubscribe -np $n build/bruteforce_fix 3000000 build/prueba.txt "los mismos del proyecto 1" $m | grep -E "CSV|Key|PROC"
  done
done
