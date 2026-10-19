# DES (Data Encryption Standard) - puntos 1 y 2

## Qué es
Cifrado simétrico por bloques (misma llave para cifrar y descifrar), estandarizado en 1977.
- Bloque: **64 bits** (8 bytes). Llave: 64 bits, de los cuales **56 son útiles** y 8 son bits de paridad
  (uno por byte, paridad impar). Por eso el espacio de llaves es 2^56.
- Estructura: **red de Feistel de 16 rondas**, con una permutación inicial (IP) y su inversa (FP).
- Un texto largo se divide en bloques de 8 bytes y cada uno se cifra por separado (**modo ECB**, el que usa
  `ecb_crypt`). Si el último bloque queda incompleto, se rellena (en este proyecto, con ceros).

## Pasos para cifrar un bloque
1. **Generar 16 subllaves de 48 bits** a partir de la llave:
   1. **PC-1**: de 64 bits se eliminan los 8 de paridad y se permuta, quedando 56 bits.
   2. Se dividen en dos mitades de 28 bits: C0 y D0.
   3. En cada ronda i, C y D se rotan a la izquierda 1 bit (rondas 1, 2, 9, 16) o 2 bits (las demás).
   4. **PC-2**: se juntan C y D (56 bits) y se eligen/permutan 48 bits: subllave Ki.
2. **Permutación inicial (IP)** del bloque de 64 bits. Se parte en L0 (32 bits) y R0 (32 bits).
3. **16 rondas**, para i = 1..16:
   - L_i = R_(i-1)
   - R_i = L_(i-1) XOR f(R_(i-1), K_i)
4. **Intercambio final**: se forma R16 L16 (se invierten las mitades).
5. **Permutación final (FP = IP^-1)** y se obtiene el bloque cifrado de 64 bits.

### Función f(R, K)
1. **Expansión E**: R (32 bits) se expande a 48 bits duplicando algunos bits.
2. **XOR** con la subllave Ki de 48 bits.
3. **S-boxes**: los 48 bits se dividen en 8 grupos de 6 bits. Cada grupo entra a una S-box (S1..S8) y sale
   un valor de 4 bits (el primer y último bit eligen la fila, los 4 centrales la columna). Salen 32 bits.
   Es la parte no lineal y la que da la seguridad.
4. **Permutación P** de los 32 bits.

## Descifrado
Es **el mismo algoritmo** con las subllaves en orden inverso (K16, K15, ..., K1). Esto funciona porque
la estructura de Feistel es invertible sin importar qué haga f.

## Relevancia para el proyecto
- La llave de 56 bits se pasa a `ecb_crypt` expandida a 8 bytes (7 bits útiles por byte + paridad).
- Probar una llave = generar sus 16 subllaves y descifrar todos los bloques. Cada bloque cuesta 16 rondas,
  por eso un texto de ~315 bloques hace cada intento ~12 veces más lento que uno de 13 bloques.
- Con 56 bits hay 2^56 ≈ 7.2·10^16 llaves, y por eso el bruteforce tarda tanto.

## Diagramas (punto 2)
Fuente PlantUML en `docs/diagramas/`:
- `des_flujo.puml`: flujo general de cifrado y descifrado de DES.
- `des_funcion_f.puml`: detalle de la función f.
Se renderizan en un editor en línea (por ejemplo plantuml.com/plantuml) y se exportan a PNG.
