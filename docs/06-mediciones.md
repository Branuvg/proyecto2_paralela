# Mediciones y análisis (puntos 6 y 7)

## Metodología
- Programa: `bruteforce_fix.c`. Texto: `data/texto.txt` (347 palabras). Frase clave: `memoria distribuida`.
- **Espacio a escala:** 2^23 = 8 388 608 llaves (no 2^56). Con las llaves medibles, el reparto de 2^56/N siempre
  pondría la llave en el proceso 0. Con el espacio reducido, las posiciones relativas de la llave sí se reflejan.
- Llaves (posición en el espacio): 419430 (5%), 2097152 (25%), **4206649 (50.1%, llave elegida)**, 6291456 (75%), 7969177 (95%).
- N = 1, 2, 4, 8, 16; modos `block` (bloques contiguos, como el original) y `cyclic` (intercalado). 3 repeticiones por caso.
- Equipo: WSL2 Ubuntu en laptop de 16 hilos. Tiempo = máximo entre procesos (`MPI_Wtime`), hasta que todos terminan.
- Datos: `results/mediciones.csv`, `results/procesos.csv`. Resumen: `results/resumen.csv`. Gráficas: `results/speedup.png`, `results/tiempos.png`.
  Script: `scripts/analizar.py`. Speedup = T(N=1) / T(N); eficiencia = speedup / N.

## Tiempo promedio (s)
| Llave | Modo | N=1 | N=2 | N=4 | N=8 | N=16 |
|---|---|---|---|---|---|---|
| 419430 (5%) | block | 15.31 | 13.87 | 12.54 | 11.58 | 12.24 |
| | cyclic | 15.31 | 6.57 | 3.26 | 1.52 | 0.89 |
| 2097152 (25%) | block | 86.87 | 68.09 | 0.049 | 0.067 | 0.052 |
| | cyclic | 86.87 | 34.18 | 15.92 | 7.10 | 4.37 |
| 4206649 (50%) | block | 161.66 | 0.40 | 0.41 | 0.43 | 0.45 |
| | cyclic | 161.66 | 59.91 | 29.97 | 14.48 | 6.67 |
| 6291456 (75%) | block | 259.13 | 65.23 | 0.053 | 0.054 | 0.038 |
| | cyclic | 259.13 | 94.60 | 47.73 | 21.67 | 11.44 |
| 7969177 (95%) | block | 293.77 | 100.43 | 51.30 | 16.70 | 4.03 |
| | cyclic | 293.77 | 130.09 | 60.94 | 27.26 | 16.78 |

## Speedup
| Llave | Modo | N=2 | N=4 | N=8 | N=16 |
|---|---|---|---|---|---|
| 419430 (5%) | block | 1.10 | 1.22 | 1.32 | 1.25 |
| | cyclic | 2.33 | 4.69 | 10.06 | 17.31 |
| 2097152 (25%) | block | 1.28 | 1755 | 1305 | 1680 |
| | cyclic | 2.54 | 5.46 | 12.23 | 19.89 |
| 4206649 (50%) | block | 402 | 398 | 374 | 359 |
| | cyclic | 2.70 | 5.39 | 11.17 | 24.23 |
| 6291456 (75%) | block | 3.97 | 4865 | 4829 | 6822 |
| | cyclic | 2.74 | 5.43 | 11.96 | 22.65 |
| 7969177 (95%) | block | 2.93 | 5.73 | 17.59 | 72.93 |
| | cyclic | 2.26 | 4.82 | 10.78 | 17.51 |

Desviaciones estándar, coeficientes de variación y eficiencias están en `results/resumen.csv`.

## Cuántas llaves debe probar el proceso que gana
El tiempo lo fija el proceso que encuentra la llave: tiempo ≈ (llaves que prueba) / velocidad.
En `block` gana el proceso `llave / (2^23/N)` y prueba `llave - inicio_de_su_bloque + 1` llaves. En `cyclic` gana el proceso
`llave mod N` y prueba `llave / N + 1`.

| Llave | N | Proceso ganador (block) | Llaves que prueba (block) | Llaves que prueba (cyclic) |
|---|---|---|---|---|
| 5% | 2, 4, 8, 16 | 0 | 419 431 | 209 716 / 104 858 / 52 429 / 26 215 |
| 25% | 2 | 0 | 2 097 153 | 1 048 577 |
| 25% | 4 / 8 / 16 | 1 / 2 / 4 | 1 (la llave es la primera de su bloque) | 524 289 / 262 145 / 131 073 |
| 50% | 2 / 4 / 8 / 16 | 1 / 2 / 4 / 8 | 12 346 | 2 103 325 / 1 051 663 / 525 832 / 262 916 |
| 75% | 2 | 1 | 2 097 153 | 3 145 729 |
| 75% | 4 / 8 / 16 | 3 / 6 / 12 | 1 | 1 572 865 / 786 433 / 393 217 |
| 95% | 2 / 4 / 8 / 16 | 1 / 3 / 7 / 15 | 3 774 874 / 1 677 722 / 629 146 / 104 858 | 3 984 589 / 1 992 295 / 996 148 / 498 074 |

Las llaves 25%, 50% y 75% son casos especiales: 2097152 y 6291456 son múltiplos de 2^21, por lo que coinciden exactamente con el
inicio de un bloque cuando N ≥ 4. Eso explica los speedups de miles en `block`. La llave 4206649 está a 12 345 llaves del inicio
del bloque en el que cae (2^22 + 12345).

## Observaciones
1. **Con `block`, el tiempo depende de dónde cae la llave, no de N.**
   - Llave 5%: cae en el bloque del proceso 0 para todo N. Solo ese proceso trabaja y el speedup es ~1.1–1.3 aunque haya 16 procesos.
   - Llaves 25%, 50%, 75%: en cuanto el bloque de algún proceso empieza cerca de la llave, el tiempo cae a décimas de segundo
     y el speedup parece de cientos o miles. No es paralelismo real: es suerte de posición. Un proceso empieza casi sobre la llave.
   - Llave 95%: cae en el último bloque. Con N=16 comienza a probar 104 858 llaves antes de acertar, y el speedup es 73.
2. **Con `cyclic`, el speedup es consistente** (≈2.3–2.7 con N=2, 4.7–5.7 con N=4, 10–12 con N=8, 17–24 con N=16) sin importar la llave, porque
   todos los procesos avanzan sobre la misma región del espacio.
3. **Speedup y eficiencia > ideal:** varios speedups de `cyclic` superan N (por ejemplo 24.2 con N=16, eficiencia 1.51).
   - La velocidad por proceso no es constante: la mediana es ~24 300 llaves/s con N=1 y 30 000–35 500 con N=2–16.
   - El baseline N=1 es más lento y ruidoso (CV hasta 23%), lo que infla los speedups. No es superlinealidad real.
   - Las causas probables son el escalado de frecuencia de la CPU y la carga de fondo en el equipo.
4. **Variabilidad:** las corridas largas (N=1 y llaves altas) tienen CV de 7% a 15%; las cortas, hasta 30%.

## Para el punto 7: por qué una llave es más rápida o más lenta de encontrar
- El tiempo es el de la llave que prueba más candidatos antes de acertar, dividido entre la velocidad de un proceso (~30 000 llaves/s aquí).
- En un reparto por bloques, lo que importa es la **distancia entre la llave y el inicio del bloque que la contiene**:
  - Llave justo al inicio de un bloque: se halla casi de inmediato (mejor caso).
  - Llave al final de un bloque: el proceso debe recorrer el bloque completo (peor caso, ~2^56/N llaves).
  - Llave en el bloque 0 o muy cerca del inicio del espacio: la encuentra el proceso 0 rápido sin importar cuántos procesos haya.
  - Llave en el último bloque, lejos de su inicio: casi no hay mejora respecto al secuencial.
- Por eso un speedup medido con una sola llave no es representativo; con varias llaves el rango observado en este experimento
  va de 1.1 a 6 800 para el mismo N.
- El reparto cíclico elimina esa dependencia de la posición. Todos los procesos recorren juntos el espacio desde el inicio; la llave
  se encuentra cuando el avance conjunto la alcanza. El tiempo queda ≈ (llave / N) / velocidad.
- Otros factores que afectan el tiempo: tamaño del texto (más bloques a descifrar por llave), velocidad por núcleo, número de núcleos
  físicos frente a hilos lógicos, y el costo de `MPI_Test` (aquí cada 1024 llaves).
