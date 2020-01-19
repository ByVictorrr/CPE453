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

// Helpers so we dont have to call sbrk every time
void *pending_start, *pending_end; 

/* flag to let know were in realloc and dont 
    trying to merge realloc ptr
    want to give mem back to os
*/
bool_t os_giveback_flag;

/*in bytes: if a block is free at the end of the list this 
 * helps determine if sbrk should give it up*/ 
#define NEW_MEM_BLK 64000
/*used to determine whether a free node can be split into two*/
#define GIVE_UP_SPACE NEW_MEM_BLK-10 

/* Used to Determine if the free block - size > 100*/
#define SPLIT_MIN OFFSET+16

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

/*========Helper functions for malloc=========*/
/* Rounds up to closest mult of num*/
size_t round_up_mult_of_num(size_t numBytes, unsigned long num){
    bool_t isMult16;

    if(!(numBytes%num) && numBytes!=0){ /*if number is already mult of num*/
        return numBytes;
    }else{
        return (numBytes-(numBytes%num))+num;
    }
}
/*Description: implied => org->size > partioned->size (size)
                This is used for partioning and org blk isFree
                Returns: the value at which is partioned
*/
struct hdr *split_hdrs(struct hdr *org, size_t size){
    struct hdr * partioned;
    partioned = org;
    org = (void*)partioned+(OFFSET+ size);        
    org->data_size = partioned->data_size - 
    (OFFSET+ size);
    org->isFree = TRUE;
    org->data = (void*)org + OFFSET;
    partioned->data_size = size;
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
        if(curr->isFree && (diff=curr->data_size - size) > 0 ){
            /*Case 2.1 - split a existing one into two*/
            if(diff+OFFSET > SPLIT_MIN){
                // *assign the space curr needs, 
				// then make another blk above it open */
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
    block->data = OFFSET + (void*)start_ptr;
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
/* Objective: to call sbrk as minimal as possible 
	(that is if pend_mem == pend_end, get more mem make call to sbrk)
    return: NULL if srbrk error
    return: new spot address normall
    Assumption: size is mult of 16
*/

    
void *update_pending(size_t size){
    void *start_helper, *ptr;
    size_t diff,
    padded = round_up_mult_of_num(size, NEW_MEM_BLK);
    /* Case 1 - if pending_start plus needed 
	 * size is going to exeeceed pending_end*/
    if(size+OFFSET >= (diff=pending_end-pending_start)){
        start_helper = sbrk(padded);
        // Case 1.1 - if inital startup
        if(!pending_start && !pending_end){
            pending_end=start_helper+padded; 
        // Case 1.2 - general case after iC
        }else{
            pending_end= pending_end+padded;
        }
        pending_start = start_helper+size+OFFSET; 
        return start_helper;
    // Case 2 - if we still have space in the pending block
    }else{
        start_helper = pending_start;
        pending_start = pending_start+size+OFFSET;
        return start_helper;
    }
}

/* Requirements:
    1.) Returns NULL, if cant allocate more space using Sbrk; 
	also setting errno to ENOMEM
    2.) Returns address of starting data section
*/
void *malloc(size_t size){

    /*Step 1 - round up size to mult of 16*/
    size_t size_mul_16 = round_up_mult_of_num(size, 16);
    void *free_spot, *new_spot;
    /*Case 1 - there is an open spot*/
    if((free_spot = open_spot(size_mul_16))){
        // Store in that spot
        return set_blk(free_spot, size_mul_16, FALSE);
    }else{
    /* Case 2 - if no spaces in between the linked 
	 * list are availble or its empty;*/
        if((new_spot = update_pending(size_mul_16)) == NULL){
            /* Case 2.1 - if we cant get any more space from os*/
            return NULL;
        }else{
            // Case 2.2 - sbrk gave us our space
            return set_blk(new_spot, size_mul_16, TRUE);
        }
    }
}
/*==============================================*/





/*==========Helper functions for free============*/
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

    void *ptr;
    struct hdr *prev = get_prev_of_blk_space(blk);
    size_t space_give_back;
    /*Case 1 - check to see if blk is the start*/
    if(blk == start){
        start=NULL;
    /*Case 2 - that means its at the end but not start*/ 
    }else{
        prev->next=NULL; // for setting the prev to end of list
    }
    ptr = safe_sbrk(-(pending_end-(void*)blk));
    pending_start=pending_end =blk;
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
            if(isEndList(curr) && isEnoughToGiveUp(prev->data_size) 
            && os_giveback_flag){
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
        // Step 1.2 - call garbage collector so it can merge 
		// before deciding isEndList
        blk->isFree = TRUE;
        //memset(blk->data,0,blk->data_size);
        /* Case 1.1 : if blk is at the end and 
		 * has no merged(or else it would have
         checked to give back to os)
         */
        if(!merge_adj_open_blks() && isEndList(blk) && 
				isEnoughToGiveUp(blk->data_size) && os_giveback_flag){
            /*!merge_adj_open_blk() - implies that the blk wasnt merged*/
            giveBackToOS(blk);
        }
   }
}




/*===========Realloc helpers==================*/
/*
souce - contents are in to be copied
destination - where you want to copy
size_dest - in bytes how many bytes 
   
*/
void copy_contents(uint8_t *source, uint8_t *destination, size_t size_dest){
    struct hdr *src_blk = (void*)source-OFFSET, *des_blk= (void*)destination-OFFSET;
    size_t size_copy;
    int i;
    if(size_dest > src_blk->data_size){
        size_copy = src_blk->data_size;
    }else{
        size_copy=size_dest;
    }
    memcpy(destination, source, size_copy);
}

/* Requirements:
    0.) Try to merge adjacent spots if possible
    1.) Returns NULL, if cant allocate more space using 
		Sbrk; also setting errno to ENOMEM;
        keeping already reserved buffer.
    2.) Returns address of starting data section

    Special Cases:
        realloc(NULL, size) = malloc(size)
        realloc(ptr, 0) = free(ptr)
*/
void *realloc(void *ptr, size_t size){
    /*special cases*/
    /*step 1 - merge*/
    struct hdr *free_blk = ptr - OFFSET;
    void *ret_helper;
    if(ptr == NULL | !inHeap(ptr)){
        return malloc(size);
    }else if(size == 0){
        free(ptr);
    /*General case: requirements*/
    }else{
        /*step 1 - check to see if enough space
                    if we set to free and merge
                    any open spaces
        */
        // Step 1 - free (look for empty adj spots merge)
        // We dont want any DATA given back to os
        if(isEndList(free_blk) && round_up_mult_of_num(size,16) >= free_blk->data_size){
           ret_helper=ptr;
           update_pending(size-free_blk->data_size);
           free_blk->data_size=size;
        }else{
            os_giveback_flag = FALSE; 
            free(ptr); //indicates we cant give mem back in free
            os_giveback_flag=TRUE;
            ret_helper = malloc(size);
            copy_contents(ptr, ret_helper, size);
        }
        return ret_helper;
    }
}

/*========================calloc helpers====================================*/
void *calloc(size_t nmemb, size_t size){
    void *ptr;
    ptr=malloc(size*nmemb);
    memset(ptr, 0, nmemb*size);
    return ptr;
}

   volatile int i;
int main(){

    #define TEST1 100*sizeof(int)
    #define TEST2 1000*sizeof(int)

    int *ptr, *re_ptr;


    size_t i=0;
    for(i=1; i < 20000; i++){
            ptr=malloc(i+NEW_MEM_BLK);
            *ptr=1;
            re_ptr = realloc(ptr, i);
            

    }

/*
    ptr=malloc(TEST1);
    for(i=0; i < TEST1/sizeof(int); i++){
        ptr[i]=i;
    }
    ptr=realloc(ptr, TEST2);

    */
    int var = 0;
    

    return 0;

}


