/*
 * par-squares: computes the square roots for numbers between 1 and n using K threads
 *
 * Programmer: Ruben Carvajal Schiaffino
 *
 * Santiago de Chile: 13/11/2013
 *
 */

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>


struct Message {

   int myid, nvalue, numthreads;
   float *data;
};   

/*
 *
 */
void *Computes(void *n) {

   int i, j, size;
   struct Message *m;
   float *s;

   printf("\n\n**************************************\n\n");
   fflush(stdout);
   m = (struct Message *) n;
   printf("From %d Beginning The Task\n\n",m->myid);
   fflush(stdout);
   size = m->nvalue / m->numthreads;
   s = m->data;
   for (i = 0, j = m->myid * size + 1; i < size; i = i + 1, j = j + 1) {
      s[i] = sqrt(j);
      fflush(stdout);
   } 
   printf("From %d Ending The Task\n\n",m->myid);
   fflush(stdout);
   sleep(2 * m->myid);
   for (i = 0; i < size; i++) {
      printf("%d.- %f\n",i,m->data[i]);
      fflush(stdout);
   }   
   pthread_exit((void *) m);
}


/*
 *
 */
void *Usage(char *argv[]) {

   printf("Usage: %s n k\n", argv[0]);
   exit(1);
}


/*
 *
 */
int main(int argc, char *argv[]) {

   pthread_t *thread;
   pthread_attr_t attribute;
   struct Message **m;
   void *exit_status;
   int n, ii, j, k, l, rc;
   float **s;
   long t0, t1;
   clock_t c0, c1;
   struct timeval timecheck;

   if (argc != 3)
      Usage(argv);
   else {
      n = atoi(argv[1]);
      k = atoi(argv[2]);
      thread = calloc(k,sizeof(pthread_t));
      s = calloc(k,sizeof(float *));
      m = calloc(k,sizeof(struct Message *));
      for (ii = 0; ii < k; ii = ii + 1)
         s[ii] = calloc(n / k,sizeof(float));
      for (ii = 0; ii < k; ii = ii + 1)
         m[ii] = calloc(1,sizeof(struct Message));
      pthread_attr_init(&attribute);
      pthread_attr_setdetachstate(&attribute,PTHREAD_CREATE_JOINABLE);

      // Este metodo para obtener el tiempo de pared tiene mucha mejor
      // resoluci'on que time
      // https://stackoverflow.com/a/45769714
      gettimeofday(&timecheck, NULL);
      t0 = (long)timecheck.tv_sec * 1000 + (long)timecheck.tv_usec / 1000;

      for (ii = 0; ii < k; ii = ii + 1) {
         printf("Main: creating thread %d\n", ii);
         m[ii]->myid = ii;
         m[ii]->nvalue = n;
         m[ii]->numthreads = k;
         m[ii]->data = s[ii];
         rc = pthread_create(&thread[ii],&attribute,Computes,(void *) m[ii]);
         printf("**** %d.- rc = %d\n",ii,rc);
      }
      pthread_attr_destroy(&attribute); 
      for (ii = 0; ii < k; ii = ii + 1) {
         rc = pthread_join(thread[ii],&exit_status);
         printf("i = %d rc = %d \n",ii,rc);
         m[ii] = (struct Message *) exit_status;
         printf("%d.- Message Received From %d\n",ii+1,m[ii]->myid);
         s[ii] = m[ii]->data;
         sleep(3);
      }

      gettimeofday(&timecheck, NULL);
      t1 = (long)timecheck.tv_sec * 1000 + (long)timecheck.tv_usec / 1000;
      c1 = clock();

      printf("\n\n**************************************\n\n");
      for (j = 0, l = 1; j < k; j = j + 1)
         for (ii = 0; ii < n / k; ii = ii + 1, l = l + 1) 
            printf("%3d - %f\n",l,s[j][ii]);

      printf ("wall time: %f\n", ((double)(t1 - t0)) / 1000);
      printf ("cpu time: %f\n", (float) (c1 - c0)/CLOCKS_PER_SEC);
   }
   return 0;
}
