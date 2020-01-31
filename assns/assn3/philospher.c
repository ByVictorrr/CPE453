/* Objective: implement dinning philospher problem 
    Philospher's states:
        1.)eating
        2.)thinking
        3.)changing - middle between eating and thinking
    

    Requirments:
        1.) Parmertize number of philosphers
        2.) Use POSIX Semaphoresto control indiv forks
        3.) Avoid deadlock, and neighbor phil never eating same time
        4.) Non-adj pilosers to eat at same time
        5.) Philosphers stay over their food for a random time
        6.) Each time philospher changes sate, print a status line 
            Format: "Eat", "Think", ""- for changing with forks held by philospher
            State changes include:
                – changing among “eat”, “think” and “transition”
                – picking up a fork
                – setting down a fork
        7.) optional cmd-line arg (int) indicating how many times,
            each philospher should go through his or her eat-think cycle before exiting
        8.) Number forks (0,1,2,3,4)
                    philosers(A,B,C,D, E)
                    sit pos (.5, 1.5, 2.5,..,4.5)

        9.) Binary semaphore around the state printing and updating to ensure that 
            prints a consistent statw
        10.) 


*/


#define NUM_PHILOSPHOPHERS 5
typedef enum STATES{EATING, THINKING, CHANGING} state_t;
#include <stdio.h>
#include <string.h>
#include <pthread.h>

int get_cycles(int argc, char *cycle);
pthread_t *get_thread_by_label(char ascii, pthread_t *threads);


/*usage:  ./philospher [cycles]*/
int main(int argc, char *argv[]){
    int cycles, i;

    pthread_t *threads;
    if((cycles = get_cycles(argc, argv[1])) == -1){   
        fprintf(stderr, "%s", "Usage: ./philospher [Cycles]\n") /*flush stderr*/
        return;
    }
    /*Cycles gonna be 0(no parm) or greater than zero(yes parm) */

    if(!(threads = (pthread_t*)malloc(sizeof(pthread_t)* NUM_PHILOSPHOPHERS)){
        exit(EXIT_FAILURE);
    }
    for(i=0; i< NUM_PHILOSPHOPHERS; i++)
        pthread_create(&threads[i], NULL, NULL, NULL);


    // Initally they all start out starving (trying to go in eat state before)

    return 0;
}
/* Description: helper to get optional cycles if it exist
        
        Returns: {0 if no parm}
                 {cycle if correct in put}
                  { -1 cycles if invalid input}
*/
int get_cycles(int argc, char *cycle){
    int cycles;
    if (argc > 2){
        return -1;
    }else if (cycle == NULL){
        return 0;
    }else if(argc == 2 && !(cycles=atoi(cycle))){
        /*not a valid input*/
        return -1;
    }
    return cycles;
}
/* Description: returns a thread given a label associated with it*/
pthread_t *get_thread_by_label(char ascii, pthread_t *threads){
    if(ascii < NUM_PHILOSPHOPHERS + 'a'){
        return &threads[ascii-'a'];
    }
    return NULL;
}

