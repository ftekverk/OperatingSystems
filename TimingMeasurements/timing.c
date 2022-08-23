// Name : Finn Tekverk
// Program : Create a program that measures the elapsed time of several operations.
//              (Mutexes, file operations, etc.)
// Purpose : Written for Operating Systems class assingment 

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h> 


#include <pthread.h>


int main(){
    //rusage
    struct rusage startTime1, endTime1;
    int size = 4096;
    float user_loop_time;
    float sys_loop_time;

    //calculate time of running a for loop with Loop_iterations iterations



    /*   ------------- SECTION : ALLOCATE WITH MMAP microseconds -------------  */
    //measure empty loop
    int repetitions = 1000000;
    getrusage(RUSAGE_SELF, &startTime1);
    for(int i = 0; i < repetitions; i++){}
    getrusage(RUSAGE_SELF, &endTime1);
    user_loop_time = calculateUserTimeDiff(&startTime1, &endTime1);
    sys_loop_time = calculateSystemTimeDiff(&startTime1, &endTime1);

    getrusage(RUSAGE_SELF, &startTime1);
    for(int i = 0; i < repetitions; i++){
        int *pagePointer = mmap(NULL, size, PROT_READ, MAP_ANONYMOUS|MAP_PRIVATE,  -1, 0);
    }
    getrusage(RUSAGE_SELF, &endTime1);

    printf("--- Average time to allocate one page of memory (microseconds)--- \n");
    printf("User Time Avg. : %f \n" , (calculateUserTimeDiff(&startTime1, &endTime1) - user_loop_time) / repetitions );
    printf("System Time Avg. : %f \n" , (calculateSystemTimeDiff(&startTime1, &endTime1) - sys_loop_time) / repetitions );
    


    /*   ------------- SECTION : LOCK WITH pthread_mutex_lock() microseconds -------------  */
    int thread_repetitions = 200000;
    //measure empty loop
    getrusage(RUSAGE_SELF, &startTime1);
    for(int i = 0; i < thread_repetitions; i++){}
    getrusage(RUSAGE_SELF, &endTime1);
    user_loop_time = calculateUserTimeDiff(&startTime1, &endTime1);
    sys_loop_time = calculateSystemTimeDiff(&startTime1, &endTime1);
    
    //initialize many mutexes
    pthread_mutex_t sample_mutex[thread_repetitions];
    for(int i =0; i< thread_repetitions; i++){
        pthread_mutex_init(&sample_mutex[i], NULL);
    }
    
    //measure and calculate average time
    getrusage(RUSAGE_SELF, &startTime1);
    for(int i = 0; i < thread_repetitions; i++){
        pthread_mutex_lock(&sample_mutex[i]);
    }
    getrusage(RUSAGE_SELF, &endTime1);

    printf("--- Average time to Close a Thread (microseconds)--- \n");
    printf("User Time Avg. : %f \n" , (calculateUserTimeDiff(&startTime1, &endTime1) - user_loop_time) / thread_repetitions );
    printf("System Time Avg. : %f \n" , (calculateSystemTimeDiff(&startTime1, &endTime1) - sys_loop_time) / thread_repetitions );
    



    /*   ------------- SECTION : Bypass Cache USING TMP microseconds -------------  */
    struct timeval startTime2, endTime2;
    int readWriteRepetitions = 10000;
    int byte_count = 4096;
   
    //measure the wall time for an empty loop
    float wall_loop_time;
    gettimeofday(&startTime2, NULL);
    for(int i = 0; i< readWriteRepetitions; i++){}
    gettimeofday(&endTime2, NULL);
    wall_loop_time = calculateWallTimeDiff(&startTime2, &endTime2);


    // 2.) WRITE using TMP

    //make a temp file
    int fileDes;
    char *filename = "/tmp/ftekvetest20.txt";
    fileDes = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT | O_SYNC);
    if(fileDes == -1){
        printf("Failed to make a new file trying to write\n");
        exit(EXIT_FAILURE);
    }

    //Average time to write 4096 Bytes directly to /tmp

    char charArray[byte_count];
    for(int i=0; i< byte_count; i++){
        charArray[i] = 'i';
    }

    //initialize a buffer
    long unsigned int writeBuffer = (((unsigned long) charArray + byte_count - 1) / byte_count) * byte_count;
    
    gettimeofday(&startTime2, NULL);
    for(int i=0; i<readWriteRepetitions; i++){
        write(fileDes, (void *) writeBuffer, byte_count);
    }
    gettimeofday(&endTime2, NULL);

    printf("--- Average Time For Bypassing Cache to TMP (microseconds)--- \n");
    printf("Average Time to write: %f\n", (calculateWallTimeDiff(&startTime2, &endTime2) - wall_loop_time)/readWriteRepetitions);
    
    close(fileDes);
    unlink(filename);



    // 2.) READ from TMP

    // measure time for empty loop
    float wall_loop_time2;
    gettimeofday(&startTime2, NULL);
    for(int i = 0; i< readWriteRepetitions; i++){}
    gettimeofday(&endTime2, NULL);
    wall_loop_time2 = calculateWallTimeDiff(&startTime2, &endTime2);

    //make a temp file
    int fileDes2;
    char *filename2 = "/tmp/ftekvetest27.txt";
    fileDes2= open(filename2, O_RDWR | O_CREAT | O_TRUNC | O_DIRECT | O_SYNC);
    if(fileDes2 == -1){
        printf("Failed to make a new file trying to read\n");
        exit(EXIT_FAILURE);
    }

    //create and initialize an array
    char charArray2[byte_count * 2];
    for(int i=0; i<byte_count * 2; i++){
        charArray2[i] = 'i';
    }

    // long writeBuffer2 = (((unsigned long) charArray + byte_count - 1) / byte_count) * byte_count;
    for(int i=0; i<readWriteRepetitions; i++){
        write(fileDes2, (void *) writeBuffer, byte_count);
    }

    lseek(fileDes2, 0, SEEK_SET);
    //initialize a buffer
    long readBuffer = (((unsigned long) charArray2 + byte_count - 1) / byte_count) * byte_count;

    gettimeofday(&startTime2, NULL);
    for(int i=0; i<readWriteRepetitions; i++){
        read(fileDes2, (void *) readBuffer, byte_count);
    }
    gettimeofday(&endTime2, NULL);
    printf("Average time to read: %f\n", (calculateWallTimeDiff(&startTime2, &endTime2) - wall_loop_time2)/readWriteRepetitions);

    close(fileDes2);
    unlink(filename2);


    /*   ------------- SECTION : WRITING AND READING FROM CACHE microseconds -------------  */
    //Repeat without bypass flags
    //make a temp file
    int filename3 = "/tmp/ftekvetest21.txt";
    char fileDes3 = open(filename3, O_RDWR | O_CREAT | O_TRUNC);
    if(fileDes3 == -1){
        printf("Failed to make a new file writing to cache\n");
        exit(EXIT_FAILURE);
    }

    //time an empty loop
     //measure the wall time for an empty loop
    gettimeofday(&startTime2, NULL);
    for(int i = 0; i< readWriteRepetitions; i++){}
    gettimeofday(&endTime2, NULL);
    wall_loop_time2 = calculateWallTimeDiff(&startTime2, &endTime2);

    //Average time to write 4096 Bytes directly to /tmp
    gettimeofday(&startTime2, NULL);
    for(int i=0; i<readWriteRepetitions; i++){
        write(fileDes3, charArray, byte_count);
    }
    gettimeofday(&endTime2, NULL);
    printf("--- Average Time For Cache Op (microseconds)--- \n");
    printf("Average Time to write: %f\n", (calculateWallTimeDiff(&startTime2, &endTime2) - wall_loop_time2)/readWriteRepetitions);




    // Average time to read from Cache
    //Read the characters from file
    char c;
    char writeToThisArray[byte_count];
    gettimeofday(&startTime2, NULL);
    for(int i=0; i<readWriteRepetitions; i++){
        read(fileDes, writeToThisArray, byte_count);
    }
    gettimeofday(&endTime2, NULL);
    printf("Average time to read: %f\n", (calculateWallTimeDiff(&startTime2, &endTime2) - wall_loop_time2)/readWriteRepetitions);

    close(fileDes3);
    unlink(filename3);
        
    return 0;
}


int calculateWallTimeDiff(struct timeval *startTime, struct timeval *endTime){
    //Time in seconds
    return 1e6*(endTime->tv_sec - startTime->tv_sec) +
           (endTime->tv_usec - startTime->tv_usec);
}

int calculateSystemTimeDiff(struct rusage *startTime, struct rusage *endTime){
    //Time in seconds
    return 1e6*(endTime->ru_stime.tv_sec - startTime->ru_stime.tv_sec) +
           (endTime->ru_stime.tv_usec - startTime->ru_stime.tv_usec);
}

int calculateUserTimeDiff(struct rusage *startTime, struct rusage *endTime){
    //Time in seconds
    return 1e6*(endTime->ru_utime.tv_sec - startTime->ru_utime.tv_sec) +
           (endTime->ru_utime.tv_usec - startTime->ru_utime.tv_usec);
}
