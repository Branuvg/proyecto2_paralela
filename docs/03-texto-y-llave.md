# Texto a cifrar (punto 4)

- Archivo: `data/texto.txt` (UTF-8, **347 palabras**, dentro de 350 ± 15 → 335 a 365).
- Tamaño: unos 2.5 KB, es decir ~315 bloques DES de 8 bytes (con padding de ceros al final).
- **Frase clave:** `memoria distribuida` (aparece una sola vez en el texto).
- Llave de prueba usada para validar: 2 000 000 → encontrada correctamente con `-np 4` en modo `cyclic`.

## Observación de rendimiento
Con el texto de 2.5 KB, cada intento descifra ~315 bloques, así que la velocidad bajó de ~0.7 M llaves/s
(texto corto de la prueba de humo) a ~58 000 llaves/s por proceso. Consecuencia para las mediciones:

| Llave | Tiempo aprox. con N=1 |
|---|---|
| 2^20 (1 048 576) | ~18 s |
| 2^22 (4 194 304) | ~72 s |
| 2^24 (16 777 216) | ~290 s |

Las llaves de medición deben mantenerse alrededor de 2^20 a 2^23 para poder repetir varias corridas.

## Texto
La programación paralela surge de una necesidad sencilla: resolver problemas grandes en menos tiempo. Durante décadas, los fabricantes lograron programas más rápidos aumentando la frecuencia de los procesadores, pero ese camino se agotó por límites de consumo de energía y de calor. La alternativa fue colocar varios núcleos en un mismo chip y conectar muchas máquinas entre sí, de modo que el trabajo se reparta entre todos ellos.
Existen dos grandes modelos para aprovechar ese hardware. En el modelo de memoria compartida, varios hilos de ejecución leen y escriben las mismas variables dentro de una sola computadora. Herramientas como OpenMP facilitan este estilo. En el modelo de memoria distribuida, en cambio, cada proceso tiene su propio espacio de direcciones y la única forma de colaborar es enviar y recibir mensajes. Este es el paradigma que estandariza MPI, una especificación que cuenta con implementaciones libres como OpenMPI.
Para medir si una solución paralela realmente vale la pena se utilizan dos métricas. El speedup compara el tiempo de la versión secuencial contra el de la versión con varios procesos, y la eficiencia divide ese valor entre el número de procesos usados. En el caso ideal, cuatro procesos resolverían el problema cuatro veces más rápido, pero en la práctica la comunicación, las partes que no se pueden dividir y el desbalance de carga reducen esa ganancia. La ley de Amdahl explica que la fracción secuencial de un programa impone un límite máximo al beneficio que se obtiene.
Un ejemplo claro de problema que se paraleliza bien es la búsqueda por fuerza bruta de una llave de cifrado. Cada candidato se puede probar de manera independiente, así que basta dividir el espacio de llaves entre los procesos. Sin embargo, el reparto importa mucho. Si se asignan bloques contiguos y la llave buscada está en el primer bloque, un solo proceso hará todo el trabajo útil mientras los demás esperan. Si, por el contrario, se reparten las llaves de forma intercalada, la carga se distribuye de manera más pareja y el tiempo resulta más consistente sin importar dónde se encuentre la respuesta.

## Llave privada elegida
`the_key = 4206649` (2^22 + 12345). Con N=1 tarda ~72 s en encontrarse.
