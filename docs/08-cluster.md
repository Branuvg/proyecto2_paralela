# Etapa 2: clúster físico (hostfile, llaves SSH, MCA y ataque distribuido)

Procedimiento y scripts para la sección V del enunciado. Los scripts están en `scripts/cluster/` y se copian dentro de cada
contenedor (el `.dockerignore` deja `scripts/` fuera de la imagen).

## Qué se ensayó y qué no
- **Ensayado (2026-10-22):** dos contenedores en una red Docker `172.30.0.0/24` en una sola máquina: Paso A, hostfile,
  `--mca btl_tcp_if_include/oob_tcp_if_include` y `mpirun -np 8` repartido en 2 nodos, con la llave encontrada.
- **No ensayado:** la LAN física con el router y las computadoras de los compañeros. Ahí queda por verificar el firewall, el aislamiento
  del router y `--network host` (ver "Riesgos").

## Archivos
| Archivo | Uso |
|---|---|
| `cluster_hosts.example` | Plantilla del hostfile; se copia a `/app/cluster_hosts` en el maestro |
| `scripts/cluster/nodo_start.sh` | En cada nodo: arranca `sshd` y muestra hostname, IPs, núcleos y el binario |
| `scripts/cluster/ssh_keys.sh` | Paso A: `pub` (maestro), `add "<pub>"` (worker), `test <ip>` (maestro) |
| `scripts/cluster/run_cluster.sh` | Paso B: arma el `mpirun` con hostfile, subred MCA y mapa de procesos |

## Procedimiento
**1. En cada máquina:** construir la imagen (`docker build -t mpi-cluster-image .`) y levantar el contenedor con un nombre de host
único, sin guion bajo:
```bash
docker run -it --network host --hostname nodo-maestro --name mpi_node mpi-cluster-image /bin/bash
```
Desde otra terminal del host, copiar los scripts: `docker cp scripts/cluster mpi_node:/app/cluster_scripts`. Dentro del contenedor:
```bash
bash /app/cluster_scripts/nodo_start.sh
```
Anotar la IP del router de cada nodo (`hostname -I` o `ipconfig` en Windows) y los núcleos reales.

**2. Paso A, llaves SSH.** Cada imagen trae su propio par de llaves (se genera al construir), así que hay que autorizar la del maestro en los workers.
```bash
# Nodo Maestro
bash /app/cluster_scripts/ssh_keys.sh pub          # copiar la línea completa que imprime
# Cada Worker (pegar la llave entre comillas)
bash /app/cluster_scripts/ssh_keys.sh add "ssh-rsa AAAA... root@..."
# Nodo Maestro, una vez por worker
bash /app/cluster_scripts/ssh_keys.sh test 192.168.1.11
```
Sin este paso la prueba da `Permission denied (publickey,password)`; con él imprime `SSH sin contrasena OK en <host>`.

**3. Hostfile en el maestro.** `cp /app/cluster_hosts.example /app/cluster_hosts` (o crearlo) con las IPs reales, `slots` y `max_slots`
iguales al número de núcleos lógicos de cada máquina. El maestro también debe aparecer.

**4. Paso B, ataque distribuido.** En el maestro, con la subred real del router y el total de slots:
```bash
bash /app/cluster_scripts/run_cluster.sh 192.168.1.0/24 12 <llave> data/texto.txt "memoria distribuida" cyclic 23
```
Equivale a `mpirun --hostfile cluster_hosts --mca btl_tcp_if_include <subred> --mca oob_tcp_if_include <subred> --display-map -np <slots> ./bruteforce ...`.
Para el texto de otro equipo habrá que usar la versión adaptada del programa (Etapa 3); el procedimiento de red es el mismo.

## Evidencia pedida por el PDF (Paso B)
1. Captura del **comando** ejecutado (`run_cluster.sh` lo imprime con `set -x`).
2. Captura de la salida con **distintos nodos participando**: el bloque `JOB MAP` de `--display-map` lista cada nodo (IP) y sus procesos.
3. Captura del **mensaje final** con la llave encontrada (`Key = ...` y el texto descifrado).

## Resultado del ensayo (dos contenedores en una sola máquina)
- Antes del Paso A: `root@172.30.0.3: Permission denied (publickey,password).` Después: `SSH sin contrasena OK en mpiworker`.
- `JOB MAP`: `mpimaster` (4 procesos, ranks 0 a 3) y `172.30.0.3` (4 procesos, ranks 4 a 7).
- `Key = 419430`, tiempo 1.56 a 1.62 s con 8 procesos. Con 4 procesos en un solo contenedor, la misma búsqueda tardó 2.38 s.
  Esta comparación **no** mide latencia de red real, porque ambos contenedores comparten la misma CPU.

## Hallazgos y riesgos
- **Nombres de host con guion bajo rompen OpenMPI 4.1.2.** Con `mpi_worker` el `orted` remoto se cae con un segfault
  (`orte_regx_base_nidmap_parse`) y mpirun reporta `ORTE has lost communication with a remote daemon`. Con `mpiworker`
  el mismo procedimiento funciona. Usar nombres como `nodo-maestro` y `nodo-worker1`.
- **Nombres de host únicos.** Con `--network host` el contenedor hereda el hostname de la máquina virtual de Docker, que puede repetirse
  entre compañeros. `--hostname` sí se acepta junto con `--network host` (probado en Docker 29.7.2).
- **`--network host` en Docker Desktop (Windows y macOS).** En la prueba de esta máquina, el contenedor veía solo las interfaces de la VM de Docker
  (`192.168.65.x` y `172.x`), no la IP del Wi-Fi de Windows. `btl_tcp_if_include` necesita una interfaz del contenedor que pertenezca a esa subred.
  Hay que activar "Enable Host Networking" (Settings > Resources > Network, Docker Desktop 4.34 o superior) y repetir la comprobación con
  `hostname -I` dentro del contenedor. Si sigue sin verse la IP del router, probar WSL2 con `networkingMode=mirrored`.
- **sshd.** El `CMD` de la imagen es `/bin/bash`; hay que arrancar `sshd` en cada nodo (`nodo_start.sh`). Con red host usa el puerto 22 de la máquina:
  puede chocar con un `sshd` del host (por ejemplo Remote Login en macOS).
- **Firewall de Windows.** Permitir tráfico entrante en redes privadas (PowerShell como administrador, ajustar según el caso):
  `New-NetFirewallRule -DisplayName "MPI Docker" -Direction Inbound -Action Allow -Profile Private,Domain -Protocol TCP`.
- **Router.** Algunas redes (institucionales o de invitados) aíslan a los clientes entre sí; entonces los nodos no se ven y no hay solución en el contenedor.
- **Mismo binario y datos.** Todos los nodos necesitan `/app/bruteforce` y los archivos de datos en la misma ruta (la imagen los trae con `COPY`).
