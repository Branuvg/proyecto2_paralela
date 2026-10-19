# Entorno de trabajo

## Verificación (2026-10-16)
- WSL2 Ubuntu, glibc 2.43, 16 hilos disponibles (`nproc`).
- OpenMPI 5.0.10 (`mpicc`, `mpirun`), gcc, OpenSSL 3.5.5 (`libssl-dev`).
- `mpirun -np 2 hostname` funciona.
- **Falta `rpc/des_crypt.h`**: glibc lo eliminó, por lo que `bruteforce_.c` no compila tal cual.

## Solución: shim de compatibilidad
`compat/rpc/des_crypt.h` implementa `ecb_crypt` y `des_setparity` con la API DES de OpenSSL
(mismo DES estándar, mismos valores `DES_ENCRYPT=0`/`DES_DECRYPT=1` de glibc). Así `bruteforce_.c`
queda sin modificar.

## Compilar y ejecutar (desde WSL, en la raíz del repo)
```bash
mpicc -Icompat -Wno-deprecated-declarations -o build/bruteforce_orig bruteforce_.c -lcrypto
mpirun --oversubscribe -np 4 build/bruteforce_orig
```

## Resultado de la ejecución del código original
- Compila sin errores.
- Con `-np 4` cada proceso imprime su rango (`Process i lower .. upper ..`), con lo que se confirma
  la partición en bloques contiguos de 2^56 / N.
- No termina por sí solo: `the_key = 2^54` es inalcanzable (~1.8·10^16 llaves) y además la frase
  de búsqueda nunca coincide (ver `01-hallazgos-codigo.md`).

## Convenciones del repo
- `docs/*.md`: documentación paso a paso (espejo del Word).
- `docs/diagramas/*.puml`: diagramas PlantUML (se renderizan en un editor en línea a PNG).
- El Word final (`*.docx`) está en `.gitignore`.
