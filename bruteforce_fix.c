/*
 * bruteforce_fix.c - version corregida e instrumentada de bruteforce_.c
 *
 * Uso (modo propio, Parte 1/2 - cifra el texto y luego lo busca):
 *   mpirun -np N bruteforce_fix <llave> <archivo_texto> "<frase clave>" [block|cyclic] [bits]
 *
 * Uso (modo cruzado, Parte 3 - el texto ya viene cifrado por otro equipo):
 *   mpirun -np N bruteforce_fix --b64 <archivo.b64> "<frase clave>" [block|cyclic] [bits]
 *
 *   bits    : (opcional, default 56) tamano del espacio de busqueda 2^bits, para experimentos
 *             a escala donde el reparto en bloques sigue siendo significativo
 *   llave   : entero en [0, 2^56) con el que se cifra el texto (solo modo propio)
 *   --b64   : <archivo.b64> es el ciphertext de otro equipo en base64 (una linea);
 *             no se cifra nada, solo se decodifica y se busca la llave que lo descifra
 *   block   : cada proceso recibe un bloque contiguo del espacio (como el original)
 *   cyclic  : el proceso id prueba id, id+N, id+2N, ... (reparto intercalado)
 *
 * Cambios respecto al original (ver docs/01-hallazgos-codigo.md):
 *   - ecb_crypt usa el largo real (multiplo de 8, con padding de ceros), no 16 fijo
 *   - mascara 0xFEUL y llave unsigned long (sin UB en el desplazamiento)
 *   - llave, texto y frase por argumentos
 *   - fin limpio si nadie encuentra la llave (no hay MPI_Wait colgado)
 *   - MPI_Test cada TEST_EVERY llaves, no en cada una
 *   - medicion con MPI_Wtime, llaves probadas y tiempo por proceso
 *   - modo --b64 (Parte 3): lee y decodifica el ciphertext de otro equipo en vez
 *     de cifrar un texto propio (ver docs/10-descifrado-cruzado.md)
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <rpc/des_crypt.h>

#define TEST_EVERY 1024
#define NOT_FOUND  (-1L)

static void expand_key(unsigned long key, char *k8) {
  unsigned long k = 0;
  for (int i = 0; i < 8; ++i) {
    key <<= 1;
    k += (key & (0xFEUL << i * 8));
  }
  memcpy(k8, &k, 8);
  des_setparity(k8);
}

void decrypt(unsigned long key, char *ciph, int len) {
  char k8[8];
  expand_key(key, k8);
  ecb_crypt(k8, ciph, len, DES_DECRYPT);
}

void encrypt(unsigned long key, char *ciph, int len) {
  char k8[8];
  expand_key(key, k8);
  ecb_crypt(k8, ciph, len, DES_ENCRYPT);
}

static const char *search;

/* len = largo del buffer cifrado (multiplo de 8) */
int tryKey(unsigned long key, const char *ciph, int len) {
  char temp[len + 1];
  memcpy(temp, ciph, len);
  temp[len] = 0;
  decrypt(key, temp, len);
  return strstr(temp, search) != NULL;
}

static char *read_file(const char *path, int *n) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = calloc(sz + 8, 1);          /* ceros extra para el padding */
  if (fread(buf, 1, sz, f) != (size_t)sz) { fclose(f); free(buf); return NULL; }
  fclose(f);
  *n = (int)sz;
  return buf;
}

/* valor 0-63 de un caracter base64, o -1 si no es base64 (incluye '=', saltos
 * de linea y espacios, que simplemente se ignoran al decodificar) */
static int b64_val(unsigned char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}

/* Lee un archivo de texto en base64 (como lo publican otros equipos) y
 * devuelve el ciphertext decodificado, recortado al multiplo de 8 completo
 * (ecb_crypt necesita el largo exacto en bloques de 8 bytes). */
static char *read_b64_file(const char *path, int *n) {
  int rawlen = 0;
  char *raw = read_file(path, &rawlen);
  if (!raw) return NULL;
  unsigned char *out = malloc(rawlen);     /* cota superior, nunca se excede */
  int outlen = 0;
  unsigned int acc = 0;                    /* unsigned: evita overflow con signo */
  int accbits = 0;
  for (int i = 0; i < rawlen; i++) {
    int v = b64_val((unsigned char)raw[i]);
    if (v < 0) continue;
    acc = (acc << 6) | (unsigned int)v;
    accbits += 6;
    if (accbits >= 8) {
      accbits -= 8;
      out[outlen++] = (char)((acc >> accbits) & 0xFF);
    }
  }
  free(raw);
  *n = (outlen / 8) * 8;                   /* descarta un bloque incompleto, si hubiera */
  return (char *)out;
}

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);
  MPI_Comm comm = MPI_COMM_WORLD;
  int N, id;
  MPI_Comm_size(comm, &N);
  MPI_Comm_rank(comm, &id);

  int is_b64 = (argc > 1 && strcmp(argv[1], "--b64") == 0);

  if (argc < 4 || (is_b64 && argc < 4)) {
    if (id == 0)
      fprintf(stderr,
        "Uso: %s <llave> <archivo_texto> \"<frase>\" [block|cyclic] [bits]\n"
        "  o: %s --b64 <archivo.b64> \"<frase>\" [block|cyclic] [bits]\n",
        argv[0], argv[0]);
    MPI_Finalize();
    return 1;
  }

  int bits = (argc > 5) ? atoi(argv[5]) : 56;
  const unsigned long upper = 1UL << bits;          /* espacio de llaves (DES: 2^56) */
  unsigned long the_key = is_b64 ? 0 : strtoul(argv[1], NULL, 10);
  search = argv[3];
  int cyclic = (argc > 4 && strcmp(argv[4], "cyclic") == 0);

  int ciphlen = 0;
  char *cipher;
  if (is_b64) {
    /* Parte 3: el texto ya viene cifrado por otro equipo, solo se decodifica */
    cipher = read_b64_file(argv[2], &ciphlen);
    if (!cipher || ciphlen == 0) {
      if (id == 0) fprintf(stderr, "Error: no se pudo leer/decodificar %s\n", argv[2]);
      MPI_Finalize();
      return 1;
    }
  } else {
    int textlen = 0;
    char *text = read_file(argv[2], &textlen);
    if (!text || the_key >= upper) {
      if (id == 0) fprintf(stderr, "Error: archivo ilegible o llave fuera de [0, 2^56)\n");
      MPI_Finalize();
      return 1;
    }
    ciphlen = (textlen + 7) / 8 * 8;                /* multiplo de 8 */
    cipher = malloc(ciphlen + 1);
    memcpy(cipher, text, ciphlen);
    encrypt(the_key, cipher, ciphlen);
    free(text);
  }
  cipher[ciphlen] = 0;

  unsigned long mylower, myupper, step;
  if (cyclic) {
    mylower = id; myupper = upper; step = N;
  } else {
    unsigned long range = upper / N;
    mylower = range * id;
    myupper = (id == N - 1) ? upper : range * (id + 1);
    step = 1;
  }

  long found = NOT_FOUND, local = NOT_FOUND;
  unsigned long tried = 0;
  int ready = 0;
  MPI_Request req;
  MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, 0, comm, &req);

  MPI_Barrier(comm);
  double t0 = MPI_Wtime();

  for (unsigned long i = mylower; i < myupper; i += step, ++tried) {
    if (tried % TEST_EVERY == 0) {
      MPI_Test(&req, &ready, MPI_STATUS_IGNORE);
      if (ready) break;
    }
    if (tryKey(i, cipher, ciphlen)) {
      local = (long)i;
      for (int node = 0; node < N; node++)
        if (node != id) MPI_Send(&local, 1, MPI_LONG, node, 0, comm);
      break;
    }
  }
  double elapsed = MPI_Wtime() - t0;

  /* si nadie aviso (llave no encontrada o este proceso la hallo), cerrar el Irecv */
  if (!ready) { MPI_Cancel(&req); MPI_Wait(&req, MPI_STATUS_IGNORE); }

  long result;
  double tmax;
  unsigned long *all_tried = id == 0 ? malloc(N * sizeof(unsigned long)) : NULL;
  double *all_time = id == 0 ? malloc(N * sizeof(double)) : NULL;
  MPI_Reduce(&local, &result, 1, MPI_LONG, MPI_MAX, 0, comm);
  MPI_Reduce(&elapsed, &tmax, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
  MPI_Gather(&tried, 1, MPI_UNSIGNED_LONG, all_tried, 1, MPI_UNSIGNED_LONG, 0, comm);
  MPI_Gather(&elapsed, 1, MPI_DOUBLE, all_time, 1, MPI_DOUBLE, 0, comm);

  if (id == 0) {
    for (int p = 0; p < N; p++)
      printf("PROC,%d,%lu,%.6f\n", p, all_tried[p], all_time[p]);
    if (result == NOT_FOUND) {
      printf("Llave no encontrada\n");
    } else {
      decrypt((unsigned long)result, cipher, ciphlen);
      printf("Key = %ld\n\n%s\n\n", result, cipher);
      printf("CSV,%d,%s,%lu,%ld,%.6f\n", N, cyclic ? "cyclic" : "block", the_key, result, tmax);
    }
  }

  MPI_Finalize();
  return 0;
}
