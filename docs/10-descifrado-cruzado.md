# 10 — Descifrado cruzado y análisis (Parte 3)

Sección III (validación local) y Sección VI (discusión) del enunciado `ins/Paralela_Proyecto_2_P2.md`.

## Adaptación del programa

`bruteforce_fix.c` originalmente cifraba el texto él mismo con la llave recibida por argumento y luego la
buscaba (ver `docs/07-bitacora-parte2.md`, hallazgo pendiente de la Etapa 1). Se agregó un modo nuevo para
leer el ciphertext de otro equipo sin modificar el comportamiento existente (usado en todas las mediciones
de la Parte 1):

```
mpirun -np N bruteforce_fix --b64 <archivo.b64> "<frase clave>" [block|cyclic] [bits]
```

- `read_b64_file`: decodifica el archivo base64 (una sola línea, como lo publican los equipos) a binario,
  usando un acumulador `unsigned int` para evitar desbordamiento, y recorta al múltiplo de 8 completo que
  pide `ecb_crypt`.
- El resto del programa (reparto `block`/`cyclic`, `MPI_Test` cada `TEST_EVERY`, `MPI_Allreduce`/`MPI_Reduce`
  al final) no cambia.
- Validado con un ciphertext de prueba propio (`cifrar.c`, llave 777): el modo `--b64` encontró la llave y
  recuperó el texto exacto.

## Grupo elegido para el descifrado cruzado

| Grupo | Palabra objetivo | Archivo |
|---|---|---|
| 3223232343223423423423 - minecraft biomas | `Open MPI es una` | `cruzado/grupo_minecraft/texto_cifrado.b64` (2136 bytes, 267 bloques) |

**Descartado:** el armado de llave *original* (sin corregir, `0xFE` con `key` tipo `int`/`long`, el mismo
bug documentado en `docs/01-hallazgos-codigo.md`). Se probó explícitamente en `[0, 16 000 000)` y no
aparece.

**Llave encontrada: `18014398509481984` (2^54).** No se llegó por fuerza bruta a ciegas — resultó ser el
mismo equipo autor del repo de referencia usado para comparar metodologías (Kevin Villagrán/Joel Jaquez),
quienes confirmaron la llave directamente al preguntarles. Se verificó de dos formas independientes antes
de darla por buena:
1. **Verificación propia:** `mpirun -np 2 bruteforce --b64 texto_cifrado.b64 "Open MPI es una" block 55`
   decodifica el texto completo y legible sobre Open MPI con esa llave exacta.
2. **Verificación criptográfica cruzada:** se recompiló su propio `cifrar.c` y se cifró su propio texto con
   esa llave; el resultado coincide byte a byte en 266 de 267 bloques DES-ECB con el ciphertext que
   publicaron (el único bloque distinto es una diferencia de fin de línea en la copia local del texto, no
   de la llave) — una coincidencia así en bloques ECB independientes es estadísticamente imposible con una
   llave incorrecta.

La búsqueda a ciegas por fuerza bruta (reparto cíclico, 16 procesos locales) alcanzó a cubrir sin éxito
las primeras ~300 millones de llaves del espacio completo (2^56) antes de obtener la confirmación directa;
2^54 ≈ 1.8×10^16 está muy por encima de lo que una búsqueda a ciegas habría cubierto a tiempo para la
entrega, lo que confirma que preguntar el orden de magnitud (pregunta 6) hubiera ahorrado horas de cómputo.

## Comparación Ts vs. Tp (grupo cruzado, clúster real con Tailscale)

Con la llave real (`18014398509481984`) y el ciphertext de Kevin/Joel, reparto `block`, espacio completo
(2^56 bits, el real de DES, sin escalar):

| Corrida | Procesos | Nodos | Tiempo | Speedup | Eficiencia |
|---|---|---|---|---|---|
| Local (Ts) | 4 | 1 (maestro) | 0.017601 s | 1.00 | 1.00 |
| Distribuida (Tp) | 8 | 2 (maestro + compañera, Tailscale, relay DERP Miami) | 1.460849 s | 0.012 | 0.0015 |

**El clúster fue ~83 veces más lento que local, no más rápido.** La razón es la misma que ya anticipó el
equipo de referencia con su propio hallazgo: con reparto en bloques, 2^54 cae justo al inicio del rango de
un proceso tanto con 4 como con 8 procesos (`PROC,1,0,...` en Ts y `PROC,2,0,...` en Tp — ambos con
`tried=0`, la encontraron en el primer intento). El tiempo de cómputo real es despreciable en los dos
casos; lo que domina en la corrida distribuida es el **arranque de `orted` remoto por SSH** y la
**sincronización en `MPI_Barrier`/el aviso `MPI_Send` entre nodos a través del relay DERP de Tailscale**
(~200-300 ms de ida y vuelta medidos con `tailscale ping`, nunca hubo conexión P2P directa). Cuando casi
todo el "trabajo" es overhead de arranque y mensajería, agregar un segundo nodo solo suma más overhead sin
nada que paralelizar.

## Validación adicional del pipeline (con llave propia conocida, espacio reducido)

Antes de tener la llave del grupo cruzado, se validó el mismo camino completo (Ts local → Tp en el clúster
con Tailscale) usando el propio texto y llave de la Parte 1 (`data/texto.txt`, llave `4206649`, frase
`"memoria distribuida"`, espacio reducido a 2^23 para que la medición sea repetible, igual que en
`docs/06-mediciones.md`). A diferencia de la comparación de arriba, aquí la llave sí cae en medio del
espacio de búsqueda real (no al inicio de ningún rango), así que el resultado muestra un caso donde
distribuir sí ayuda:

| Corrida | Procesos | Nodos | Tiempo | Speedup | Eficiencia |
|---|---|---|---|---|---|
| Local (Ts) | 4 | 1 (maestro, vía WSL2/motor nativo) | 18.183062 s | 1.00 | 1.00 |
| Distribuida (Tp) | 8 | 2 (maestro + 1 compañero, Tailscale) | 10.938858 s | 1.66 | 0.21 |

Nota de medición: la primera corrida de Ts dio 52.28 s por una contaminación de CPU (otra búsqueda corría
de fondo saturando los 16 núcleos de la máquina). Se pausó esa búsqueda (`kill -STOP`) antes de medir Ts
limpio y se reanudó después (`kill -CONT`) — importante dejarlo anotado porque es exactamente el tipo de
factor externo que puede arruinar una medición de tiempo sin que el código tenga ningún error.

Comparando ambas tablas: la eficiencia de 0.21 (llave en medio del espacio) contra 0.0015 (llave al borde
de un rango) muestra lo mismo que ya se documentó en `docs/06-mediciones.md` para la Parte 1 — el resultado
de un clúster distribuido depende enormemente de *dónde* cae la llave, no solo de cuántos procesos/nodos se
usen.

## Discusión (Sección VI del enunciado)

### 1. ¿Por qué OpenMPI se congela indefinidamente sin `--mca btl_tcp_if_include`?

Cuando MPI arranca, cada proceso anuncia las direcciones IP de todas sus interfaces de red para que los
demás nodos sepan cómo contactarlo (canal "out-of-band", OOB, y luego el canal de datos BTL/TCP). Si no se
limita la búsqueda a una subred, OpenMPI incluye interfaces virtuales como `docker0`, `veth*` o adaptadores
de VPN/VirtualBox. Esas interfaces existen en todas las máquinas pero casi nunca están en la misma subred
entre una máquina y otra (p. ej. `docker0` suele ser `172.17.0.1/16` en cualquier instalación de Docker,
pero no son la misma red física). El proceso remoto intenta conectarse por esa IP, el paquete nunca llega
(porque no hay ruta real o cae en una NAT distinta) y como TCP reintenta en vez de fallar inmediatamente,
el programa se queda esperando una respuesta que nunca llega — sin mensaje de error, porque desde el punto
de vista de MPI sigue "intentando conectar". `--mca btl_tcp_if_include`/`oob_tcp_if_include` fuerzan a MPI a
anunciar y usar solo una interfaz conocida y alcanzable entre todos los nodos (la subred del router, o en
nuestro caso `tailscale0`), eliminando la ambigüedad.

### 2. Problemas de red enfrentados

- **Docker Desktop en Windows no comparte la interfaz física con el contenedor.** Con `--network host` y
  "Enable host networking" activo, el contenedor seguía viendo solo IPs internas de la VM de Docker
  (`192.168.65.x`/`172.x`), nunca la IP real del Wi-Fi. Se confirmó que es una limitación de diseño de
  Docker Desktop (no comparte namespace de red real como en Linux), no un error de configuración —
  verificado con un motor Docker nativo dentro de WSL2, donde sí funcionó.
- **La IP física cambió a media sesión** al cambiar de red (de hotspot a la red de la universidad y
  viceversa), lo que invalidaba cualquier hostfile ya armado.
- **Firewall institucional interceptando TLS (Fortinet).** Al construir la imagen con la capa de Tailscale
  conectados a la red de la universidad, la descarga del instalador fallaba con un error de certificado
  SSL no porque Tailscale estuviera mal, sino porque el firewall de la universidad hace inspección TLS
  (man-in-the-middle) sobre el tráfico HTTPS saliente y presenta su propio certificado. Se resolvió
  construyendo la imagen desde otra red (datos móviles) y volviendo a la red de la universidad después.
- **Se adoptó Tailscale** (VPN mesh, interfaz `tailscale0` con IP `100.x` igual en todas las máquinas) en
  vez de seguir peleando con la red host de Docker, evitando los tres problemas anteriores a la vez.

### 3. Tiempo local vs. clúster físico

Con la llave real del grupo cruzado (2^54, cae al borde de un rango): Ts = 0.0176 s (1 nodo) vs.
Tp = 1.4608 s (2 nodos vía Tailscale) — el clúster fue ~83x **más lento**, porque el cómputo real es
despreciable y domina el overhead de arrancar el proceso remoto por SSH y sincronizar a través del relay
DERP. Con la llave propia de la Parte 1 (en medio del espacio, no al borde): Ts = 18.18 s vs. Tp = 10.94 s,
speedup real de 1.66x. La diferencia entre ambos casos es el mejor ejemplo concreto de que el speedup de un
clúster depende de dónde cae el trabajo útil, no solo de la infraestructura.

### 4. ¿Qué pasa con 32 procesos MPI en una máquina de 4 núcleos físicos?

Es oversubscription severa: el sistema operativo tiene que repartir 32 procesos activos entre solo 4
núcleos lógicos mediante cambios de contexto constantes. Cada cambio de contexto tiene un costo fijo
(guardar/restaurar registros, invalidar cachés L1/L2 del núcleo), y con DES, que es una carga intensiva en
CPU sin esperas de I/O, ese costo no se esconde detrás de ningún tiempo muerto. El resultado no es un
error, sino un rendimiento mucho peor que con 4 procesos: el tiempo total aumenta porque la mayor parte se
gasta cambiando entre procesos en vez de calculando, y además compiten entre sí por las mismas cachés,
degradando también el rendimiento individual de cada uno. Es la misma razón por la que `cluster_hosts` y
`run_cluster.sh` insisten en no pasar `max_slots` del número real de núcleos físicos.

### 5. Vulnerabilidades y retos logísticos de HPC con hardware heterogéneo

- **Superficie de ataque de SSH sin restricciones.** Para que MPI funcione entre nodos se necesita
  `PermitRootLogin yes` y acceso sin contraseña — exactamente lo que cualquier guía de seguridad recomienda
  evitar. Es aceptable en un contenedor efímero de laboratorio, pero sería inaceptable en producción sin
  aislar esas llaves y esa configuración a una red privada y confiable.
- **Confianza implícita entre nodos.** Una vez que el maestro puede ejecutar comandos sin contraseña en los
  workers (y viceversa, si se configura así), cualquier nodo comprometido compromete a todo el clúster.
- **Heterogeneidad real de hardware y SO.** Windows, Mac y Linux se comportan de forma distinta frente a
  redes virtuales, firewalls y virtualización de contenedores — lo que funciona en una máquina no garantiza
  nada en otra, como se vio con Docker Desktop. Un administrador de HPC real tiene que anticipar y
  documentar estas diferencias, no asumir homogeneidad.
- **Dependencia de servicios de terceros.** Usar una VPN mesh como Tailscale resuelve el problema de red,
  pero añade una dependencia externa (su propio servicio de control y, a veces, sus relays DERP) que no
  existe si todos los nodos están en la misma LAN real.
- **Coordinación logística entre personas.** Sincronizar llaves SSH, IPs y versiones de la imagen entre 2-3
  personas en tiempo real, cada una con su propia conexión y sistema, fue en la práctica más difícil que el
  código en sí — y a escala real (decenas o cientos de nodos) ese problema solo crece.

### 6. ¿Se logró el objetivo? ¿Qué fue lo más difícil?

Sí se logró: se adaptó el programa para leer el ciphertext de otro equipo, se confirmó la llave real
(`18014398509481984`) con doble verificación (confirmación directa del otro equipo + prueba criptográfica
independiente), y se corrió la comparación Ts/Tp en un clúster físico real con dos máquinas distintas.

Lo más difícil no fue el código, fue la infraestructura de red: Docker Desktop en Windows resultó no
compartir la interfaz física con el contenedor (un límite de diseño, no un error de configuración),
el firewall de la universidad interceptaba TLS y bloqueaba el tráfico de Tailscale, la IP física cambió
varias veces de sesión a sesión, y la sincronización manual de la llave SSH pública se corrompió repetidas
veces al pasarla por una app de chat que insertaba espacios al reflowear el texto largo — se resolvió
hasta que se dejó de pegar la llave a mano y se transmitió como archivo versionado en el repo.

Lo que se haría distinto: pedir desde el principio el orden de magnitud de la llave del equipo cruzado
(o al menos confirmar si usan el armado de llave original o corregido) antes de lanzar una búsqueda a
ciegas sobre 2^56 — se gastaron varios minutos de cómputo en balde que se habrían evitado con una pregunta
directa. También se aprendería a transmitir cualquier dato binario/base64 largo entre compañeros como
archivo (git, o adjunto) desde el principio, nunca pegado a mano en un chat.
