# Imagen estandar para el cluster MPI del Proyecto 2 Parte 2.
# Encapsula la compilacion y ejecucion de OpenMPI para que no dependa del
# sistema operativo del host (Windows, macOS, Linux).
FROM ubuntu:22.04

# Evita prompts interactivos de apt durante el build
ENV DEBIAN_FRONTEND=noninteractive

# OpenMPI prohibe correr procesos como root; en contenedores sin volumenes
# anulamos esa restriccion (enunciado, seccion IV).
ENV OMPI_ALLOW_RUN_AS_ROOT=1
ENV OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1

# Dependencias base obligatorias del enunciado + libssl-dev, que hace falta
# para enlazar con -lcrypto (DES de OpenSSL que usa el shim compat/).
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        openmpi-bin \
        libopenmpi-dev \
        openssh-server \
        libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# --- SSH sin intervencion: MPI instancia procesos en los workers por ssh ---
# Permitir login de root y quitar la verificacion manual de host keys.
RUN mkdir -p /var/run/sshd \
    && sed -i 's/#\?PermitRootLogin.*/PermitRootLogin yes/' /etc/ssh/sshd_config \
    && printf '\nHost *\n    StrictHostKeyChecking no\n    UserKnownHostsFile /dev/null\n' \
        >> /etc/ssh/ssh_config

# Llave sin passphrase y autoautorizada, para conexiones ssh libres.
RUN ssh-keygen -t rsa -N "" -f /root/.ssh/id_rsa \
    && cat /root/.ssh/id_rsa.pub >> /root/.ssh/authorized_keys \
    && chmod 700 /root/.ssh \
    && chmod 600 /root/.ssh/authorized_keys

# --- Codigo fuente y compilacion ---
WORKDIR /app
COPY compat ./compat
COPY data ./data
COPY bruteforce_fix.c ./
RUN mpicc -Wall -Icompat -Wno-deprecated-declarations -o bruteforce bruteforce_fix.c -lcrypto

# Puerto SSH para la comunicacion entre nodos
EXPOSE 22

CMD ["/bin/bash"]
