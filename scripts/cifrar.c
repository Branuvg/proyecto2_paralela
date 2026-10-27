/*
 * cifrar.c - cifra/descifra un archivo con DES (ECB) usando la misma expansion de llave que bruteforce_fix.c.
 * Uso: cifrar enc|dec <llave> <entrada> <salida>
 *   enc: rellena con ceros hasta multiplo de 8 y cifra. dec: descifra (la salida conserva el padding).
 * Compilar: gcc -Icompat -Wno-deprecated-declarations -o build/cifrar scripts/cifrar.c -lcrypto
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rpc/des_crypt.h>

static void expand_key(unsigned long key, char *k8) {
  unsigned long k = 0;
  for (int i = 0; i < 8; ++i) {
    key <<= 1;
    k += (key & (0xFEUL << i * 8));
  }
  memcpy(k8, &k, 8);
  des_setparity(k8);
}

int main(int argc, char **argv) {
  if (argc != 5 || (strcmp(argv[1], "enc") && strcmp(argv[1], "dec"))) {
    fprintf(stderr, "Uso: %s enc|dec <llave> <entrada> <salida>\n", argv[0]);
    return 1;
  }
  unsigned long key = strtoul(argv[2], NULL, 10);
  FILE *f = fopen(argv[3], "rb");
  if (!f) { perror("entrada"); return 1; }
  fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
  int len = (int)((n + 7) / 8 * 8);
  char *buf = calloc(len + 8, 1);
  if (fread(buf, 1, n, f) != (size_t)n) { perror("lectura"); return 1; }
  fclose(f);
  char k8[8];
  expand_key(key, k8);
  ecb_crypt(k8, buf, len, argv[1][0] == 'e' ? DES_ENCRYPT : DES_DECRYPT);
  FILE *o = fopen(argv[4], "wb");
  if (!o) { perror("salida"); return 1; }
  fwrite(buf, 1, len, o);
  fclose(o);
  printf("%s: %ld bytes de entrada -> %d bytes de salida\n", argv[1], n, len);
  return 0;
}
