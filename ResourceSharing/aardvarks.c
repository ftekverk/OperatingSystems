// Name : Finn Tekverk
// Program : Resource sharing. A modification on the "dining philosophers" problem,
//            with anteaters sharing anthills.
// Purpose : Written for Operating Systems class assingment 




#include "/comp/111/assignments/aardvarks/anthills.h" 
#include <semaphore.h>

#define TRUE 1
#define FALSE 0

int initialized=FALSE; // semaphores and mutexes are not initialized 

//Shared data among threads
    //  # of ants
    //  anthills
    //  Timing info

double start_Times[AARDVARKS_PER_HILL * ANTHILLS];



//keep track of ants in each anthill
int antsLeft[ANTHILLS];

// define your mutexes and semaphores here 
// they must be global variables. 

// *** SEMAPHORES *** //
//We want a semaphore for each locking operation.
sem_t anthillSemaphores[3];


// *** MUTEX *** //
//We want a mutex for every anthill so only one anteater accesses
//the count of ants at a time
pthread_mutex_t anthillMutexes[ANTHILLS];

//Function to create 1 second delay
void timeDelay(){
    double start_time = elapsed();
    while( elapsed() - start_time < 1){
        //nothing. wait for a second
    }
    //after a second we are done with the semaphore

}


// first thread initializes mutexes 
void *aardvark(void *input) { 
    char aname = *(char *)input; 
    // first caller needs to initialize the mutexes!
    pthread_mutex_lock(&init_lock); 
    if (!initialized++) { // this succeeds only for one thread
        // initialize your mutexes and semaphores here
        for(int i = 0; i<3; i++){
            //semaphore initialize
            int semReturn = sem_init(&anthillSemaphores[i], 0, AARDVARKS_PER_HILL);
            if(semReturn != 0) printf("Error in Initializing Semaphore\n");

            for(int i = 0; i<3; i++){
                sem_trywait(&anthillSemaphores[0]);
                sem_trywait(&anthillSemaphores[1]);
                sem_trywait(&anthillSemaphores[2]);
            }

            //mutex init
            int mutexReturn = pthread_mutex_init(&anthillMutexes[i], NULL);
            if(mutexReturn != 0) printf("Error in Initializing Mutex\n");

            //also initialize how many ants are in each anthill -- used for scheduling
            antsLeft[i] = ANTS_PER_HILL;
        }
        //initialize the start times so elapsed - start time > 1
        for(int i = 0; i< ANTHILLS * AARDVARKS_PER_HILL; i++ ){
            start_Times[i] = -100;
        }
    } 
    pthread_mutex_unlock(&init_lock); 

    // now be an aardvark
    while (chow_time()) { 
        //only one anteater should check a hill at a time
        pthread_mutex_lock(&anthillMutexes[0]);
        //check one of the hills randomly
        int hill_to_check = rand() % ANTHILLS;
        //choose anthill that has most ants
        // int maxAnts = -1;
        // for(int i = 0; i < 3; i++){
        //     if(antsLeft[i] > maxAnts && antsLeft[i] > 0){
        //         maxAnts = antsLeft[i];
        //         hill_to_check = i;
        //     }
        // }


        //check the 3 indices that correspond to anthill we chose
        for(int i = ANTHILLS * hill_to_check; i < (ANTHILLS * hill_to_check) + AARDVARKS_PER_HILL; i++ ){
            //check if any semaphores have been claimed for more than a second
            if(elapsed() - start_Times[i] > 1.05){
                //free the semaphor
                sem_post(&anthillSemaphores[hill_to_check]);
                start_Times[i] = elapsed();
                break;
            }
        }

        //claim the anthill spot if one was available
        if(sem_trywait(&anthillSemaphores[hill_to_check]) != -1){
            pthread_mutex_unlock(&anthillMutexes[0]);

//SLURPING
            pthread_mutex_lock(&anthillMutexes[1]);
            //check ants left in the anthill
            if(antsLeft[hill_to_check] > 0){
                //try to slurp
                antsLeft[hill_to_check]--;
                // start_Times[anthillSpot] = elapsed();
                pthread_mutex_unlock(&anthillMutexes[1]);
                int slurpValue = slurp(aname, hill_to_check); // identify self, slurp from first anthill
                //if the slurp succeeded, update the number of ants
                // if(slurpValue == 1){
                //     pthread_mutex_lock(&anthillMutexes[2]);
                //     antsLeft[hill_to_check]--;
                //     // printf("Ants in hill %i: %i\n",hill_to_check, antsLeft[hill_to_check]);
                //     pthread_mutex_unlock(&anthillMutexes[2]);
                // }
            }
            else{
                pthread_mutex_unlock(&anthillMutexes[1]);
            }
//SLURPING
        
        }
        else{ //make sure to unlock if we don't get the anthill
            pthread_mutex_unlock(&anthillMutexes[0]);
        }
    }

    //

    return NULL; 
} 
