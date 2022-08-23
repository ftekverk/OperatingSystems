// Name : Finn Tekverk
// Program : Create a program that simulates a terminal (shell).
// Purpose : Written for a class assingment


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>


int main(int argc, char *argv[]){

    char ***commandsArray [100];   
    char *string, *command;
    size_t size = 200 * sizeof(char); //allows our user input to be only 200 chars
    string = (char*) malloc (size); //allocate on the heap
    char **string_pointer = &string;

    char exitString[] = "exit\n";
    char **index;

    //array that has user input as it is typed, separating chunks by | character
    char **userInput = malloc(size);

    //array that separates each string of the array
    // char ***commandsArray = malloc(size);

    //array that holds pointers to commands arrays
    // char ***pipeCommandsArray = malloc(size);

    for(int i = 0; i < 100; i++){
        commandsArray[i] = malloc(100 * sizeof(char *));
    }


    //User input loop
    while(1){
        printf("jsh$ ");
        //error check getline
        if(getline(string_pointer, &size, stdin) < 0){
            fprintf(stderr, "jsh error: Error Getting Line \n");
        }

        //error check getline
        if(string_pointer == NULL){
            printf("No Input Provided\n");
        }

        //Checks our input
            //if user has not typed "exit"
            if( strcmp(string, exitString ) != 0){
                int i = 0;  
                int pipeCount = 0;
                
                //read for pipe characters
                while((command = strsep(&string,"|")) != NULL ){
                    // printf("here");
                    if(*command == '\0') continue;
                    userInput[pipeCount] = malloc(size);
                    strcpy(userInput[pipeCount], command);
                    pipeCount++;
                    
                }
                removeEndLine(userInput, pipeCount);

                // for each pipe section, separate by spaces and store in new array
                //allow for 25 pipes
                int pipeArr[50];
                for(int x = 0; x < pipeCount; x++){
                    //making pipes. One less than the amount of code segments
                    if(x < pipeCount - 1){
                        int pipe_return = pipe(pipeArr + x*2);
                        if(pipe_return < 0) printf("An error concerning pipes occured. \n");
                    }
                    //parse our input, separating by spaces this time
                    int i = 0;
                    while( (command = strsep(&userInput[x], " ")) != NULL ){
                        //ignore extra spaces
                        if(*command == '\0') continue;
                        // if(empty_check(command) != 0) break;
                        commandsArray[x][i] = malloc(size);
                        strcpy(commandsArray[x][i], command);
                        i++;
                    }
                    commandsArray[x][i] = NULL;

                    // printf(commandsArray[x][0]);

                    // pipeCommandsArray[x] = &commandsArray;
                    // printf("%i : \n", i);

                }
                //call pipe command ( After for loop )
                if(pipeCount > 1){
                    pipe_forking_function(commandsArray, pipeCount, pipeArr);
                }
                else{
                    forking_function(commandsArray[0]);
                }

                //clean memory
                for(int j = 0; j < i; j++){
                    freeMemory(commandsArray, i);
                    free(string);
                    free(command);
                }
                
            }
            //If user has typed "exit"
            else{  
                exit(0);


            }
    }
}


//Forking fuction without pipes. Make a child to execute the proccess
//and check the exit statuses.
void forking_function(char **commandsArray){
    pid_t rc = fork();
    int status;
    if (rc < 0){
        //fork failed; exit
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (rc == 0){ //child (new process)
        int status_code = execvp(commandsArray[0], commandsArray);
        //error checking execvp
        if(status_code < 0){
            fprintf(stderr, "jsh error: Command not command: %s\n", commandsArray[0]);
        }
        //command not command
        if (errno == 2){
            exit(127);
        }
        //If execvp fails, need to exit with status 127 no matter what
        if(status_code< 0) exit(127);
        exit(0);
    }
    //Original Process
    else{
        int pidwait = waitpid(rc, &status, 0);
        if(WEXITSTATUS(status)){
            printf("jsh status: %d\n", WEXITSTATUS(status)); 
        }
        else{
            printf("jsh status: %d\n", 0);
        }
    }
    return;
}

//If there is piping used, fork, and set up the pipes for each child
//based on the child's position in the original command line input.
void pipe_forking_function(char ***commandsArray, int totalChildren, int *pipeArr){
    // printf("In forking function \n");
    int status;
    int rc;
    int childNum = 0;

    int rc_final;

    //start forking for all the pipes
    while(childNum < totalChildren){
        rc = fork();
        if (rc < 0){
            //fork failed; exit
            fprintf(stderr, "fork failed\n");
            exit(1);
        }
        //child (new process)
        else if (rc == 0){
            // printf("1 %d\n", childNum);
            //first child, only change output
            if(childNum == 0){
                dup2(pipeArr[1], 1);
                close(pipeArr[1]);
            }
            //if last child, change only input
            else if(childNum == totalChildren - 1){
                dup2(pipeArr[2*childNum - 2 ], 0);
                close(pipeArr[2*childNum - 2]);
            }
            //intermediate children 
            else{
                dup2(pipeArr[2*childNum + 1], 1);
                dup2(pipeArr[2*childNum - 2], 0); //get input from prev pipe
                close(pipeArr[2*childNum - 2]);
                close(pipeArr[2*childNum + 1]);
            }
            int status_code = execvp(commandsArray[childNum][0], commandsArray[childNum]);
            //error checking execvp
            if(status_code < 0){
                fprintf(stderr, "jsh error: Command not command: %s\n", commandsArray[childNum][0]);
                exit(127);
            }
        }
        //Original Process
        else{
            if(childNum == 0){
                close(pipeArr[1]);
            }
            //if last child, change only input
            else if(childNum == totalChildren - 1){
                close(pipeArr[2*childNum - 2]);
            }
            //intermediate children 
            else{
                close(pipeArr[2*childNum - 2]);
                close(pipeArr[2*childNum + 1]);
            }
            //wait after closing to run all processes concurrently
            // waitpid(rc, &status, 0);
        }
        childNum++;

        //find last process pid
        if(childNum == totalChildren) {
            rc_final = rc;
        }
    }
    
    //wait specifically on the last forked child process to obtain exit status
    int childStatus;
    waitpid(rc_final, &status, 0);
    childStatus = WEXITSTATUS(status);
    printf("jfs status: %d\n", childStatus);


    return;
}

//called at program end to free most of the memory.
void freeMemory(char **commandsArray, int size){
    for(int i=0; i<size; i++){
        if(commandsArray[i] != NULL){
            free(commandsArray[i]);
        }
    }
}


//Rewrites the last character with an empty character.
//SHOULD ONLY BE CALLED IF THE LAST CHARACTER IS AN ENDLINE
void removeEndLine(char **commandsArray, int i){
    //sets last spot to string terminator
    commandsArray[i-1][strlen(commandsArray[i-1]) - 1] = '\0';
    commandsArray[i] = NULL;
}

