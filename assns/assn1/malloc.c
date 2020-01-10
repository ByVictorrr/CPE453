#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

typedef enum Bool{FALSE, TRUE} bool_t;

struct hdr{
    size_t data_size;
    bool_t isFree;
    uintptr_t *data;
    struct hdr *next;
};

struct hdr *start;
void *pending_mem, *pending_end; // Helpers so we dont have to call sbrk every time

/*Used for a condition to check if a relative between opening and size to be inserted to ll*/
#define OPENING_DIFF_SIZE 100 


#define GIVE_UP_SPACE 10000 /*in bytes: if a block is free at the end of the list this helps determine if sbrk should give it up*/ 

/*======================Helper functions for malloc============================*/
/* Objective: checks the linked list for any open spot
    returns: NULL if start is empty | 
             no other empty spots | 
             if the size the existed was to small
    returns: hdr address at which is free
*/
void *check_open_spots(size_t size){
    if(start == NULL){
        return NULL;
    }
    struct hdr *next = start->next, *prev = start;
    size_t diff;
    while(next != NULL){
        /*checking until go to end of list if there is an openening in the list*/
        /*Thiis checks if blk is free && differnce is smaller than amount*/
        if(prev->isFree && (diff=prev->data_size - size) < OPENING_DIFF_SIZE  && diff > 0){
            return prev;
        }
        prev = next;
        next = next->next;
    }
    return NULL;
}
/* Objective: to add a new_blk onto the end of the linked list
    returns: 1 if nothing is linked
    returns: 0 if it appended new_blk onto the list

    HELPER FUNCTION for set_blk if isNEWSPOTt
*/
int append_new(struct hdr *new_blk){
    // Case 1 - where start doesnt have any items
    if(start == NULL){
        return 1;
    }
    struct hdr *next=start->next,*prev = start;
    // Case 2 - where start has at least one item
    next = start->next, prev = start;
    while(next != NULL){
        prev = next;
        next=next->next;
    }
    prev->next = new_blk;
    return 0;
}


void *set_blk(void *start_ptr, size_t size, bool_t isNewSpot){
    // step 1 - set fields of newly allocated space
    struct hdr *block = start_ptr;
    block->data = sizeof(struct hdr) + start_ptr;
    /*Account if there is an opening opening st. block.size > size*/
    if(isNewSpot){
        block->data_size = size;
        block->next = NULL;
    // step 2 - link this to the start
        if(append_new(block))
            start = block;
    }
    block->isFree = FALSE;
    //Case 1 - if its for appending
    return block->data;
}

size_t round_mult16(size_t numBytes){
    return (numBytes-(numBytes%16))+16;
}

#define NEW_MEM_BLK 64000
/* Objective: to call sbrk as minimal as possible (that is if pend_mem == pend_end, get more mem make call to sbrk)
    return: NULL if srbrk error
    return: new spot address normall
    Assumption: size is mult of 16
*/
void *update_pending(size_t size){
    void *start;
    // Case 1 - if we dont have any more pending space
    if(pending_mem == pending_end | size+pending_mem > pending_end){
        size_t blk_size = size > NEW_MEM_BLK ? (blk_size = size) : (blk_size = NEW_MEM_BLK); //mult of 16 (how much given)
        start = sbrk(blk_size);
        pending_mem=start+size + sizeof(struct hdr); //points to the next open space
        pending_end= start+blk_size; // points to end of pending
        return start;
    // Case 2 - if we still have space in the pending block
    }else{
        start = pending_mem;
        pending_mem = pending_mem + size + sizeof(struct hdr);
        return start;
    }
}


/*===============================================================*/
/* Requirements:
    1.) Returns NULL, if cant allocate more space using Sbrk; also setting errno to ENOMEM
    2.) Returns address of starting data section
*/
void *malloc(size_t size){

    size_t size_mul_16 = round_mult16(size);
    /* Case 1 - to check the linked list see if any 
                spots are avail with size capacity*/
    void *free_spot, *new_spot;
    if((free_spot = check_open_spots(size_mul_16))){
        // Store in that spot
        return set_blk(free_spot, size_mul_16, FALSE);
    }else{
    /* Case 2 - if no spaces in between the linked list are availble or its empty;
    */
        if((new_spot = update_pending(size_mul_16)) == NULL){
            /* Case 2.1 - if we cant get any more space from os*/
            return NULL;
        }else{
            // Case 2.2 - sbrk gave us our space
            return set_blk(new_spot, size_mul_16, TRUE);
        }
    }
}



/* TODO: see if I need to change the contents of ptr (maybe i can use **ptr_adr = &ptr)
/**/

/* Requirements:
    0.) Try to give up space if a lot is free sbrk(negative value)
*/
void free(void *ptr){ //*ptr points to the data section
    char * start_block, *next_block;
    if(ptr){
        // step 1 - free the pointer
        start_block = ((char*)ptr)-sizeof(struct hdr);
        struct hdr *blk = start_block, *next_blk;
        blk->isFree = TRUE;
        // Case 1 - see if next is null if so you can use sbrk with negative to take off
        if((next_blk = blk->next) == NULL){
            // step 4 - check if this is a lot of data_size (free to srbk)
            if(blk->data_size > GIVE_UP_SPACE){
                sbrk((blk->data_size)*-1) == NULL ? fputc("Cant give mem back to os", stderr) : fputc("gave mem back to tos", stdout);
            }
        
        // Case 2 - blk isnt at the end (therefore we cant give os back data)
        }else{
            /* step 5: implied that next isnt null so check if next_blk is Free */
            if(next_blk->isFree){
                /*If isFree merge blk and next_blk then*/ 
                size_t next_blk_size = next_blk->data_size + sizeof(struct hdr);
                next_block = next_blk;
                memset(next_block,0,next_blk_size); //clear out any data saved (ereasing it)
                blk->data_size +=next_blk_size;
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



int main(){


    /*TC 1 - Testing freeing hdr1->hdr2->hdr3 (hdr2)
                            free(hdr2) : hdr1->free->hdr3
             Description: because ptr4 size is 21 and hdr2's was 100 thus(0<diff < OPENING_DIFF_SIZE):
             Expected: malloc(21) : hdr1->hdr4->hdr3
                            

    TC 2 - Testing freeing hdr1->hdr2->hdr3 (hdr2)
                            free(hdr2) : hdr1->free->hdr3
             Description: because ptr4 size is 100 and hdr2's was 22 thus isnt (0<diff < OPENING_DIFF_SIZE):
             Expected: malloc(21) : hdr1->free->hdr3->hdr4

                            
    
    TC 3 - Testing freeing hdr1->hdr2->hdr3 (hdr2)
                            free(hdr2) : hdr1->free->hdr3
             Description: because ptr4 size is 20 and hdr2's was 200 thus (diff > OPENING_DIFF_SIZE):
             Expected: malloc(20) : hdr1->free->hdr3->hdr4

                            
    TC 4 - Testing freeing hdr1->hdr2->hdr3->hdr4
             Description: let hdr4->size = GIVE_UP_SPACE + 20; 
             Expected: free(ptr4) : hdr1->free->hdr3 (not there os took it)

    TC 5 - Testing multiple of 16 sizes
             Description: let hdr4->size = 10
             Expected: size to be 16


    int *ptr1 = (int*)malloc(100);
    int *ptr2 = (int*)malloc(25);
    int *ptr3 = (int*)malloc(21);
    *ptr1 = 1;
    *ptr2 = 2;
    *ptr3 = 3;
    int *ptr4 = (int*)malloc(10);
    *ptr4 = 4;
    free(ptr4);
    */


    return 0;
}
