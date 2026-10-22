#!/bin/bash
# Ejecutar DENTRO del contenedor de cada nodo (maestro y workers): arranca sshd y muestra datos de red.
service ssh start
echo "hostname : $(hostname)"
echo "IPs      : $(hostname -I)"
echo "nucleos  : $(nproc)"
echo "binario  : $(ls -l /app/bruteforce 2>&1)"
