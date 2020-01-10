#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

typedef enum Bool{FALSE, TRUE} bool_t;

struct hdr{
    size_t data_size;
    struct block *next;
    bool_t isFree;
};

struct block{
    struct hdr header;
    uintptr_t *data;
};

struct block *start;
/*Used for a condition to check if a relative between opening and size to be inserted to ll*/
#define OPENING_DIFF_SIZE 100 



/*======================Helper functions for malloc============================*/
/* Objective: checks the linked list for any open spot
    returns: NULL if start is empty | 
             no other empty spots | 
             if the size the existed was to small
    returns: block address at which is free
*/
void *check_open_spots(size_t size){
    if(start == NULL){
        return NULL;
    }
    struct block *next = start->header.next, *prev = start;
    size_t diff;
    while(next != NULL){
        /*checking until go to end of list if there is an openening in the list*/
        /*Thiis checks if blk is free && differnce is smaller than amount*/
        if(prev->header.isFree && (diff=prev->header.data_size - size) < OPENING_DIFF_SIZE  && diff > 0){
            return prev;
        }
        prev = next;
        next = next->header.next;
    }
    return NULL;
}
/* Objective: to add a new_blk onto the end of the linked list
    returns: 1 if nothing is linked
    returns: 0 if it appended new_blk onto the list

    HELPER FUNCTION for set_blk if isNEWSPOTt
*/
int append_new(struct block *new_blk){
    // Case 1 - where start doesnt have any items
    if(start == NULL){
        return 1;
    }
    struct block *next=start->header.next,*prev = start;
    // Case 2 - where start has at least one item
    next = start->header.next, prev = start;
    while(next != NULL){
        prev = next;
        next=next->header.next;
    }
    prev->header.next = new_blk;
    return 0;
}


void *set_blk(void *start_ptr, size_t size, bool_t isNewSpot){
    // step 1 - set fields of newly allocated space
    struct block *block = start_ptr;
    block->data = sizeof(struct hdr) + start_ptr;
    /*Account if there is an opening opening st. block.size > size*/
    if(isNewSpot){
        block->header.data_size = size;
    }
    block->header.isFree = FALSE;
    block->header.next = NULL;
    // step 2 - link this to the start
    //Case 1 - if its for appending
   if(isNewSpot){
    if(append_new(block)){
        start = block;
    }
   }
    return block->data;
}

/*===============================================================*/






/* Requirements:
    1.) Returns NULL, if cant allocate more space using Sbrk; also setting errno to ENOMEM
    2.) Returns address of starting data section
*/
void *malloc(size_t size){

    /* Case 1 - to check the linked list see if any 
                spots are avail with size capacity*/
    void *free_spot, *new_spot;
    if((free_spot = check_open_spots(size))){
        // Store in that spot
        return set_blk(free_spot, size, FALSE);
    }else{
    /* Case 2 - if no spaces are availble 
                in the linked list;*/
        if((new_spot  = sbrk(size + sizeof(struct hdr))) == NULL){
            /* Case 2.1 - if we cant get any more space from os*/
            return NULL;
        }else{
            // Case 2.2 - sbrk gave us our space
            return set_blk(new_spot, size, TRUE);
        }
    }
}




#define GIVE_UP_SPACE 10000 /*in bytes*/

/* TODO: see if I need to change the contents of ptr (maybe i can use **ptr_adr = &ptr)
/**/

/* Requirements:
    0.) Try to give up space if a lot is free sbrk(negative value)
*/
void free(void *ptr){ //*ptr points to the data section
    void * start_block, *next_block;
    if(ptr){
        // step 1 - free the pointer
        start_block = ptr-sizeof(struct hdr);
        ptr = NULL;
        struct block *blk = start_block, *next_blk;
        blk->header.isFree = TRUE;
        // step 2 - see if you can merge with adjacent

        // Case 1 - see if next is null if so you can use sbrk with negative to take off
        if((next_blk = blk->header.next) == NULL){
            // step 4 - check if this is a lot of data_size (free to srbk)
            if(blk->header.data_size > GIVE_UP_SPACE){
                sbrk((blk->header.data_size)*-1);
            }
        
        // Case 2 - blk isnt at the end (therefore we cant give os back data)
        }else{
            /* step 5: implied that next isnt null so check if next_blk is Free */
            if(next_blk->header.isFree){
                /*If isFree merge blk and next_blk then*/ 
                size_t next_blk_size = next_blk->header.data_size + sizeof(struct hdr);
                next_block = next_blk;
                memset(next_block,0,next_blk_size); //clear out any data saved (ereasing it)
                blk->header.data_size +=next_blk_size;
            }
        }
    }else{
        fputs("Cant free 0x0", stderr);
    }
}

/* Requirements:
    0.) Try to merge adjacent spots if possible
    1.) Returns NULL, if cant allocate more space using Sbrk; also setting errno to ENOMEM;
        keeping already reserved buffer.
    2.) Returns address of starting data section

    Special Cases:
        realloc(NULL, size) = malloc(size)
        realloc(ptr, 0) = free(ptr)
*/
void realloc(void *ptr, size_t size);


;

int main(){


    int *ptr1 = (int*)malloc(100);
    int *ptr2 = (int*)malloc(100);
    int *ptr3 = (int*)malloc(21);
    *ptr1 = 1;
    *ptr2 = 2;
    *ptr3 = 3;
    //free(ptr1);
    free(ptr2);
    int *ptr4 = (int*)malloc(21);
    *ptr4 = 4;


    return 0;
}
