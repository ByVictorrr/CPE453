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
#define LAST NUM_PHILOSPHOPHERS-1
#define EQUAL_PER_COLUMN 13
typedef enum STATES{EATING, THINKING, CHANGING} state_t;
typedef enum SEMA_STATE{USED, UN_USED} sema_state_t;
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

int get_cycles(int argc, char *cycle);
pthread_t *get_thread_by_label(char ascii, pthread_t *threads);
void phil_life(void *id);

/*========= Global var=============*/
int cycles;/* Every thread has the same amount of cycles (so made global)*/
sem_t forks[NUM_PHILOSPHOPHERS];/*num of forks = num of phils*/
sem_t print_lock; 
/*===============================*/


/* WOULD NEED MUTEXT LOCKS IF KEEPING r_fork, and l_fork and state*/

typedef struct semaphore{
    sem_t *sema;
    sema_state_t flag;
}sema_t;

typedef struct philospher{
    pthread_t thread; /* represents its session*/
    sema_t right, left; /* For altering global fork before done*/
    state_t state; /* state of the philospher*/
}phil_t;

phil_t phils[NUM_PHILOSPHOPHERS];

#define SIZE_SB_HEAD (NUM_PHILOSPHOPHERS*EQUAL_PER_COLUMN)+(NUM_PHILOSPHOPHERS+1)
void build_header(){
    int i,j, per_col = EQUAL_PER_COLUMN;
    char sb_array[3][SIZE_SB_HEAD+1] = {'\0'}, *sb=sb_array[0];
    /* ROW 1 or sb_array[0]*/
    for(j=0; j < NUM_PHILOSPHOPHERS; j++){
        *sb='|';
        sb++;
        for(i=0; i< per_col;i++){
            *sb='=';
            sb++;
        }
    }
    *sb='|';
    /* ROW 2 or sb_array[1]*/
    sb=sb_array[1];
    for(j=0; j < NUM_PHILOSPHOPHERS; j++){
        *sb='|';
        sb++;
        for(i=0; i< per_col;i++){
            if (i == per_col/2){
                *sb = j+'A';
            }else{
                *sb=' ';
            }
            sb++;
        }
    }
    *sb='|';
    /* ROW 3 or sb_array[2]*/
    sb=sb_array[2];
    for(j=0; j < NUM_PHILOSPHOPHERS; j++){
        *sb='|';
        sb++;
        for(i=0; i< per_col;i++){
            *sb='=';
            sb++;
        }
    }
    *sb='|';
    printf("%s\n",sb_array[0]);
    printf("%s\n",sb_array[1]);
    printf("%s\n",sb_array[2]);
    
}

/*usage:  ./philospher [cycles]*/
int main(int argc, char *argv[]){
    int  i;


    if((cycles = get_cycles(argc, argv[1])) == -1){   
        fprintf(stderr, "%s", "Usage: ./philospher [Cycles]\n"); /*flush stderr*/
        return;
    }


    sem_init(&print_lock, 0, 1);
    /* Init the fork*/
    for(i=0; i< NUM_PHILOSPHOPHERS; i++){
        if(sem_init(&forks[i], 0, 1) <= -1){ /*each fork starting*/
            perror('semaphore init problem');
            exit(EXIT_FAILURE);
        }

    }

    build_header();
    /* Create new thread and init phils*/
    for(i=0; i< NUM_PHILOSPHOPHERS; i++){
        int r = (i+1)%NUM_PHILOSPHOPHERS;
        int l = (i)%NUM_PHILOSPHOPHERS;
        phils[i].state=CHANGING;
        phils[i].right.flag = UN_USED;
        phils[i].left.flag = UN_USED;
        /* LAST SEMAPHORE IS DYSLEXIC*/
        if(i == LAST){
            /* TO AVOID DEAD LOCK(last philos takes right first) */
            phils[i].right.sema = &forks[l];
            phils[i].left.sema = &forks[r];
        }else{
            phils[i].right.sema = &forks[r];
            phils[i].left.sema = &forks[l];
        }
        pthread_create(&phils[i].thread, NULL, phil_life, &i);
        pthread_join(phils[i].thread, NULL);
    }

    // Initally they all start out starving (trying to go in eat state before)

    return 0;
}
/*==========SEMAPHORE HELPERS===================*/
/* NOTE : THE LAST SEMAPHORE IS DYSLEXIC
            (its right is the real left and vice versa)*/
void down(sem_t *sema){
    /*alter right to taken that is USED*/
    sem_wait(sema);
}
void up(sem_t *sema){
    /*alter right to taken that is UNUSED*/
    sem_post(sema);
}

char map_fork_to_char(int fork, int id){
    int l, r;
    l=id%NUM_PHILOSPHOPHERS;
    r=(id+1)%NUM_PHILOSPHOPHERS;
    /*TODO : NEED NEW WAY TO FIND IF current phil is using semaphore*/

    if((fork == r && phils[id].right.flag == USED) | (fork == l && phils[id].left.flag==USED)){
        return fork + '0';
    }
    return '-';
}
/* USE print lock(print state and forks used(COME BACK TOO)*/
void print_sema(int id){
    phil_t curr = phils[id];
    const int len = EQUAL_PER_COLUMN;
    int i;
    const char *STATUS[3] = {"Eat", "Think","  "};
                
    char sb[EQUAL_PER_COLUMN+1+1] = {'\0'}, *pSb=sb;

    down(&print_lock);

    sb[0] = '|';
    pSb++;

    /* Setting equals*/
    for(i=0; pSb-sb< len-1; pSb++, i++){
        if(i < NUM_PHILOSPHOPHERS){
            *pSb=map_fork_to_char(i,id); /*STATE OF FORKS THEN AFTER ALL BLANKS*/
        }else{
            /*HERE DO STATE*/
            strcpy(pSb, STATUS[curr.state]);
            pSb=pSb+strlen(STATUS[curr.state]);
            while(pSb-sb < len){
                *pSb=' ';
                pSb++;
            }
        }
    }
    printf("%s\n", sb);
    if(id == LAST){
        /* CHANGE SCHEDULEING HAVE NON ADJ EATING AT SAME TIME*/
        printf("|\n");
    }

    up(&print_lock);
}

void take_forks(int id){
    /* Use fork */ 
    down(phils[id].left.sema);
    phils[id].state = CHANGING;
    phils[id].left.flag = USED;
    print_sema(id);
    down(phils[id].right.sema);// inside down (changes right to used)
    phils[id].state = EATING;
    phils[id].right.flag = USED;
    print_sema(id);
}

void give_forks(int id){
    /* PUT BACK FORKS in the order we picked up*/
    up(phils[id].left.sema);
    phils[id].state = CHANGING;
    phils[id].left.flag = UN_USED;
    print_sema(id);
    up(phils[id].right.sema); /**(phils[id].left)=UN_USED;(inside up)*/
    phils[id].state = THINKING;
    phils[id].right.flag = UN_USED;
    print_sema(id);
}
void dawdle() {
/*
* sleep for a random amount of time between 0 and 999
* milliseconds. This routine is somewhat unreliable, since it
* doesn’t take into account the possiblity that the nanosleep
* could be interrupted for some legitimate reason.
*
* nanosleep() is part of the realtime library and must be linked
* with –lrt
*/
    struct timespec tv;
    int msec = (int)(((double)random() / RAND_MAX) * 1000);
    tv.tv_sec = 0;
    tv.tv_nsec = 1000000 * msec;
    if ( -1 == nanosleep(&tv,NULL) ) {
        perror("nanosleep");
    }
}
void eat(int id){
    /* Randomize*/
    dawdle();
    print_sema(id);
}

void think(int id){
    dawdle();
    print_sema(id);
}

void phil_life(void *_id){
    int id=*((char*)_id);
    int i;

    /* INITAL condition*/
    for(i=0; i< cycles; i++){
        /* Take forks(in function change philospher state)*/
        take_forks(id);
        eat(id);
        /* Put down forks(change philosphers state*/
        give_forks(id);
        think(id);
    }
}

//===========================================================//
/* Description: helper to get optional cycles if it exist
        
        Returns: {1 if no parm}
                 {cycle if correct in put}
                 { -1 cycles if invalid input}
*/
int get_cycles(int argc, char *cycle){
    int cycles;
    if (argc > 2){
        return -1;
    }else if (cycle == NULL){
        return 1;
    }else if(argc == 2 && !(cycles=atoi(cycle))){
        /*not a valid input*/
        return -1;
    }
    return cycles;
}

