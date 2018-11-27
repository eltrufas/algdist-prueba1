#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

void *Usage(char *argv[]) {
   printf("Usage: %s n\n", argv[0]);
   exit(1);
}

int main(int argc, char *argv[]) {
   int i;
   int n;
   float *s;
   long t0, t1;
   clock_t c0, c1;
   struct timeval timecheck;

   if (argc != 2)
      Usage(argv);
   else {
      n = atoi(argv[1]);
      s = calloc(n, sizeof(float));

      c0 = clock();
      // Este metodo para obtener el tiempo de pared tiene mucha mejor
      // resoluci'on que time
      // https://stackoverflow.com/a/45769714
      gettimeofday(&timecheck, 0);
      t0 = (long)timecheck.tv_sec * 1000 + (long)timecheck.tv_usec / 1000;

      for (i = 1; i <= n; i++) {
        s[i] = sqrt(i);
      }

      gettimeofday(&timecheck, 0);
      t1 = (long)timecheck.tv_sec * 1000 + (long)timecheck.tv_usec / 1000;
      c1 = clock();

//      printf("\n\n**************************************\n\n");
      for (i = 1; i <= n; i++) {
//            printf("%3d - %f\n",i,s[i]);

      }

      printf ("wall time: %f\n", ((double)(t1 - t0)) / 1000);
      printf ("cpu time: %f\n", (float) (c1 - c0)/CLOCKS_PER_SEC);
   }
   return 0;
}
