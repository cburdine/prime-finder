
/*
 * calculates a list of primes
 * using 4 auxilliary threads
 *
 * each thread tests numbers of the
 * form:
 *   12n + 1
 *   12n + 5
 *   12n + 7
 *   12n + 11
 * 
 * flags are:
 * -n [number] specifies a max (default 1000)
 * -q suppress console updates
 *
 * usage: prime-finder (-q) (-n [number]) [file]
*/
#define VERSION "prime-finder v1.0\n"
#define REPOSITORY_URL  "Repository URL: https://github.com/cburdine/prime-finder\n\n"


#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>

struct targs {
    int threadNum;
    int silentMode;
    int numIterations;
    int* primeArr;
    int* primeArrLen;
    int* numThreadsDone;
    int* iteration;
    sem_t* threadSem;
    pthread_mutex_t* iterationLock;
};

void* threadFunc(void* args);

int lessThan(const void* l, const void* r);

void handleInterrupt(int signal);

int shutdown = 0;

int main(int argc, char** argv){
    
    const char* USAGE = "usage: prime-finder [-q] [-n number] output_file\nFor info about this program use the flag '-h'\n";
    
    const char* HELP = 
"\nprime-finder finds all prime numbers from 2 up to a\n\
specified maximum integer value. The prime numbers are\n\
written in plaintext to the indicated file as well as\n\
stdout. To disable printing to stdout, use the -q flag.\n\
Also, if the user terminates the program with SIGINT (^C),\n\
the user will also have the option to dump all discovered\n\
primes to the indicated output file and shutdown safely.\n\
\n";
    
    const char* HELP_EXAMPLE = 
"Example usage:\n\
\tprime-finder -q -n 1000000 output.txt\n\
\n\
This will find all primes less than 1000000.\n\n";

    const int DEFAULT_N = 100;
    
    struct targs threadArgTemplate;
    
    char chopt;
    char* fileOutName;
    int i, max, silent, dumpingPrimes;
    FILE* fileOut;
    pthread_t threads[4];
    struct targs *threadArgs;    
    
    struct sigaction interruptHandler;

    sem_t* threadSems;
    
    max = DEFAULT_N;
    silent = 0;
    dumpingPrimes = 0;

    interruptHandler.sa_handler =  handleInterrupt;
    sigemptyset(&interruptHandler.sa_mask);
    interruptHandler.sa_flags = 0;

    sigaction(SIGINT, &interruptHandler, NULL);
    
    while((chopt = getopt(argc, argv, "hqn:")) > 0){
        switch(chopt){
            case 'h':
                fputs(VERSION, stdout);
                fputs(HELP, stdout);
                fputs(HELP_EXAMPLE, stdout);
                fputs(REPOSITORY_URL, stdout);
                return 0;

            case 'q':
                silent = 1;
                break;

            case 'n':
                if(sscanf(optarg,"%d", &max) <= 0){
                    perror(optarg);
                    fputs(USAGE, stderr);
                    return 1;
                }
                if(max < 1){
                    fputs("Error- n must be greater than 0\n", stderr);
                    fputs(USAGE, stderr);
                    return 1;
                }
                break;

            case '?':
                fputs(USAGE, stderr);
                return 1;
                break;
        }
    }

    if(optind >= argc){
        fputs("Error- must specify output file\n", stderr);
        fputs(USAGE, stderr);
        return 1;
    }
    
    fileOutName = argv[optind];
    fileOut = fopen(fileOutName, "w");
    fprintf(fileOut, "%d\n%d\n", 2, 3);    
    
    threadArgTemplate.threadNum = 0;
    threadArgTemplate.silentMode = silent;
    threadArgTemplate.numIterations = (max + 11)/12;
    
    threadArgTemplate.primeArr = malloc(
        threadArgTemplate.numIterations * 4 * sizeof(int)
    );

    /* 
       initialize primeArr with 5 to prevent divisor
       race conditions on first iteration 
    */
    threadArgTemplate.primeArr[0] = 5;
    
    threadArgTemplate.primeArrLen = malloc(sizeof(int));
    *(threadArgTemplate.primeArrLen) = 1;
    
    threadArgTemplate.numThreadsDone = malloc(sizeof(int));
    *(threadArgTemplate.numThreadsDone) = 0;
    
    threadArgTemplate.iteration = malloc(sizeof(int));
    *(threadArgTemplate.iteration) = 0;   
    
    threadArgTemplate.iterationLock = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(threadArgTemplate.iterationLock, NULL);
    
    threadArgs = malloc(4 * sizeof(struct targs));
    
    threadSems = malloc(sizeof(sem_t) * 4);
    for(i = 0; i < 4; ++i){
        sem_init(&threadSems[i],0, 1);
    } 
    

    for(i = 0; i < 4; ++i){
        memcpy(&threadArgs[i], &threadArgTemplate, sizeof(struct targs));
        threadArgs[i].threadNum = i;
        threadArgs[i].threadSem = &threadSems[i];        
    }

    /* dispatch threads */
    pthread_mutex_lock(threadArgTemplate.iterationLock);
    for(i = 0; i < 4; ++i){
        pthread_create(&threads[i], NULL, threadFunc, &threadArgs[i]);
    }
    pthread_mutex_unlock(threadArgTemplate.iterationLock);
    
    /* maintain the main thread here */
    while(!shutdown && *(threadArgTemplate.iteration) < threadArgTemplate.numIterations){
       
         
        /* advance to next iteration */
        if(*(threadArgTemplate.numThreadsDone) >= 4){
            pthread_mutex_lock(threadArgTemplate.iterationLock);
            
            ++(*(threadArgTemplate.iteration));
            
            *(threadArgTemplate.numThreadsDone) = 0;
            
            for(i = 0; i < 4; ++i){
                sem_post(&threadSems[i]);
            } 
            
            pthread_mutex_unlock(threadArgTemplate.iterationLock);
        }
    }
    
    if(shutdown){
        puts("\nkilling threads and dumping to file...\n");
    }

    /* join threads from children */
    for(i = 0; i < 4; ++i){
        if(shutdown){
            pthread_kill(threads[i], 0);
        } else {
            pthread_join(threads[i], NULL);
        }
    }    
     
    /* sort array of primes */
    qsort((void*)threadArgTemplate.primeArr, *(threadArgTemplate.primeArrLen), sizeof(int), lessThan);

    
    if(!silent){
        printf("--- PRIMES ---\n");
        printf("2\n3\n");
    }
    for(i = 0; i < *(threadArgTemplate.primeArrLen); ++i){
        if(!silent){
            printf("%d\n", threadArgTemplate.primeArr[i]);
        }
        fprintf(fileOut, "%d\n", threadArgTemplate.primeArr[i]);
    }
    
    printf("wrote %d primes to %s\n", *(threadArgTemplate.primeArrLen) + 2, fileOutName); 

    /* cleanup */
    free(threadArgs);

    for(i = 0; i < 4; ++i){
        sem_destroy(threadSems);
    }
    free(threadSems);

    free(threadArgTemplate.primeArr);
    free(threadArgTemplate.primeArrLen);
    free(threadArgTemplate.numThreadsDone);
    free(threadArgTemplate.iteration); 
    pthread_mutex_destroy(threadArgTemplate.iterationLock);
    free(threadArgTemplate.iterationLock);

   fclose(fileOut); 
}

void* threadFunc(void* args){
    struct targs *threadArgs;
    int i, pMax, testNum, isPrime, prevLen;
     
    threadArgs = (struct targs*) args; 
    
    switch(threadArgs->threadNum){
        case 0:
            testNum = 5;
            break;
        case 1:
            testNum = 7;
            break;
        case 2:
            testNum = 11;
            break;
        default:
            testNum = 13;
            break;
    }

    sem_wait(threadArgs->threadSem);
    prevLen = *(threadArgs->primeArrLen);

    while(*(threadArgs->iteration) < threadArgs->numIterations){
        isPrime = 1;
        pMax = (int)(sqrt((double) testNum)) + 12;
        for(i = 0; i < prevLen && threadArgs->primeArr[i] < pMax && isPrime; ++i){
            
            if(testNum % threadArgs->primeArr[i] == 0){
                isPrime = 0;
            }
        }

        pthread_mutex_lock(threadArgs->iterationLock);
        
        if(isPrime){
            prevLen = *(threadArgs->primeArrLen); 
            threadArgs->primeArr[prevLen] = testNum;
            ++prevLen;
            *(threadArgs->primeArrLen) = prevLen; 
        }
       
        ++(*(threadArgs->numThreadsDone));

        pthread_mutex_unlock(threadArgs->iterationLock);
        
        sem_wait(threadArgs->threadSem);
        testNum += 12;
    }
    
    pthread_exit(NULL); 
}

int lessThan(const void* l, const void* r){
    return *((int*) l) - *((int*) r);
}


void handleInterrupt(int signal){
    char ch;
    
    ch = '\0';
    
    if(!shutdown){
        printf("\nReceived signal %d.\n", signal);
        
        while(ch != 'y' && ch != 'n'){
            if(ch != '\0'){
                puts("\ntype 'y' or 'n'.\n\n");
            }
            puts("Dump and shutdown? (y/n)\n");
            scanf("%c", &ch); 
        }

        if(ch == 'y'){
            shutdown = 1;
        }
       
    }
}


