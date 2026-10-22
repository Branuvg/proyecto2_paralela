# Bitácora — Proyecto 2 Parte 2

Plan en tres etapas: **(1)** imagen Docker y comunicación local, **(2)** clúster físico con el equipo,
**(3)** descifrado cruzado y análisis.

## 2026-10-22 — Etapa 1: Imagen Docker y comunicación local

**Objetivo:** tener la imagen estándar levantada y verificar la comunicación SSH/MPI.

**Archivos agregados**
- `Dockerfile` (raíz): imagen del clúster sobre `ubuntu:22.04`.
- `.dockerignore`: evita mandar `build/`, `docs/`, `.git`, etc. al build.

**Decisiones del Dockerfile**
- Paquetes obligatorios: `build-essential`, `openmpi-bin`, `libopenmpi-dev`, `openssh-server`.
- **Agregado `libssl-dev`** (no está en los 4 obligatorios): el código enlaza con `-lcrypto` porque el
  shim `compat/rpc/des_crypt.h` usa el DES de OpenSSL.
- SSH sin intervención: `PermitRootLogin yes`, `StrictHostKeyChecking no`, llave sin passphrase
  generada en el build y autoautorizada.
- `ENV OMPI_ALLOW_RUN_AS_ROOT=1` y `OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1` (OpenMPI prohíbe correr como root).
- Compila el binario como `/app/bruteforce`.

**Construcción**
```bash
docker build -t mpi-cluster-image .
```

**Tabla de validación (enunciado, sección IV)**
| Validación | Esperado | ¿Funcionó? |
|---|---|---|
| `mpicc --version` | gcc 11.4.0 (Ubuntu 22.04) | ✔ `11.4.0` |
| `mpirun -np 2 echo "Proceso MPI ejecutado"` | lo imprime 2 veces | ✔ |
| `service ssh start` | arranca sshd | ✔ |
| `ssh localhost echo "Conexion SSH sin contrasena exitosa"` | imprime el mensaje | ✔ |
| `mpirun -np 4 ./bruteforce 419430 data/texto.txt "memoria distribuida" cyclic 23` | halla la llave | ✔ `Key = 419430` |

**Hallazgo (pendiente para la Etapa 3):** `bruteforce_fix.c` cifra el texto él mismo con la llave que
recibe y luego la busca. En la Parte 2 recibiremos un texto ya cifrado por otro equipo, así que hay que
adaptarlo para leer un ciphertext desde archivo.

**Estado**
- [x] Dockerfile con las especificaciones obligatorias.
- [x] Imagen `mpi-cluster-image` construida y validada localmente.
- [ ] Etapa 2: hostfile, `--network host`, llaves SSH inter-nodo, `--mca`, ataque distribuido.
- [ ] Etapa 3: leer ciphertext de otro equipo + análisis.
