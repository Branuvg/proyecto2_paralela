
# En Mac el OpenSSL de Homebrew no esta en la ruta por defecto y <unistd.h>
# este script para las que usamos maaaac

set -e
cd "$(dirname "$0")/.."
mkdir -p build

OSSL=$(brew --prefix openssl@3)
FLAGS=(-Wall -Icompat -Wno-deprecated-declarations -D_POSIX_C_SOURCE=200809L
       -I"$OSSL/include" -L"$OSSL/lib")

mpicc "${FLAGS[@]}" -o build/bruteforce_fix bruteforce_fix.c -lcrypto
echo "OK: build/bruteforce_fix"
echo "Nota: build/bruteforce_orig no se compila en macOS (incluye <unistd.h> y no se modifica)."
