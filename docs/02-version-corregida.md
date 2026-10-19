# bruteforce_fix.c (versión corregida e instrumentada)

Mantiene la estructura de `bruteforce_.c` (`encrypt`, `decrypt`, `tryKey`, `main` con `MPI_Irecv`/`MPI_Test`)
y corrige los hallazgos de `01-hallazgos-codigo.md`.

## Uso
```bash
mpirun -np N build/bruteforce_fix <llave> <archivo_texto> "<frase clave>" [block|cyclic]
```
- `block`: bloques contiguos (igual al original). `cyclic`: el proceso `id` prueba `id, id+N, id+2N, ...`.
- Compilar: `scripts/build.sh` (desde WSL). Prueba rápida: `scripts/smoke.sh`.

## Salida
- `PROC,<rank>,<llaves probadas>,<segundos>` por proceso.
- `CSV,<N>,<modo>,<llave real>,<llave hallada>,<tiempo>` (tiempo = máximo entre procesos), listo para el análisis.

## Cambios
| Hallazgo | Cambio |
|---|---|
| 1 | `ecb_crypt` usa `ciphlen` (múltiplo de 8, padding con ceros) |
| 2, 4 | Texto y frase por argumentos |
| 3 | `0xFEUL` y llaves `unsigned long` |
| 5 | La llave se elige por argumento |
| 6 | `MPI_Reduce(MAX)` + `MPI_Cancel` del `Irecv`: termina aunque no se halle la llave |
| 8 | `encrypt(key, ciph, len)` |
| 9 | `MPI_Test` cada 1024 llaves |

## Prueba de humo (llave 3 000 000, texto corto)
| N | Modo | Tiempo (s) | Llaves por proceso |
|---|---|---|---|
| 1 | block | 4.26 | 3 000 000 |
| 4 | block | 4.49 | ~3 000 000 en cada uno (la llave está en el bloque del proceso 0) |
| 4 | cyclic | 1.12 | ~750 000 en cada uno |

Con bloques contiguos, una llave baja cae en el bloque del proceso 0 y los demás solo esperan: no hay
speedup (4.26 s → 4.49 s). Con reparto cíclico el trabajo se divide parejo (speedup ≈ 3.8).
Velocidad aproximada: ~0.7 M llaves/s por proceso.
