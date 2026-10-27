# Proyecto 2 Parte 1 Programación Paralela con OpenMPI

MPI es el estándar para el modelo paralelo de memoria distribuida. Debido a su interoperabilidad y portabilidad, MPI nos permite interconectar máquinas para crear conjuntos computacionales homogéneos y heterogéneos. El paradigma de intercambio de mensajes nos permite interconectar máquinas en área local sin depender de la topología de la red de interconexión. Esto hace que MPI sea muy escalable.

En equipos de 3 integrantes (los mismos del proyecto 1), deben utilizar MPI para mejorar un programa que encuentra la llave privada con la que fué cifrado un texto plano. La búsqueda se hará probando todas las posibles combinaciones de llaves, hasta encontrar una que descifra el texto. Se sabrá si logro descifrar el texto correctamente validando si el mismo contiene como substring una palabra/frase clave de búsqueda, la cual se indica explicitamente

(por ejemplo, si el texto cifrado fuera este párrafo, ‘los mismos del proyecto 1’ podría ser una buena frase clave para saber si se decifró correctamente).

Además del cifrado, descifrado y paralelización con OpenMPI, el equipo debe realizar un análisis de speedups y performance de procesos, para entender la distribución del espacio de búsqueda de las llaves. Deberá ponerse especial atención al impacto que tiene la llave que elijan en el performance y speedups aparentes del algoritmo, así como la consistencia del mismo con distintas llaves.

Trabajando con el código [bruteforce\_.c](./bruteforce_.c), realizar las siguientes tareas:

1. Investigar sobre [DES](https://en.wikipedia.org/wiki/Data_Encryption_Standard) y describir los pasos requeridos para cifrar/descifrar un texto para comprender lo que realiza el algoritmo.
2. Dibujar un diagrama de flujo describiendo el algoritmo DES
3. Compilar y ejecutar el programa bruteforce.c
4. Cifrar un texto de 350 +/-15 palabras máximo que elija el grupo usando una llave privada
5. Explicar DETALLADAMENTE mediante diagramas como funcionan las funciones junto con el programa dentro de bruteforce\_.c:
   1. decrypt (key, \*ciph, len) y encrypt (key, \*ciph, len)
   2. tryKey (key, \*ciph, len)
   3. memcpy
   4. strstr
6. Medir el tiempo de búsqueda de la llave privada mediante bruteforce, recuerden hacer varias mediciones y promediarlas para obtener un tiempo más adecuado utilizando diferentes llaves de encripcion.
7. Argumentar que ocurre al eleguir diferentes llaves de encripcion al momento de distribuir la busqueda. Que hace que sea mas rapido o mas lento encontrarla?

Subir documento pdf con lo especificado.