#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>

#include "chrono.c"

// #define DEBUG 0
#define DEBUG 1
#define MAX_THREADS 64
#define LOOP_COUNT 1

#define MAX_TOTAL_ELEMENTS (500*1000*1000)  // if each float takes 4 bytes
                                            // will have a maximum 500 million elements
                                            // which fits in 2 GB of RAM

pthread_t Thread[ MAX_THREADS ];
int my_thread_id[ MAX_THREADS ];
int   partialSum[ MAX_THREADS ];   


int nThreads;  // numero efetivo de threads
               // obtido da linha de comando  
int nTotalElements;  // numero total de elementos
               // obtido da linha de comando      
               
int InputVector[ MAX_TOTAL_ELEMENTS ] = {3, 1, 7, 0, 4, 1, 6, 3};   // will NOT use malloc
                                     // for simplicity                              
  
pthread_barrier_t myBarrier;

// Codigo a ser modificado ***********************************

int min( int a, int b )
{
   if( a < b )
      return a;
   else
      return b;
}

void *prefixPartialSum(void *ptr)
{
    int myIndex = *((int *)ptr);
    int nElements = nTotalElements / nThreads;
    
    // assume que temos pelo menos 1 elemento por thhread
    int first = myIndex * nElements;
    int last = min( (myIndex+1) * nElements, nTotalElements ) - 1;

    #if DEBUG == 1
      printf("thread %d here! first=%d last=%d\n", myIndex, first, last );
    #endif
    
    if( myIndex != 0 )
        pthread_barrier_wait(&myBarrier);  

    // AQUI É ONDE A OPERAÇÃO REALMENTE É FEITA VVVVVVVVVV
        
    // work with my chunck
    int myPartialSum = 0;
    for( int i=first; i<=last ; i++ )
        myPartialSum += InputVector[i];
    
    for( int i=first; i<=last-1 ; i++ )
        InputVector[i+1] += InputVector[i];
        
    // store my result 
    partialSum[ myIndex ] = myPartialSum;     
        
    // AQUI É ONDE A OPERAÇÃO REALMENTE É FEITA ^^^^^^^^^^
    
    pthread_barrier_wait(&myBarrier);    
    
    if(myIndex != 0)
        partialSum[myIndex] += partialSum[myIndex-1];

    return NULL;
}

// ******************************************************************

int main( int argc, char *argv[] )
{
    int i;
    chronometer_t parallePrefixTime;
    
    if( argc != 3 ) {
         printf( "usage: %s <nTotalElements> <nThreads>\n" ,
                 argv[0] ); 
         return 0;
    } else {
         nThreads = atoi( argv[2] );
         if( nThreads == 0 ) {
              printf( "usage: %s <nTotalElements> <nThreads>\n" ,
                 argv[0] );
              printf( "<nThreads> can't be 0\n" );
              return 0;
         }     
         if( nThreads > MAX_THREADS ) {  
              printf( "usage: %s <nTotalElements> <nThreads>\n" ,
                 argv[0] );
              printf( "<nThreads> must be less than %d\n", MAX_THREADS );
              return 0;
         }     
         nTotalElements = atoi( argv[1] ); 
         if( nTotalElements > MAX_TOTAL_ELEMENTS ) {  
              printf( "usage: %s <nTotalElements> <nThreads>\n" ,
                 argv[0] );
              printf( "<nTotalElements> must be up to %d\n", MAX_TOTAL_ELEMENTS );
              return 0;
         }     
    }
    
    printf( "will use %d threads to prefix %d total elements\n\n", nThreads, nTotalElements );
    
    // inicializaçoes
    // initialize InputVector
    // for( int i=0; i<nTotalElements ; i++ )
    //     InputVector[i] = (float)i+1;
        
    chrono_reset( &parallePrefixTime );
    
    pthread_barrier_init(&myBarrier, NULL, nThreads);

    // thread 0 será main
    
    my_thread_id[0] = 0;
    for (i=1; i < nThreads; i++) {
      my_thread_id[i] = i;
      pthread_create( &Thread[i], NULL, 
                      prefixPartialSum, &my_thread_id[i]);
    }


    // Medindo tempo SEM criacao das threads
    pthread_barrier_wait(&myBarrier);        // que acontece se isso for omitido ?        
    chrono_start( &parallePrefixTime );

    prefixPartialSum( &my_thread_id[i] ); // main faz papel da thread 0
    // chegando aqui todas as threads sincronizaram, até a 0 (main)
    
    // main faz a reducao da soma global
    float globalSum = 0;
    for( int i=0; i<nThreads ; i++ ) {
        printf( "globalSum = %f\n", globalSum );
        globalSum += partialSum[i];
    }    
        
    // Measuring time after threads finished...
    chrono_stop( &parallePrefixTime );

    // main imprime o resultado global
    printf( "globalSum = %f\n", globalSum );
    
    for (i=1; i < nThreads; i++)
        pthread_join(Thread[i], NULL);   // isso é necessário ?


    pthread_barrier_destroy( &myBarrier );

    chrono_reportTime( &parallePrefixTime, "parallelPrefixTime" );
    
    // calcular e imprimir a VAZAO (numero de operacoes/s)
    double total_time_in_seconds = (double) chrono_gettotal( &parallePrefixTime ) /
                                      ((double)1000*1000*1000);
    printf( "total_time_in_seconds: %lf s\n", total_time_in_seconds );
                                  
    double OPS = (nTotalElements)/total_time_in_seconds;
    printf( "Throughput: %lf OP/s\n", OPS );
    
    return 0;
}
