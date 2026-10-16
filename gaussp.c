#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>

#include "pvm3.h"

#define GRPNAME "grupinhoS2"
#define PIVOT_TAG 101
#define GATHER_TAG 102

// each process keeps its own rows, simple and pratico
int local_rows(int N, int me, int nproc) {
  if (me >= N) {
    return 0;
  }
  return (N - 1 - me) / nproc + 1;
}

void matrix_load(char nom[], double *tab, int N, int me, int tids[], int nproc) {
  FILE *f;
  double *row;
  int i, j, target, count;

  count = local_rows(N, me, nproc);

  if (me == 0) {
    // process zero reads the file then distribui the rows
    f = fopen(nom, "r");
    row = malloc(N * sizeof(double));

    for (i = 0; i < N; i++) {
      for (j = 0; j < N; j++) {
        fscanf(f, "%lf", &row[j]);
      }

      target = i % nproc;
      if (target == 0) {
        for (j = 0; j < N; j++) {
          tab[(i / nproc) * N + j] = row[j];
        }
      } else {
        pvm_initsend(PvmDataDefault);
        pvm_pkdouble(row, N, 1);
        pvm_send(tids[target], PIVOT_TAG);
      }
    }

    fclose(f);
    free(row);
  } else {
    for (i = 0; i < count; i++) {
      pvm_recv(tids[0], PIVOT_TAG);
      pvm_upkdouble(&tab[i * N], N, 1);
    }
  }
}

void matrix_save(char nom[], double *tab, int N, int me, int tids[], int nproc) {
  FILE *f;
  double *rows;
  char output[255];
  int i, count, source;

  count = local_rows(N, me, nproc);

  if (me == 0) {
    // here we put the rows back in the original order
    rows = malloc(N * N * sizeof(double));
    for (i = 0; i < count; i++) {
      memcpy(&rows[(i * nproc) * N],
             &tab[i * N],
             N * sizeof(double));
    }

    for (source = 1; source < nproc; source++) {
      int source_count = local_rows(N, source, nproc);
      int row;
      double *source_rows = malloc(source_count * N * sizeof(double));

      pvm_recv(tids[source], GATHER_TAG);
      pvm_upkdouble(source_rows, source_count * N, 1);
      for (row = 0; row < source_count; row++) {
        memcpy(&rows[(source + row * nproc) * N],
               &source_rows[row * N],
               N * sizeof(double));
      }
      free(source_rows);
    }

    strcpy(output, nom);
    strcat(output, ".result");
    f = fopen(output, "w");
    for (i = 0; i < N * N; i++) {
      fprintf(f, "%8.2f%c", rows[i], (i % N == N - 1) ? '\n' : ' ');
    }
    fclose(f);
    free(rows);
  } else {
    pvm_initsend(PvmDataDefault);
    pvm_pkdouble(tab, count * N, 1);
    pvm_send(tids[0], GATHER_TAG);
  }
}

void gauss(double *tab, int N, int me, int tids[], int nproc) {
  double *pivot_row;
  double factor;
  int k, i, j, owner, owner_row, count;
  int *receivers;
  int receiver_count;

  pivot_row = malloc(N * sizeof(double));
  receivers = malloc((nproc - 1) * sizeof(int));

  for (k = 0; k < N - 1; k++) {
    // the pivot owner shares the line so everybody can calculare
    owner = k % nproc;
    owner_row = k / nproc;

    if (me == owner) {
      for (j = 0; j < N; j++) {
        pivot_row[j] = tab[owner_row * N + j];
      }

      receiver_count = 0;
      for (i = 0; i < nproc; i++) {
        if (i != owner) {
          receivers[receiver_count++] = tids[i];
        }
      }
      pvm_initsend(PvmDataDefault);
      pvm_pkdouble(pivot_row, N, 1);
      pvm_mcast(receivers, receiver_count, PIVOT_TAG);
    } else {
      pvm_recv(tids[owner], PIVOT_TAG);
      pvm_upkdouble(pivot_row, N, 1);
    }

    count = local_rows(N, me, nproc);
    for (i = 0; i < count; i++) {
      int global_row = me + i * nproc;
      if (global_row > k) {
        // each process updates only its local work
        factor = tab[i * N + k] / pivot_row[k];
        for (j = k + 1; j < N; j++) {
          tab[i * N + j] -= factor * pivot_row[j];
        }
        tab[i * N + k] = 0.0;
      }
    }
  }

  free(receivers);
  free(pivot_row);
}

int dowork(int argc, char **argv, int me, int tids[], int nproc) {
  int N;
  double *tab;
  struct timeval start, end;
  double elapsed;

  if (argc != 3) {
    printf("Usage: %s <matrix size> <matrix name>\n", argv[0]);
    exit(-1);
  }

  N = atoi(argv[1]);
  tab = malloc(N * N * sizeof(double));

  matrix_load(argv[2], tab, N, me, tids, nproc);
  pvm_barrier(GRPNAME, nproc);

  // only the elimination time counts for the speedup
  if (me == 0) {
    gettimeofday(&start, (struct timezone *)0);
  }
  gauss(tab, N, me, tids, nproc);
  pvm_barrier(GRPNAME, nproc);
  if (me == 0) {
    gettimeofday(&end, (struct timezone *)0);
    elapsed = (end.tv_sec - start.tv_sec) +
              (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("computation time: %10.8f sec.\n", elapsed);
  }

  matrix_save(argv[2], tab, N, me, tids, nproc);
  free(tab);
  return 0;
}

/*
// dowork pra testar save e load
int dowork_test(int argc, char **argv, int me, int tids[], int nproc) {
  int N;
  double *tab;


  if (argc != 3) {
    printf("Usage: %s <matrix size> <matrix name>\n", argv[0]);
    exit(-1);
  }

  N = atoi(argv[1]);
  tab = malloc(N * N * sizeof(double));

  matrix_load(argv[2], tab, N, me, tids, nproc);
  pvm_barrier(GRPNAME, nproc);
  matrix_save(argv[2], tab, N, me, tids, nproc);
  free(tab);
  return 0;
}*/


int main(int argc, char **argv) {
  int nproc, mytid, *tids, me, i;

  mytid = pvm_mytid();
  nproc = pvm_siblings(&tids);
  me = pvm_joingroup(GRPNAME);
  pvm_barrier(GRPNAME, nproc);
  pvm_freezegroup(GRPNAME, nproc);
  for (i = 0; i < nproc; i++) {
    tids[i] = pvm_gettid(GRPNAME, i);
  }

  dowork(argc, argv, me, tids, nproc);

  pvm_lvgroup(GRPNAME);
  pvm_exit();
  return 0;
}
