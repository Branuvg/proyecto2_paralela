#!/bin/bash
# Paso B (ataque distribuido). Ejecutar DENTRO del contenedor del Nodo Maestro, en /app.
#   run_cluster.sh <subred> <np> [argumentos de ./bruteforce...]
# Ejemplo:
#   run_cluster.sh 192.168.1.0/24 12 419430 data/texto.txt "memoria distribuida" cyclic 23
# Variables opcionales: HOSTFILE (por defecto /app/cluster_hosts), MAP=0 para no mostrar el mapa de procesos.
set -e
[ $# -ge 2 ] || { echo "Uso: $0 <subred> <np> [args de ./bruteforce]"; exit 1; }
SUBNET=$1; NP=$2; shift 2
HOSTFILE=${HOSTFILE:-/app/cluster_hosts}
MAPFLAG="--display-map"; [ "${MAP:-1}" = 0 ] && MAPFLAG=""
cd /app
set -x
mpirun --hostfile "$HOSTFILE" \
  --mca btl_tcp_if_include "$SUBNET" --mca oob_tcp_if_include "$SUBNET" \
  $MAPFLAG -np "$NP" ./bruteforce "$@"
