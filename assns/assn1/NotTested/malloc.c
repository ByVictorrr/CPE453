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

#define OFFSET sizeof(struct hdr)

struct hdr *start;
void *pending_start, *pending_end; // Helpers so we dont have to call sbrk every time

/*Used for a condition to check if a relative between opening and size to be inserted to ll*/
#define OPENING_DIFF_SIZE 100 


#define NEW_MEM_BLK 64000
#define GIVE_UP_SPACE NEW_MEM_BLK-100 /*in bytes: if a block is free at the end of the list this helps determine if sbrk should give it up*/ 
#define SPLIT_MIN 100 /*used to determine whether a free node can be split into two*/


/*Description: helper function to shrink/grow heap*/
void *safe_sbrk(ssize_t size){
    void *ptr;
    if((ptr=sbrk(size))==NULL){
       /*fputc("giving back memory error", stderr);*/
       return NULL;
    }else{
        return ptr;
    }
}
/*Description: higher level abstraction letting if blk is last*/
bool_t isEndList(struct hdr *blk){
    return !blk->next;
}
/*Description: higher level abstraction saying 
                ok to giving OS more mem*/
bool_t isEnoughToGiveUp(size_t size_free_blk){
    if(size_free_blk +(pending_end-pending_start) > GIVE_UP_SPACE){
        return TRUE;
    }else{
        return FALSE;
    }
}

/*======================Helper functions for malloc============================*/
size_t round_mult16(size_t numBytes){
    bool_t isMult16;
    if(!(numBytes%16)){ /*if number is already mult of 16*/
        return numBytes;
    }else{
        return (numBytes-(numBytes%16))+16;
    }
}
/*Description: implied => org->size > partioned->size (size)
                This is used for partioning and org blk isFree
                Returns: the value at which is partioned
*/
struct hdr *split_hdrs(struct hdr *org, size_t size){
    size_t size_mult_16 = round_mult16(size);
    struct hdr * partioned;
    partioned = org;
    org = partioned+(OFFSET+ size_mult_16);        
    org->data_size = partioned->data_size - (2*sizeof(struct hdr) + size_mult_16);
    org->isFree = TRUE;
    org->data = (uint8_t*)org + sizeof(struct hdr);
    partioned->data_size = size_mult_16;
    /*finally reassign adj next's*/
    org->next=partioned->next;
    partioned->next=org;
    return partioned;
}


/* Objective: checks the linked list for any open spot
    returns values: (1)NULL if start is empty 
                        or if nothing can support size
                    (2) address of free ptr
*/

struct hdr *open_spot(size_t size){
    /*Case 1 - no blks in list*/
    if(start == NULL){
        return NULL;
    }
    struct hdr *next = start->next, 
               *curr=start,*embedded;
    ssize_t diff;

    /* Step 1 - go through ll*/
    while(curr){ /*need to use curr because might be start*/
        /*Case 2 - Check to see if a blk isFree 
                   and it has enough cap to hold new blk*/
        if(curr->isFree && (diff=curr->data_size - size) > 0){
            /*Case 2.1 - split a existing one into two*/
            if(diff > SPLIT_MIN){
                // *assign the space curr needs, then make another blk above it open */
              return split_hdrs(curr,  size);
            /*Case 2.2 - not enough space to split*/
            }else{
                return curr;
            }
        }
        curr=next;
        if(next == NULL){
            break;
        }         
        next=next->next;
    }
    return NULL;
}
/* Objective: to add a new_blk onto the end of the linked list
    returns: 1 if nothing in list
    returns: 0 if it appended new_blk onto the list

    HELPER FUNCTION for set_blk if isNewSpot
*/
void append_new_blk(struct hdr *new_blk){
    // Case 1 - where start doesnt have any items
    if(start == NULL){
        start = new_blk;
    }else{
        struct hdr *next=start->next,*prev = start;
        // Case 2 - where start has at least one item
        next = start->next, prev = start;
        while(next != NULL){
            prev = next;
            next=next->next;
        }
        prev->next = new_blk;
    }
}
uintptr_t *set_blk(struct hdr *start_ptr, size_t size, bool_t isNewSpot){
    // step 1 - set fields of newly allocated space
    struct hdr *block = start_ptr;
    block->data = sizeof(struct hdr) + (void*)start_ptr;
    // Case 1 - is it a new spot?
    if(isNewSpot){
        block->data_size = size;
        block->next = NULL;
        append_new_blk(block);
    }
    block->isFree = FALSE;
    return block->data;
}
size_t abs(ssize_t diff){return diff > 0? diff : -diff;}
/* Objective: to call sbrk as minimal as possible (that is if pend_mem == pend_end, get more mem make call to sbrk)
    return: NULL if srbrk error
    return: new spot address normall
    Assumption: size is mult of 16
*/

struct hdr *update_pending(size_t size){
    void *start_helper;
    size_t diff;
    // Case 1 - if pending_start plus needed size is going to exeeceed pending_end*/
    if(size + OFFSET  >= (diff=pending_end-pending_start)){
        // Case 1.1 - tell use if we should get one more blk or size+OFFSET more mem
        size_t extra_space = size+OFFSET>=NEW_MEM_BLK?(size+OFFSET-diff):NEW_MEM_BLK-(size+OFFSET);
        // Case - where its initally empty
        if(!pending_start && !pending_end){
            start_helper=safe_sbrk(NEW_MEM_BLK);
            pending_start=start_helper+size+OFFSET;
            pending_end=pending_start+NEW_MEM_BLK;
            return start_helper;
        }else{
            safe_sbrk(extra_space);
        }
        // Case 1.1.1 - where the ask size is bigger than 64000
        if(size+OFFSET>=NEW_MEM_BLK){
            /*extra_space = (size+OFFSET) - diff;*/
            //set pend_start==pend_end
            start_helper=pending_start;
            pending_end=pending_start = pending_end+extra_space;
        }else{
            size_t needed=size+OFFSET-diff;
            /*extra_space = NEW_MEM_BLK-(size+OFFSET);*/
            start_helper=pending_start;
            //1. set the pend_start to pend_start=size+OFFSET-diff
            pending_start=pending_end+needed;
            pending_end=pending_end+(NEW_MEM_BLK-needed);
        }

        return start_helper;
    // Case 2 - if we still have space in the pending block
    }else{
        start_helper = pending_start;
        pending_start = pending_start + size + sizeof(struct hdr);
        return start_helper;
    }
}

/* Requirements:
    1.) Returns NULL, if cant allocate more space using Sbrk; also setting errno to ENOMEM
    2.) Returns address of starting data section
*/
void *malloc(size_t size){

    /*Step 1 - round up size to mult of 16*/
    size_t size_mul_16 = round_mult16(size);
    void *free_spot, *new_spot;
    /*Case 1 - there is an open spot*/
    if((free_spot = open_spot(size_mul_16))){
        // Store in that spot
        return set_blk(free_spot, size_mul_16, FALSE);
    }else{
    /* Case 2 - if no spaces in between the linked list are availble or its empty;
    */
        if((new_spot = update_pending(size_mul_16)) == NULL){
            /* Case 2.1 - if we cant get any more space from os*/
            fputc("cant get any more mem from os", stderr);
            return NULL;
        }else{
            // Case 2.2 - sbrk gave us our space
            return set_blk(new_spot, size_mul_16, TRUE);
        }
    }
}

/*===============================================================*/





/*======================Helper functions for free============================*/
/* Description: used for case where the freed one is the 
                last one in the list
*/
struct hdr *get_prev_of_blk_space(struct hdr *blk){
    struct hdr*curr=start,*prev;
    while(curr){
        // Case 1- if is start blk
        if(start == blk){
            return start;
        // Case 2 - curr is that blk
        }else if(curr==blk){
            return prev;
        }
        prev=curr;
        curr=curr->next;
    }
    return NULL;
}
/*Description: will be used to give mem back to os*/
void giveBackToOS(struct hdr *blk){

    struct hdr *prev = get_prev_of_blk_space(blk);
    /*Case 1 - check to see if blk is the start*/
    if(blk == start){
        start=NULL;
    /*Case 2 - that means its at the end but not start*/ 
    }else{
        prev->next=NULL; // for setting the prev to end of list
    }
    safe_sbrk(-(blk->data_size+OFFSET+pending_end-pending_start));
    pending_start=pending_start=blk;
    
}
/*Description: if there are any consequtive 
                free blocks this will merge them 
        returns: 1 - indicates that we have gave mem back to os
                 0 - indicates we have not given mem back to os
                */
int merge_adj_open_blks(){
    /*go through the start and look merge the adj free*/
    struct hdr *curr = start->next, *prev = start, *temp;
    while(curr){
        /* Case 1 - indicate whether we should merge*/
        if(prev->isFree == TRUE && curr->isFree == TRUE){
            // Step 1 - change size
            prev->data_size += curr->data_size + sizeof(struct hdr);
            // Step 2 - merge adj ones
            temp = curr->next; // get copy before changin
            if(isEndList(curr) && isEnoughToGiveUp(prev->data_size)){
                giveBackToOS(prev);
                return 1;
            }
            prev->next = curr->next;
            curr->next = NULL;
        }else{
            temp = curr->next;
        }
        prev = curr;
        curr = temp;
    }
    return 0;
}

bool_t inHeap(void *ptr){
    struct hdr *temp = start;
    while(temp){
        if(temp->data == ptr){
            return TRUE;
        }else{
            temp=temp->next;
        }
    }
    return FALSE;
}

void free(void *ptr){ //*ptr points to the data section
    uint8_t * start_block, *next_block;
    // Case 1 - if its not NULL && in heap
    if(ptr && inHeap(ptr)){
        // Step 1.1 - get actual starting block
        start_block = ((uint8_t*)ptr)-OFFSET;
        struct hdr *blk = start_block, *next_open_blk, *prev;
        size_t pending_diff = pending_end - pending_start;
        // Step 1.2 - call garbage collector so it can merge before deciding isEndList
        blk->isFree = TRUE;
        /* Case 1.1 : if blk is at the end and has no merged(or else it would have
         checked to give back to os)
         */
        if(!merge_adj_open_blks() && isEndList(blk) && isEnoughToGiveUp(blk->data_size)){
            /*!merge_adj_open_blk() - implies that the blk wasnt merged*/
            giveBackToOS(blk);
        }
   }else{
        fputs("Cant free ptr", stderr);
    }
}




/*========================Realloc helpers====================================*/

/* Requirements:
    0.) Try to merge adjacent spots if possible
    1.) Returns NULL, if cant allocate more space using Sbrk; also setting errno to ENOMEM;
        keeping already reserved buffer.
    2.) Returns address of starting data section

    Special Cases:
        realloc(NULL, size) = malloc(size)
        realloc(ptr, 0) = free(ptr)
*/
void *realloc(void *ptr, size_t size){
    /*special cases*/
    /*step 1 - merge*/
    struct hdr *not_free_blk, *free_blk;
    if(ptr == NULL | !inHeap((free_blk=ptr-sizeof(struct hdr)))){
        return malloc(size);
    }else if(size == 0){
        free(ptr);
    /*General case: requirements*/
    }else{
        /*step 1 - merge*/
        merge_adj_open_blks();
        /*Step 2- see if there is space after the merge*/
        if(free_blk->data_size >= size){
            /*split the bigger with newly asked*/
            return ((void *)split_hdrs(free_blk, size))+sizeof(struct hdr);
        /*Step 3 - no space*/
        }else{
            free(ptr);
            return malloc(size);
        }
    }
}

/*========================calloc helpers====================================*/
void *calloc(size_t nmemb, size_t size){
    uintptr_t *ptr;
    struct hdr *head;
    head=(struct hdr *)(malloc(size*nmemb) - sizeof(struct hdr));
    for(ptr=head->data; ptr != head->data+head->data_size; ptr+=size){
        *ptr=0;
    }
    return head->data;
}


int main(){
    
    /*TC 1 - free curr then next(is end of list)
      Inital Expected: hdr1->hdr2->hdr3->hdr4
      Output Expected:
                (after free(ptr4)): hdr1->hdr2->hdr3->(free)
                (after free(ptr3)): hdr1->hdr2->NULL
    
    */
    int *ptr1 = (int*)malloc(1600);
    int *ptr2 = (int*)malloc(10000);
    int *ptr3 = (int*)malloc(NEW_MEM_BLK);
    int *ptr4 = (int *)malloc(20);

    *ptr1=1;
    *ptr2=2;
    *ptr3=3;

    free(ptr4);
    free(ptr3);


    return 0;
}


