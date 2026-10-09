
#include <stdlib.h>
#include <stdio.h>
	 
#include <sys/time.h>
#include <time.h>

#include <errno.h>

//neww
#include <string.h>
#include <math.h>

//sibling
#include <sys/types.h>
#include "pvm3.h"

#define GRPNAME "tokenring"


void matrix_load ( char nom[], double *tab, int N , int me, int tids[], int nproc) {
  int k, j;
  int local_count = 0;
  int my_rows = 0;
  int target;
  double *row_buf = NULL;
  FILE *f = NULL;
   
  printf("\n %d  Funcionando\n", me);
	
  if (me == 0) {
	printf("[P0] A tentar abrir o ficheiro: %s\n", nom);
    if ((f = fopen (nom, "r")) == NULL) { 
      perror ("matrix_load : fopen "); 
      pvm_exit();
      exit(-1);
    } 
	 
	printf("[P0] Ficheiro aberto com sucesso!\n");
	  
    row_buf = (double *)malloc(N * sizeof(double));
    if (row_buf == NULL) {
      perror("matrix_load : malloc ");
      pvm_exit();
      exit(-1);
    }

    for (k = 0; k < N; k++) {
		printf("\n outra coisa %d \n", me);
      // Read one line from file
      for (j = 0; j < N; j++) {
        fscanf (f, "%lf", &row_buf[j]);
      }

      target = k % nproc;
      if (target == 0) {
        // Store in local buffer of P0 
        for (j = 0; j < N; j++) {
          *(tab + local_count * N + j) = row_buf[j];
        }
        local_count++;
      } else {
        // Send row to target process
        pvm_initsend(PvmDataDefault);
        pvm_pkdouble(row_buf, N, 1);
        pvm_send(tids[target], 101);
      }
    }

    fclose (f);
    free(row_buf);

  // Worker processes receive their assigned cyclic rows */
  } else {
    /* Calculate expected local rows count */
    for (k = 0; k < N; k++) {
      if (k % nproc == me) {
		printf("\n alguma coisa %d \n", me);  
        my_rows++;
      }
    }

    /* Receive rows sequentially from Process 0 */
    for (k = 0; k < my_rows; k++) {
      pvm_recv(tids[0], 101);
      pvm_upkdouble((tab + k * N), N, 1);
    }
  }
  
}

void matrix_save ( char nom[], double *tab, int N , int me, int tids[], int nproc  ) {
  FILE *f;
  int i,j;

  if (me == 0) {
    if ((f = fopen (nom, "w")) == NULL) { perror ("matrix_save : fopen "); } 
    for (i=0; i<N; i++) {
      for (j=0; j<N; j++) {
        fprintf (f, "%8.2f ", *(tab+i*N+j) );
      }
      fprintf (f, "\n");
    }
    fclose (f);
  }
}

void matrix_display ( double *tab,int  N ) {
  int i,j;

  for (i=0; i<N; i++) {
    for (j=0; j<N; j++) {
      printf ("%8.2f ", *(tab+i*N+j) );
    }
    printf ("\n");
  }

}

void gauss ( double * tab, int N ) {
  int i,j,k;
  double pivot;

  for ( k=0; k<N-1; k++ ){ /* mise a 0 de la col. k */
    /* printf (". "); */
    if ( fabs(*(tab+k+k*N)) <= 1.0e-11 ) {
      printf ("ATTENTION: pivot %d presque nul: %g\n", k, *(tab+k+k*N) );
      exit (-1);
    }
    for ( i=k+1; i<N; i++ ){ /* update lines (k+1) to (n-1) */
      pivot = - *(tab+k+i*N) / *(tab+k+k*N);
      for ( j=k; j<N; j++ ){ /* update elts (k) - (N-1) of line i */
	*(tab+j+i*N) = *(tab+j+i*N) + pivot * *(tab+j+k*N);
      }
      /* *(tab+k+i*N) = 0.0; */
    }
  }
  printf ("\n");
}
/*
int dowork (int argc, char **argv, int me, int tids[], int nproc ) {
  int N, i, j, k;
  double *tab, pivot;
  char nom[255];
  FILE *f;
  struct timeval tv1, tv2;	//for timing
  int duree;
  
  if (argc != 3){
    printf ("Usage: %s <matrix size> <matrix name>\n", argv[0]);
    exit (-1);
  } 
  N = atoi ( argv[1] );
  strcpy(nom, argv[2]);
  if ( (tab=malloc(N*N*sizeof(double))) == NULL ) {
    printf ("Cant malloc %d bytes\n", N*N*sizeof(double));
    exit (-1);
  }
  gettimeofday( &tv1, (struct timezone*)0 );
      matrix_load ( nom , tab, N,  me, tids, nproc  );
  gettimeofday( &tv2, (struct timezone*)0 );
  duree = (tv2.tv_sec - tv1.tv_sec) * 1000000 + tv2.tv_usec - tv1.tv_usec;
  printf ("loading time: %10.8f sec.\n", duree/1000000.0 );

  gettimeofday( &tv1, (struct timezone*)0 );
      //gauss ( tab, N );
  gettimeofday( &tv2, (struct timezone*)0 );
  duree = (tv2.tv_sec - tv1.tv_sec) * 1000000 + tv2.tv_usec - tv1.tv_usec;
  printf ("computation time: %10.8f sec.\n", duree/1000000.0 );

  sprintf ( nom+strlen(nom), ".result" );
  matrix_save ( nom, tab, N ,  me, tids, nproc );
}*/

int dowork (int argc, char **argv, int me, int tids[], int nproc ) {
  int N, i, j, k;
  double *tab, pivot;
  char nom[255];
  FILE *f;
  struct timeval tv1, tv2;	//for timing
  int duree;
  
  if (argc != 3){
    printf ("Usage: %s <matrix size> <matrix name>\n", argv[0]);
    exit (-1);
  } 
  N = atoi ( argv[1] );
  strcpy(nom, argv[2]);
  if ( (tab=malloc(N*N*sizeof(double))) == NULL ) {
    printf ("Cant malloc %d bytes\n", N*N*sizeof(double));
    exit (-1);
  }

  matrix_load (nom , tab, N,  me, tids, nproc  );

  matrix_display (tab, N );
  matrix_save ( nom, tab, N ,  me, tids, nproc );
}

/*  tokenring example using PVM 3.4 
    - uses sibling() to determine the nb of spawned tasks (xpvm and pvm> ok)
    - uses group for token ring communication
*/


int main(int argc, char ** argv) {
    int NPROC = 8;		/* default nb of proc */
    int mytid;                  /* my task id */
    int *tids;                  /* array of task id */
    int me;                     /* my process number */
    int i;

    /* enroll in pvm */
    mytid = pvm_mytid();

    /* determine the size of my sibling list */
    NPROC = pvm_siblings(&tids); 
    /* WARNING: tids are in order of spawning, which is different from
       the task index JOINING the group */

    me = pvm_joingroup( GRPNAME ); /* me: task index in the group */
    pvm_barrier( GRPNAME, NPROC );
    pvm_freezegroup ( GRPNAME, NPROC );
    for ( i = 0; i < NPROC; i++) tids[i] = pvm_gettid ( GRPNAME, i); 

/*--------------------------------------------------------------------------*/
/*           all the tasks are equivalent at that point                     */

     //printf("Main process: %d", me);
	
	 dowork(argc, argv, me, tids, NPROC );

     pvm_lvgroup( GRPNAME );
     pvm_exit();
	
	return 0;
}

/* Simple example passes a token around a ring */
/*
int dowork( int me, int tids[], int nproc ) {
     int token;
     int src, dest;
     int count  = 1;
     int stride = 1;
     int msgtag = 4;

     //Determine neighbors in the ring 
     src = pvm_gettid (GRPNAME, (me - 1 + nproc) % nproc );
     dest= pvm_gettid (GRPNAME, (me + 1) % nproc );
     if( me == 0 ) { 
        token = dest;
        pvm_initsend( PvmDataDefault );
        pvm_pkint( &token, count, stride );
        pvm_send( dest, msgtag );
        pvm_recv( src, msgtag );
     }
     else {
        pvm_recv( src, msgtag );
        pvm_upkint( &token, count, stride );
        pvm_initsend( PvmDataDefault );
        pvm_pkint( &token, count, stride );
        pvm_send( dest, msgtag );
     }
     printf("P%d (%x) token ring done\n", me, pvm_mytid());
	return 0;
}
*/