#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>

#include "chrono.c"

// #define DEBUG 0
#define DEBUG 1
#define LOOP_COUNT 1

#define MAX_TOTAL_ELEMENTS (500*100*100)  // if each float takes 4 bytes
                                            // will have a maximum 500 million elements
                                            // which fits in 2 GB of RAM

int nTotalElements;  // numero total de elementos
               // obtido da linha de comando  

int InputVector[ MAX_TOTAL_ELEMENTS ];   // will NOT use malloc
                                     // for simplicity   
int OutputVector[ MAX_TOTAL_ELEMENTS ];  


void *prefixPartialSum()
{
    // work with my chunck
    
    OutputVector[0] = InputVector[0];
    for( int i=0; i<nTotalElements-1 ; i++ )
        OutputVector[i+1] = InputVector[i+1] + OutputVector[i];   

    return NULL;
}

int main( int argc, char *argv[] )
{
    int i;
    chronometer_t sequencialPrefixTime;
    
    if( argc != 2 ) {
         printf( "usage: %s <nTotalElements>\n" ,
                 argv[0] ); 
         return 0;
    } else {  
         nTotalElements = atoi( argv[1] ); 
         if( nTotalElements > MAX_TOTAL_ELEMENTS ) {  
              printf( "usage: %s <nTotalElements>\n" ,
                 argv[0] );
              printf( "<nTotalElements> must be up to %d\n", MAX_TOTAL_ELEMENTS );
              return 0;
         }     
    }
    
    printf( "prefixando %d elementos\n", nTotalElements );

    // inicializaçoes
    // initialize InputVector
    for( int i=0; i<nTotalElements ; i++ )
        InputVector[i] = (float)i+1;
        
    chrono_reset( &sequencialPrefixTime );

    chrono_start( &sequencialPrefixTime );

    prefixPartialSum(); 

     // Measuring time after threads finished...
    chrono_stop( &sequencialPrefixTime );

    #if DEBUG == 1
        printf("\n");
        for (int i = 0; i < nTotalElements; i++)
            printf("%d ", OutputVector[i]);
        printf("\n");
    #endif

    chrono_reportTime( &sequencialPrefixTime, "sequencialPrefixTime" );
    
    // calcular e imprimir a VAZAO (numero de operacoes/s)
    double total_time_in_seconds = (double) chrono_gettotal( &sequencialPrefixTime ) /
                                      ((double)1000*1000*1000);
    printf( "total_time_in_seconds: %lf s\n", total_time_in_seconds );
                                  
    double OPS = (nTotalElements)/total_time_in_seconds;
    printf( "Throughput: %lf OP/s\n", OPS );
    
    return 0;

}