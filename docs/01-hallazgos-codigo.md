# Hallazgos sobre bruteforce_.c

| # | Problema | Efecto | Corrección |
|---|---|---|---|
| 1 | `ecb_crypt(..., 16, ...)` fija el largo en 16; `len` se ignora | Solo se cifran los primeros 16 bytes; el resto queda en claro | Usar `len` redondeado a múltiplo de 8 (padding) |
| 2 | `search = " los mismos del proyecto 1 "` con espacios; el texto dice `(los mismos del proyecto 1)` | `strstr` nunca coincide: el programa no termina | Frase clave que sí aparezca en el texto |
| 3 | `0xFE << i*8` es `int` de 32 bits; desplazar >= 32 es UB | Bytes 4–7 de la llave mal formados; espacio efectivo reducido | `0xFEUL << i*8` y `key` como `unsigned long` |
| 4 | `eltexto` tiene ~50 palabras | El enunciado pide 350 ± 15 | Redactar texto nuevo |
| 5 | `the_key = 2^54` | ~1.8·10^16 intentos, no medible | Llaves entre 2^20 y 2^30 para medir |
| 6 | Si nadie encuentra la llave, el proceso 0 se cuelga en `MPI_Wait` | Bloqueo | Manejo de "no encontrada" |
| 7 | `encrypt` antes de `MPI_Init`; `i < myupper`; `Irecv` pendiente al finalizar | Detalles menores | Documentar / ajustar |
| 8 | `encrypt` no recibe `len`, `decrypt` sí (y lo ignora) | Firmas inconsistentes con el enunciado | Unificar |
| 9 | `MPI_Test` en cada llave | Overhead | Sondear cada N iteraciones |

## Efecto de la llave en el reparto (para el punto 7)
La partición es por bloques contiguos: el tiempo depende de dónde cae la llave dentro de su bloque
y de qué proceso la contiene. Una llave baja la encuentra el proceso 0 casi de inmediato (speedup
aparente enorme); una llave en el último bloque tarda casi igual con 1 que con N procesos.
