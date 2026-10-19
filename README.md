# Proyecto 2 Parte 1: Fuerza bruta sobre DES con OpenMPI

Computación Paralela y Distribuida, Sección 20, Universidad del Valle de Guatemala.

Análisis de un programa que encuentra por fuerza bruta la llave privada con la que se cifró un texto con DES. La búsqueda se
reparte entre procesos MPI y se mide cómo la llave elegida afecta el tiempo y los speedups. Las instrucciones del proyecto están
en [proyecto_2_parte_1_openmpi.md](proyecto_2_parte_1_openmpi.md).

## Integrantes
- Iris Ayala, 23965
- Gabriel Bran, 23590
- Anggie Quezada, 23643

## Contenido del repositorio
| Ruta | Descripción |
|---|---|
| `bruteforce_.c` | Código base original (no se modifica) |
| `bruteforce_fix.c` | Versión corregida e instrumentada, con reparto por bloques o cíclico |
| `compat/rpc/des_crypt.h` | Reemplazo de `<rpc/des_crypt.h>` con OpenSSL, porque glibc ya no lo incluye |
| `data/texto.txt` | Texto de 347 palabras que se cifra (frase clave: `memoria distribuida`) |
| `scripts/` | `build.sh` (compilar), `smoke.sh` y `prueba_texto.sh` (pruebas rápidas), `medir.sh` (experimento), `analizar.py` (análisis y gráficas) |
| `results/` | Datos crudos (`mediciones.csv`, `procesos.csv`), resúmenes, gráficas y un ejemplo de ejecución |
| `docs/` | Documentación paso a paso y diagramas PlantUML (`docs/diagramas/`) |

## Requisitos
Probado en Ubuntu sobre WSL2 con:
- OpenMPI 5.0.10 (`mpicc`, `mpirun`)
- OpenSSL 3.5.5 con headers (`libssl-dev`)
- Python 3 con `pandas` y `matplotlib` (solo para `scripts/analizar.py`)

```bash
sudo apt install openmpi-bin libopenmpi-dev libssl-dev
```

No se ha probado en macOS ni en Windows nativo.

## Compilar
Desde la raíz del repositorio:
```bash
bash scripts/build.sh
```
Esto genera `build/bruteforce_orig` (código original) y `build/bruteforce_fix` (versión corregida). El flag `-Icompat` hace que el
código original compile sin modificarlo.

## Ejecutar
```bash
mpirun --oversubscribe -np <N> build/bruteforce_fix <llave> <archivo_texto> "<frase clave>" [block|cyclic] [bits]
```
- `llave`: entero en [0, 2^bits) con el que se cifra el texto.
- `block`: bloques contiguos, como el original. `cyclic`: el proceso `id` prueba `id`, `id+N`, `id+2N`, ...
- `bits`: tamaño del espacio de búsqueda, 2^bits. Por defecto 56 (el espacio real de DES).

Ejemplo:
```bash
mpirun --oversubscribe -np 4 build/bruteforce_fix 419430 data/texto.txt "memoria distribuida" cyclic 23
```

Salida:
- `PROC,<proceso>,<llaves probadas>,<segundos>`: una línea por proceso.
- `Key = <llave>` y el texto descifrado.
- `CSV,<N>,<modo>,<llave real>,<llave hallada>,<tiempo>`: el tiempo es el máximo entre procesos.

El código original `build/bruteforce_orig` usa una llave fija de 2^54 y nunca termina.

## Reproducir el experimento
```bash
bash scripts/medir.sh      # 135 mediciones (tarda más de una hora); es reanudable
python scripts/analizar.py # genera results/resumen.csv y las gráficas
```
Usa un espacio a escala de 2^23 llaves, 5 llaves (5%, 25%, 50%, 75% y 95% del espacio), N = 1, 2, 4, 8 y 16, los dos modos de reparto y
3 repeticiones por combinación. Conviene no usar el equipo para otras tareas pesadas mientras corre.

## Resultados principales
- Con bloques contiguos el tiempo depende de dónde cae la llave y casi no de N. Con el mismo N el speedup va de 1.1 a más de 6 800
  según la llave.
- Con reparto cíclico el speedup es consistente: unas 2.3 a 2.7 veces con 2 procesos, 4.7 a 5.7 con 4, 10 a 12 con 8 y 17 a 24 con 16.
- Varios speedups cíclicos superan N. No es superlinealidad real: la línea base N = 1 es más lenta y ruidosa.

El detalle, las tablas y el análisis están en [docs/06-mediciones.md](docs/06-mediciones.md).

## Documentación
1. [00-entorno.md](docs/00-entorno.md): entorno y shim de compatibilidad
2. [01-hallazgos-codigo.md](docs/01-hallazgos-codigo.md): problemas del código original
3. [02-version-corregida.md](docs/02-version-corregida.md): cambios en `bruteforce_fix.c`
4. [03-texto-y-llave.md](docs/03-texto-y-llave.md): texto, frase clave y llave privada (4206649)
5. [04-des.md](docs/04-des.md): descripción de DES
6. [05-funciones.md](docs/05-funciones.md): explicación de `encrypt`, `decrypt`, `tryKey`, `memcpy` y `strstr`
7. [06-mediciones.md](docs/06-mediciones.md): mediciones y análisis

Los diagramas están en `docs/diagramas/` como `.puml` (PlantUML) y `.png`.
