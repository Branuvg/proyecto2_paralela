# 09 — Clúster físico con Tailscale (Windows + Mac, 2-3 personas)

Guía paso a paso para conectar el clúster MPI entre máquinas de distintos sistemas operativos usando
**Tailscale** (VPN mesh basada en WireGuard, corre dentro de cada contenedor) en vez de `--network host`.
Complementa `08-cluster.md` (que documenta el intento con red host de Docker, ver Apéndice de este archivo
para el porqué no funcionó en Windows). Validado end-to-end el 2026-10-28: imagen construida, `/dev/net/tun`
accesible, contenedor conectado a Tailscale y con IP `100.x` asignada correctamente.

## 0. Por qué Tailscale y no `--network host`

Se probó primero el camino "obvio" (`docker run --network host`, como sugiere el enunciado en la Sección V)
y **no funcionó en Windows**: Docker Desktop no comparte la interfaz de red física (Wi-Fi/Ethernet) con el
contenedor, sin importar qué tan bien configurado esté "Enable host networking", el firewall o WSL2. El
contenedor siempre ve una IP interna de emulación (`192.168.65.x` / `172.x`), nunca la IP real del router.
Causa: en Linux, `--network host` comparte literalmente el namespace de red del host; en Docker Desktop
(Windows/Mac), el "host" que ve el contenedor es la VM interna de Docker, no la máquina física. Esto está
documentado a detalle en la sección 9 (Apéndice) de esta guía, por si quieren intentarlo o entender por qué
falló ayer.

**Tailscale evita el problema por completo**: en vez de depender de que Docker comparta la red física,
Tailscale corre un demonio (`tailscaled`) **dentro del propio contenedor**, crea su propia interfaz virtual
(`tailscale0`) y le asigna una IP `100.x.x.x` fija — igual en Windows, Mac y Linux, sin importar NAT,
firewalls de router ni el motor de Docker que use cada persona. Solo necesita dos flags al crear el
contenedor: `--cap-add NET_ADMIN --device /dev/net/tun`. No necesita `--network host`.

Esto es lo mismo que resolvió el problema a un grupo compañero de clase (revisado el 2026-10-28): intentaron
primero con red host de Docker Desktop y con Radmin VPN, ninguna funcionó entre Windows y Mac; con Tailscale
sí lograron el clúster completo.

---

## 1. Qué construir (cada persona, una sola vez)

Desde la raíz del repo (`proyecto2_paralela`):

```bash
docker build -t mpi-cluster-image .
docker build --network host -t mpi-cluster-ts -f Dockerfile.tailscale .
```

- `mpi-cluster-image`: la imagen base del proyecto (sin cambios).
- `mpi-cluster-ts`: capa adicional que instala Tailscale encima de la base (`Dockerfile.tailscale`, nuevo
  en el repo). El flag `--network host` en el build (no en el run) ayuda a evitar fallos intermitentes de
  resolución de DNS/TLS al descargar el instalador de Tailscale.

### 1.1 Si el build de `mpi-cluster-ts` falla con un error de certificado SSL

```
curl: (60) SSL certificate problem: unable to get local issuer certificate
```

Visto en la práctica conectados a la red de la universidad: el firewall institucional (Fortinet, en el caso
de UVG) intercepta el tráfico HTTPS saliente y rompe la verificación del certificado de `tailscale.com`.
**Solución: construir la imagen desde otra red** (datos del celular, casa, cualquier red sin ese firewall).
Una vez construida la imagen, sí puedes volver a la red de la universidad para el resto de las pruebas —
el problema es solo al descargar el instalador durante el build, no al usar Tailscale después.

Para confirmar si es esto antes de cambiar de red:
```bash
docker run --rm --network host ubuntu:22.04 sh -c \
  "apt-get update >/dev/null 2>&1; apt-get install -y openssl >/dev/null 2>&1; \
   openssl s_client -connect tailscale.com:443 -servername tailscale.com </dev/null 2>&1 | grep issuer="
```
Si el `issuer=` muestra `Fortinet` (o cualquier otra marca de firewall corporativo) en vez de una autoridad
real (Amazon, Let's Encrypt, DigiCert, etc.), confirma el problema — cambia de red y repite el build.

---

## 2. Clave de Tailscale (solo la genera una persona del equipo, una vez)

1. Crear una cuenta gratis en **https://login.tailscale.com** (puede ser con Google/GitHub/Microsoft).
2. Ir a **Settings → Keys → Generate auth key**.
3. Marcar **Reusable** (para que sirva para los 2-3 contenedores) y que expire en **1 día**.
4. Copiar la clave (empieza con `tskey-auth-...`) y pasarla al resto del equipo por un chat privado.
   **No subirla al repo, al docx ni pegarla en ningún lugar público.**
5. Al terminar las pruebas del día, **revocar la clave** en la misma pantalla (Settings → Keys) y, si
   ya no se necesitan, borrar los dispositivos de prueba en Settings → Machines.

---

## 3. Elegir roles

Decidan quién es el **Nodo Maestro** (el que lanza `mpirun`) y quiénes son **Nodos Worker**. No importa el
SO de cada uno.

Ejemplo con 3 personas:
| Rol | Persona | SO |
|---|---|---|
| Maestro | A | Windows |
| Worker 1 | B | Mac |
| Worker 2 | C | Windows |

---

## 4. Levantar el contenedor (cada persona, con su propio nombre)

Mismo comando para todos, cambiando `--hostname`, `--name` y `TS_HOSTNAME` (deben ser únicos, **sin guion
bajo** — con guion bajo OpenMPI 4.1.2 falla con un segfault en `orted`):

```bash
# Maestro (persona A)
docker run -d --name mpi_ts --hostname nodo-maestro \
  --cap-add NET_ADMIN --device /dev/net/tun \
  -e TS_AUTHKEY=tskey-auth-XXXX -e TS_HOSTNAME=nodo-maestro \
  mpi-cluster-ts sleep infinity

# Worker 1 (persona B)
docker run -d --name mpi_ts --hostname nodo-worker1 \
  --cap-add NET_ADMIN --device /dev/net/tun \
  -e TS_AUTHKEY=tskey-auth-XXXX -e TS_HOSTNAME=nodo-worker1 \
  mpi-cluster-ts sleep infinity

# Worker 2 (persona C)
docker run -d --name mpi_ts --hostname nodo-worker2 \
  --cap-add NET_ADMIN --device /dev/net/tun \
  -e TS_AUTHKEY=tskey-auth-XXXX -e TS_HOSTNAME=nodo-worker2 \
  mpi-cluster-ts sleep infinity
```

Si ya tenías un contenedor `mpi_ts` de una prueba anterior: `docker rm -f mpi_ts` antes de crear el nuevo.

**Verificar que entró a la VPN** (debe tardar unos segundos en aparecer la IP):
```bash
docker logs mpi_ts
# Debe imprimir: "Tailscale IP: 100.x.x.x"

docker exec mpi_ts tailscale ip -4
```
Si no imprime nada o tarda mucho, revisar la sección 8 (troubleshooting).

**Anotar la IP de Tailscale de cada persona** (todos deben compartir la suya con el resto del equipo, por
ejemplo en el mismo chat donde compartieron la authkey):

| Persona | Hostname | IP de Tailscale (ejemplo) |
|---|---|---|
| A (maestro) | nodo-maestro | 100.88.93.44 |
| B | nodo-worker1 | 100.68.142.84 |
| C | nodo-worker2 | 100.x.x.x |

**Probar que se ven entre ellos** (cada persona hace ping a los otros dos):
```bash
docker exec mpi_ts tailscale ping 100.68.142.84
```

---

## 5. Entrar al contenedor y preparar el nodo

Cada persona, en su propia máquina:
```bash
docker exec -it mpi_ts bash
```

Dentro, copiar los scripts de cluster (desde otra terminal del host, en la raíz del repo) y arrancar sshd:
```bash
# Desde el host (fuera del contenedor), en otra terminal:
docker cp scripts/cluster mpi_ts:/app/cluster_scripts
docker cp cluster_hosts.example mpi_ts:/app/cluster_hosts.example

# Dentro del contenedor:
bash /app/cluster_scripts/nodo_start.sh
```
Esto arranca `sshd` e imprime hostname, IPs y núcleos (`nproc`) — anota los núcleos reales de cada máquina,
los van a necesitar para el hostfile.

---

## 6. Paso A — Sincronizar llaves SSH (usando las IPs de Tailscale)

**En el Maestro**, generar/mostrar la llave pública:
```bash
bash /app/cluster_scripts/ssh_keys.sh pub
```
Copiar la línea completa (`ssh-rsa AAAA...`).

**En cada Worker**, autorizarla:
```bash
bash /app/cluster_scripts/ssh_keys.sh add "ssh-rsa AAAA... root@nodo-maestro"
```

**De nuevo en el Maestro**, probar cada worker **usando su IP de Tailscale** (no la del router):
```bash
bash /app/cluster_scripts/ssh_keys.sh test 100.68.142.84   # Worker 1
bash /app/cluster_scripts/ssh_keys.sh test 100.x.x.x       # Worker 2
```
Esperado: `SSH sin contrasena OK en nodo-workerX (<ip>)`.

---

## 7. Hostfile y Paso B — Ataque distribuido

**En el Maestro**, crear el hostfile con las IPs de Tailscale (no la plantilla `cluster_hosts.example`, que
trae IPs de router de ejemplo):
```bash
cat > /app/cluster_hosts << 'EOF'
100.88.93.44 slots=4 max_slots=4
100.68.142.84 slots=4 max_slots=4
100.x.x.x slots=4 max_slots=4
EOF
```
Ajustar `slots`/`max_slots` a los núcleos reales de cada máquina (del paso 5, `nproc`). No pasarse de los
núcleos físicos.

**Ataque distribuido**, usando `tailscale0` como interfaz en vez de una subred (es el mismo nombre de
interfaz en todas las máquinas, aunque estén en redes distintas):
```bash
bash /app/cluster_scripts/run_cluster.sh tailscale0 12 4206649 data/texto.txt "memoria distribuida" cyclic 23
```
Esto equivale a:
```bash
mpirun --hostfile /app/cluster_hosts \
  --mca btl_tcp_if_include tailscale0 --mca oob_tcp_if_include tailscale0 \
  --display-map -np 12 ./bruteforce 4206649 data/texto.txt "memoria distribuida" cyclic 23
```

Capturar pantalla de:
1. El comando ejecutado (el script lo imprime con `set -x`).
2. El bloque `JOB MAP` mostrando los distintos hostnames participando.
3. El mensaje final `Key = ...` con el texto descifrado.

---

## 8. Troubleshooting

| Síntoma | Causa | Solución |
|---|---|---|
| `docker logs mpi_ts` no imprime la IP de Tailscale | La authkey expiró, está mal copiada, o no llegó a tiempo | `docker exec mpi_ts cat /var/log/tailscaled.log`; generar una authkey nueva si expiró |
| `docker run` falla o `/dev/net/tun` no existe dentro del contenedor | Faltó `--cap-add NET_ADMIN` o `--device /dev/net/tun` | Agregar ambos flags al `docker run` |
| El build de `mpi-cluster-ts` falla con error de certificado SSL | Firewall institucional interceptando HTTPS (ver sección 1.1) | Construir la imagen desde otra red (datos móviles, casa) |
| `tailscale ping` responde pero tarda mucho (100-300ms) | No hay conexión P2P directa, el tráfico pasa por un relay (DERP) de Tailscale | Normal entre redes con NAT estricto; funciona igual, solo más lento. Si molesta, revisar NAT/UPnP del router de cada quien |
| `Permission denied (publickey,password)` en `ssh_keys.sh test` | Llave no autorizada o copiada mal | Repetir el Paso A, revisar que no se cortó la llave al copiar/pegar |
| `mpirun` se cuelga sin error | Falta especificar `tailscale0` en los `--mca`, o se usó una subred en vez del nombre de interfaz | Usar exactamente `--mca btl_tcp_if_include tailscale0 --mca oob_tcp_if_include tailscale0` |
| `orte_regx_base_nidmap_parse` / segfault en `orted` remoto | Hostname con guion bajo | Usar hostnames con guiones normales: `nodo-worker1`, nunca `nodo_worker1` |
| `/app/bruteforce` no existe en otro nodo, o el hostfile apunta a una ruta distinta | Cada quien construyó la imagen en un momento distinto o con cambios no sincronizados | Confirmar que todos reconstruyeron con el mismo `Dockerfile`/`bruteforce_fix.c` del repo (mismo commit) |
| La IP de Tailscale de alguien cambia entre sesiones | Normal si se borra y recrea el contenedor sin IP estática asignada en el admin de Tailscale | Revisar `tailscale ip -4` de nuevo antes de cada corrida; actualizar `cluster_hosts` si cambió |

---

## 9. Checklist final antes de grabar evidencia

- [ ] Las dos imágenes construidas (`mpi-cluster-image`, `mpi-cluster-ts`) por cada persona.
- [ ] Authkey generada, compartida de forma privada, y **revocada al terminar**.
- [ ] Los 3 contenedores con `--hostname`/`TS_HOSTNAME` únicos, sin guion bajo.
- [ ] `docker logs mpi_ts` confirma IP `100.x` en cada contenedor.
- [ ] `tailscale ping` entre todos los nodos responde.
- [ ] Paso A completado (SSH sin contraseña maestro → cada worker, por IP de Tailscale).
- [ ] `cluster_hosts` con las IPs de Tailscale y `slots`/`max_slots` ajustados a núcleos físicos reales.
- [ ] `run_cluster.sh ... tailscale0 ...` corrido desde el Maestro.
- [ ] Captura del comando, del `JOB MAP` y del `Key = ...` final.

---

## Apéndice: por qué falló `--network host` (contexto de lo intentado ayer)

Se intentó primero seguir la Sección V del enunciado al pie de la letra (`docker run --network host`).
Diagnóstico completo:

- Docker Desktop en Windows ≥4.34 con "Enable host networking" activo, firewall configurado, WSL2 en modo
  `mirrored`: el contenedor seguía viendo solo IPs internas (`192.168.65.x`/`172.x`), nunca la IP real del
  router.
- Confirmado con un motor Docker nativo instalado dentro de WSL2 (no la app Docker Desktop): ahí sí
  `--network host` funciona como en Linux real y el contenedor hereda la IP física. Pero esto depende de
  tener ese motor nativo instalado y de recordar invocar `docker` desde dentro de WSL (`wsl -u root --`)
  en cada comando, lo cual es frágil para coordinar entre 3 personas con sistemas distintos.
- Para Mac no se llegó a confirmar si Docker Desktop tiene la misma limitación (usa otra virtualización,
  Apple Virtualization Framework).
- Además, la IP física puede cambiar a media sesión (ej. al cambiar de hotspot a Wi-Fi de la universidad),
  lo que rompe cualquier hostfile ya armado.

Tailscale evita los tres problemas a la vez (no depende del motor de Docker, es igual en Windows/Mac/Linux,
y la IP de Tailscale no cambia aunque cambien de red física), por eso se adoptó como método principal.
