#!/bin/bash
# Paso A (sincronización de llaves SSH). Ejecutar DENTRO del contenedor.
#   ssh_keys.sh pub            Nodo Maestro: imprime su llave pública (la crea si no existe)
#   ssh_keys.sh add "<pub>"    Worker: autoriza la llave pública del maestro (idempotente)
#   ssh_keys.sh test <ip>      Maestro: comprueba el acceso SSH sin contraseña a un worker
set -e
case "$1" in
  pub)
    [ -f /root/.ssh/id_rsa ] || ssh-keygen -q -t rsa -N "" -f /root/.ssh/id_rsa
    cat /root/.ssh/id_rsa.pub
    ;;
  add)
    [ -n "$2" ] || { echo "Falta la llave pública entre comillas"; exit 1; }
    mkdir -p /root/.ssh && chmod 700 /root/.ssh
    grep -qxF "$2" /root/.ssh/authorized_keys 2>/dev/null || echo "$2" >> /root/.ssh/authorized_keys
    chmod 600 /root/.ssh/authorized_keys
    echo "Llave autorizada."
    ;;
  test)
    [ -n "$2" ] || { echo "Falta la IP del worker"; exit 1; }
    ssh -o BatchMode=yes -o ConnectTimeout=5 "$2" 'echo "SSH sin contrasena OK en $(hostname) ($(hostname -I))"'
    ;;
  *)
    echo "Uso: $0 pub | add \"<llave publica>\" | test <ip>"; exit 1
    ;;
esac
