# Explicación de las funciones de bruteforce_.c (punto 5)

Código de referencia: `bruteforce_.c` (original). Las correcciones están en `01-hallazgos-codigo.md` y `02-version-corregida.md`.

## 5.1 encrypt(key, *ciph) y decrypt(key, *ciph, len)
Ambas hacen lo mismo salvo el último argumento de `ecb_crypt` (`DES_ENCRYPT` o `DES_DECRYPT`).

### Paso 1: expandir la llave de 56 bits a 8 bytes
```c
long k = 0;
for (int i = 0; i < 8; ++i) {
  key <<= 1;
  k += (key & (0xFE << i*8));
}
```
- DES espera 8 bytes (64 bits), pero la llave a probar tiene solo 56 bits. Hay que repartir sus bits en 8 bytes de 7 bits.
- En la iteración `i`, `key` se ha desplazado `i+1` bits. La máscara `0xFE << i*8` deja pasar los bits 1 a 7 del byte `i`
  (el bit 0 se reserva para la paridad). Equivale a tomar los bits `7i ... 7i+6` de la llave original y ponerlos en el byte `i`.
- Ejemplo con `key = 1`: tras el bucle `k` = `02 00 00 00 00 00 00 00` (byte 0 = 0x02).
- **Bug:** `0xFE` es `int` de 32 bits. Para `i >= 4` el desplazamiento es indefinido; la versión corregida usa `0xFEUL`.

### Paso 2: bits de paridad
`des_setparity((char*)&k)` ajusta el bit 0 de cada byte para que el byte tenga cantidad impar de unos.
Siguiendo el ejemplo: `02 00 00 ...` pasa a `02 01 01 01 01 01 01 01`.

### Paso 3: cifrar o descifrar en sitio
`ecb_crypt(llave, buffer, 16, modo)` aplica DES en modo ECB sobre el buffer, **modificándolo directamente**
(no hay buffer de salida). El tercer argumento es el largo en bytes y debe ser múltiplo de 8.
- **Bug:** el original pasa siempre `16`, así que solo procesa los primeros 2 bloques e ignora `len`.
  La versión corregida pasa el largo real (múltiplo de 8).

Diagrama: `diagramas/expansion_llave.puml`.

## 5.2 tryKey(key, *ciph, len)
Prueba si una llave candidata descifra el texto.
1. Crea `temp[len+1]` en la pila.
2. `memcpy(temp, ciph, len)`: copia el cifrado, porque `decrypt` modifica el buffer. Sin la copia, el primer intento
   destruiría `cipher` y los siguientes descifrarían basura.
3. `temp[len] = 0`: terminador de cadena, requerido por `strstr`.
4. `decrypt(key, temp, len)`: descifra la copia con la llave candidata.
5. `strstr(temp, search) != NULL`: devuelve 1 si el resultado contiene la frase clave, 0 si no.

Con una llave incorrecta `temp` queda con bytes aleatorios; con la correcta queda el texto original.
La cadena `search` es global y viene del programa.

Diagramas: `diagramas/trykey_secuencia.puml` y `diagramas/trykey_memoria.puml`.

## 5.3 memcpy(dest, src, n)
Copia `n` bytes desde `src` hasta `dest`. No mira el contenido, no se detiene en `\0` y requiere que las regiones
**no se traslapen**. En el programa se usa en dos lugares:
- `main`: copia `eltexto` a `cipher` antes de cifrar (para no modificar el literal).
- `tryKey`: copia `cipher` a `temp` en **cada una** de las llaves probadas (el costo es pequeño frente a DES).

## 5.4 strstr(haystack, needle)
Busca la primera aparición de la subcadena `needle` dentro de `haystack`. Devuelve un puntero a esa posición o `NULL`.
- Ambas cadenas deben terminar en `\0`; la búsqueda se detiene en el primer `\0` del texto.
- Complejidad de la implementación simple: O(n·m); para este uso la comparación casi siempre falla en el primer carácter,
  así que el costo real es cercano a O(n).
- Con llaves incorrectas el texto es basura y puede contener un `\0` temprano que corta la búsqueda; con la llave
  correcta no hay `\0` hasta el padding final.
- Por eso el original nunca terminaba: buscaba `" los mismos del proyecto 1 "` (con espacios) y el texto decía
  `(los mismos del proyecto 1)`.

Diagrama: `diagramas/strstr.puml`.

## 5.5 El programa completo (main)
1. Cifra `eltexto` con `the_key`.
2. `MPI_Init`, `MPI_Comm_size` (N) y `MPI_Comm_rank` (id).
3. Divide el espacio [0, 2^56) en N bloques contiguos; el proceso `id` prueba `[id·R, (id+1)·R)`, con `R = 2^56 / N`.
4. Deja un `MPI_Irecv` abierto para enterarse si otro proceso encontró la llave.
5. Bucle: `MPI_Test` (¿ya avisaron?) y `tryKey(i)`. Si acierta, envía la llave a todos con `MPI_Send` y sale.
6. El proceso 0 hace `MPI_Wait`, descifra e imprime la llave y el texto.

Diagrama: `diagramas/main_mpi.puml`.
