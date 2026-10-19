#!/bin/bash
# Experimento del punto 6: tiempo vs llave, N procesos y modo de reparto.
# Espacio de busqueda a escala: 2^23 llaves (ver docs/06-mediciones.md).
# Reanudable: omite las combinaciones que ya estan en results/mediciones.csv.
cd "$(dirname "$0")/.."
mkdir -p results
BITS=23
REPS=3
KEYS="419430 2097152 4206649 6291456 7969177"   # ~5%, 25%, 50% (llave elegida), 75%, 95%
NS="1 2 4 8 16"
OUT=results/mediciones.csv
PROCS=results/procesos.csv
[ -f $OUT ] || echo "N,modo,llave,rep,tiempo" > $OUT
[ -f $PROCS ] || echo "N,modo,llave,rep,proc,llaves_probadas,tiempo" > $PROCS

for key in $KEYS; do
  for n in $NS; do
    for modo in block cyclic; do
      [ "$n" = 1 ] && [ "$modo" = cyclic ] && continue   # con N=1 ambos modos son identicos
      for rep in $(seq 1 $REPS); do
        grep -q "^$n,$modo,$key,$rep," $OUT && continue
        res=$(mpirun --oversubscribe --bind-to none -np $n build/bruteforce_fix \
              $key data/texto.txt "memoria distribuida" $modo $BITS)
        t=$(echo "$res" | grep '^CSV' | cut -d, -f6)
        [ -z "$t" ] && { echo "FALLO n=$n modo=$modo key=$key rep=$rep"; continue; }
        echo "$res" | grep '^PROC' | awk -F, -v n=$n -v m=$modo -v k=$key -v r=$rep \
          '{print n","m","k","r","$2","$3","$4}' >> $PROCS
        echo "$n,$modo,$key,$rep,$t" >> $OUT
        echo "n=$n modo=$modo key=$key rep=$rep t=$t"
      done
    done
  done
done
echo FIN
