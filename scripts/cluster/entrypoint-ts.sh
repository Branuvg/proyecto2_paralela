#!/bin/bash
# Entrypoint de la imagen mpi-cluster-ts (Dockerfile.tailscale).
# Si se pasa la variable TS_AUTHKEY, el contenedor entra a la VPN de Tailscale
# con su propia IP (100.x.x.x) antes de arrancar lo que se le pida. Si no se
# pasa, se comporta igual que la imagen base (mpi-cluster-image).
set -e
if [ -n "$TS_AUTHKEY" ]; then
  mkdir -p /var/lib/tailscale /var/run/tailscale
  tailscaled --state=/var/lib/tailscale/tailscaled.state >/var/log/tailscaled.log 2>&1 &
  for i in $(seq 1 20); do tailscale status >/dev/null 2>&1 && break; sleep 0.5; done
  tailscale up --authkey="$TS_AUTHKEY" --hostname="${TS_HOSTNAME:-$(hostname)}" --accept-dns=false
  echo "Tailscale IP: $(tailscale ip -4 | head -1)"
fi
exec "$@"
