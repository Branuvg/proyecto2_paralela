/*
 * Shim de <rpc/des_crypt.h> para glibc >= 2.32/2.43, que ya no incluye este header.
 * Implementa ecb_crypt y des_setparity con la API DES de OpenSSL, con la misma
 * semantica que glibc, para compilar bruteforce_.c sin modificarlo.
 * Compilar con: -Icompat  y enlazar con: -lcrypto
 */
#ifndef COMPAT_RPC_DES_CRYPT_H
#define COMPAT_RPC_DES_CRYPT_H

#include <openssl/des.h>

#define DES_ENCRYPT_MODE 0   /* valores de glibc */
#define DES_DECRYPT_MODE 1
#undef DES_ENCRYPT           /* OpenSSL los define con otro valor */
#undef DES_DECRYPT
#define DES_ENCRYPT DES_ENCRYPT_MODE
#define DES_DECRYPT DES_DECRYPT_MODE
#define DESERR_NONE 0
#define DESERR_BADPARAM 3

static inline void des_setparity(char *key) {
  DES_set_odd_parity((DES_cblock *)key);
}

static inline int ecb_crypt(char *key, char *data, unsigned datalen, unsigned mode) {
  DES_key_schedule ks;
  if (datalen % 8) return DESERR_BADPARAM;
  DES_set_key_unchecked((const_DES_cblock *)key, &ks);
  for (unsigned i = 0; i < datalen; i += 8)
    DES_ecb_encrypt((const_DES_cblock *)(data + i), (DES_cblock *)(data + i), &ks,
                    mode == DES_ENCRYPT_MODE ? 1 : 0);
  return DESERR_NONE;
}

#endif
