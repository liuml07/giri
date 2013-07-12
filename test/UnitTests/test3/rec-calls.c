#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#define NUM_THREADS     2


int increament ( int i )
{
  return i + 1;
}

int add ( int a, int b )
{
  return a+b;
}

int rec_fun ( int *a , int *b )
{
  int x, y, z;

  x = *a;
  
  if( *a == 1 )
    return *b;

  z = 1;

  y = rec_fun ( &z, b);
  
  return x+y;   
}

main(int argc, char *argv[])
{
  int x, y;

  x = atoi( argv[1] );
  y = atoi( argv[2] );
  return rec_fun( &x, &y );

  /*
  int x, y, z, w;

  x = 10;

  threadCreator ();

  y = increament(x);
 
  z = increament(y);

  w = add( x, add(y,z) ) ;

  rec-fun();
 
  return w;
  */
}


void *PrintHello(void *threadid)
{
   long tid;
   tid = (long)threadid;
   printf("Hello World! It's me, thread #%ld! pthread_self #%lu!\n", tid, pthread_self());
   pthread_exit(NULL);
}

int threadCreator ()
{
   pthread_t threads[NUM_THREADS];
   int rc;
   long t;

   printf("MAIN thread pthread_self #%lu!\n", pthread_self());

   for(t=0; t<NUM_THREADS; t++){
      printf("In main: creating thread %ld\n", t);
      rc = pthread_create(&threads[t], NULL, PrintHello, (void *)t);
      if (rc){
         printf("ERROR; return code from pthread_create() is %d\n", rc);
         exit(-1);
      }
   }
   pthread_exit(NULL);
}


