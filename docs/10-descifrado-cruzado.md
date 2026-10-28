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
aparece, así que el grupo cifró con el armado **corregido** (igual que la mayoría, según también reportó
un grupo compañero que pasó por el mismo problema con otro texto).

**Estado de la búsqueda:** en progreso al momento de escribir esto. Se cubrieron sin éxito las primeras
~300 millones de llaves del espacio completo (2^56) en reparto cíclico con 16 procesos locales — muy por
encima de las llaves "amigables" de otros grupos conocidos (3 141 592, 4 206 649, 419 430), lo que sugiere
una llave más grande o varios dígitos más. *(Actualizar esta sección con la llave y el texto recuperado en
cuanto termine la búsqueda; si no termina a tiempo para la entrega, documentar el intento igual — ver
sección de discusión, pregunta 6.)*

## Validación del pipeline local → clúster (con llave propia conocida)

Antes de tener la llave del grupo cruzado, se validó el camino completo (Ts local → Tp en el clúster con
Tailscale, ver `docs/09-cluster-tailscale.md`) usando el propio texto y llave de la Parte 1
(`data/texto.txt`, llave `4206649`, frase `"memoria distribuida"`, espacio reducido a 2^23 para que la
medición sea repetible, igual que en `docs/06-mediciones.md`). Esto no sustituye el requisito de la Sección
III (que pide el texto de *otro* equipo), pero confirma que el resto de la metodología funciona:

| Corrida | Procesos | Nodos | Tiempo | Speedup | Eficiencia |
|---|---|---|---|---|---|
| Local (Ts) | 4 | 1 (maestro, vía WSL2/motor nativo) | 18.183062 s | 1.00 | 1.00 |
| Distribuida (Tp) | 8 | 2 (maestro + 1 compañero, Tailscale) | 10.938858 s | 1.66 | 0.21 |

Nota de medición: la primera corrida de Ts dio 52.28 s por una contaminación de CPU (la búsqueda del grupo
cruzado corría de fondo saturando los 16 núcleos de la máquina). Se pausó esa búsqueda (`kill -STOP`) antes
de medir Ts limpio y se reanudó después (`kill -CONT`) — importante dejarlo anotado porque es exactamente
el tipo de factor externo que puede arruinar una medición de tiempo sin que el código tenga ningún error.

Con reparto cíclico y 2^23 llaves de espacio, la eficiencia de 0.21 con 8 procesos es baja comparada con los
speedups cíclicos de la Parte 1 local (2.3-2.7x con 2 procesos, hasta 17-24x con 16, ver `docs/06-mediciones.md`).
La diferencia es la latencia de red entre nodos (Tailscale, posiblemente con relay DERP en vez de conexión
P2P directa) y el arranque de los procesos remotos por SSH, que no existen en una corrida de un solo nodo.

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

Con la llave propia (sección anterior, para validar el pipeline): Ts = 18.18 s (4 procesos, 1 nodo) vs.
Tp = 10.94 s (8 procesos, 2 nodos), speedup 1.66x. *(Agregar aquí la comparación con la llave real del
grupo cruzado en cuanto se tenga.)*

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

*(Completar con la reflexión final del equipo una vez cerrado el ejercicio, incluyendo si se alcanzó a
recuperar la llave del grupo cruzado a tiempo o no, y qué se haría distinto la próxima vez — por ejemplo,
pedir una pista del orden de magnitud de la llave antes de lanzar una búsqueda a ciegas contra todo 2^56.)*
