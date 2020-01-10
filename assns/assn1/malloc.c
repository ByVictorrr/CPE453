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
#define SPLIT_MIN 1 /*used to determine whether a free node can be split into two*/


/*======================Helper functions for malloc============================*/
/*h1 - not free, h2 is free (has to have a valid address), size is size of h1 block*/
void split_hdrs(struct hdr **h1, struct hdr **h2, size_t size){
            *h1 = *h2;
            *h2 = *h1+sizeof(struct hdr)+size;        
            (*h2)->data_size = (*h1)->data_size - (2*sizeof(struct hdr) + size);
            (*h2)->isFree = TRUE;
            (*h2)->data = (uint8_t*)(*h1) + sizeof(struct hdr);
            (*h1)->data_size = size;
            /*finally reassign adj next's*/
            (*h2)->next=(*h1)->next;
            (*h1)->next=(*h2);
}

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
    struct hdr *next = start->next, *curr=start,*embedded;
    ssize_t diff;
    while(curr != NULL){
        /*checking until go to end of list if there is an openening in the list*/
        /*Thiis checks if blk is free && differnce is smaller than amount*/
        /*Case 1 - space for another*/
        if(curr->isFree && (diff=curr->data_size - size) > 0){
            /*Case 1.1 - split a existing one into two*/
            if(diff > SPLIT_MIN){
                // *assign the space curr needs, then make another blk above it open */
               split_hdrs(&embedded, &curr,  size);
                return embedded;
            /*Case 1.2 - not enough space to split*/
            }else{
                return curr;
            }
        }
        curr=next;
        if(next == NULL){
            break;
        }else{
            next=next->next;
        }
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
    if(pending_mem == pending_end | size + sizeof(struct hdr)+pending_mem > pending_end){
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

/*===============================================================*/

/*======================Helper functions for free============================*/
size_t size_merged_next_open_blks(struct hdr *ptr){
    /*Objective: ptr is one that we want to free (and to add up forward isFree blks)
        Basis step: ptr->next == NULL | ptr->isFree == FALSE;
        Inductive step: ptr = ptr->next ....ptr=NULL implies ptr+K
    */
   size_t ret_size = (ptr)->data_size + sizeof(struct hdr);

   if(ptr->next == (struct hdr *)NULL){
       return ret_size;
   }else if(ptr->next->isFree == FALSE){
       return ret_size;
   }else{
       return size_merged_next_open_blks(ptr->next) + ret_size;
   }
}

/*Returns: consequetive free block | the last block*/
struct hdr *get_last_adj_open_blk(struct hdr *ptr){
    if(ptr==NULL){
        return NULL;
    }
    struct hdr *next=ptr->next, *prev=ptr, *prev_prev = ptr;
    while(prev->isFree == TRUE && next!=NULL){
        prev_prev = prev;
        prev = next;
        next=next->next;
    }
    if(!prev->isFree){
        return prev_prev; 
    }
    if(next == NULL){
        return prev;
    }
}
/*Finds ajacent: mergeing them together */
void gargbage_collect(){
    /*go through the start and look merge the adj free*/
    struct hdr *next = start->next, *prev = start;
    while(next){
        if(prev->isFree == TRUE && next->isFree == TRUE){
            /*merge them*/
            prev->data_size += next->data_size + sizeof(struct hdr);
            prev->next = next->next;
        }
        prev = next;
        next = next->next;
    }
}

/* TODO: see if I need to change the contents of ptr (maybe i can use **ptr_adr = &ptr)
/**/
/* Requirements:
    0.) Try to give up space if a lot is free sbrk(negative value)
*/
void free(void *ptr){ //*ptr points to the data section
    uint8_t * start_block, *next_block;
    if(ptr){
        // step 1 - free the pointer
        start_block = ((uint8_t*)ptr)-sizeof(struct hdr);
        struct hdr *blk = start_block, *next_open_blk;
        blk->isFree = TRUE;
        size_t pending_diff = pending_end - pending_mem;

        // Case 1 - indicates if we are at the top of the heap (end of linked list)
        if((next_open_blk = get_last_adj_open_blk(blk))->next == NULL){
            if((blk->data_size=size_merged_next_open_blks(blk)-sizeof(struct hdr)) + pending_diff > NEW_MEM_BLK){
                sbrk((blk->data_size + pending_diff)*-1) == NULL 
                    ?fputc("Cant give mem back to os", stderr) 
                    : fputc("gave mem back to tos", stdout);
                    pending_end=pending_mem=start_block; // TODO : CHECK OVER THIS
                return;
            }
            
        // Case 2 - blk isnt at the end (therefore we cant give os back data)
        }else{
                // To see if the next block is free
                if(next_open_blk->isFree && blk != next_open_blk){
                    blk->next = next_open_blk->next; // delete the in between
                    blk->data_size = size_merged_next_open_blks(blk) - sizeof(struct hdr);
                }
                // otherwise dont do anything because its fine by itself
        }
    }else{
        fputs("Cant free 0x0", stderr);
    }
    // CASE - Garbage collect (i.e) check if the curr then next pattern is being freed
    gargbage_collect();
}
/*===============================================================*/

/*========================Realloc helpers====================================*/

bool_t inHeap(void *ptr){
    struct hdr *temp = start;
    while(temp){
        if((void*)temp+sizeof(struct hdr) == ptr){
            return TRUE;
        }
        temp=temp->next;
    }
    return FALSE;
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
void *realloc(void *ptr, size_t size){
    /*special case*/
    if(ptr == NULL | (ptr != NULL && !inHeap(ptr))){
        return malloc(size);
    }else if(size == 0){
        free(ptr);
    /*requirements*/
    }else{
        /*step 1 - merge*/
        struct hdr *p = ptr-sizeof(struct hdr), *blk_free = p;
        size_t org = p->data_size; 
        gargbage_collect(); // changes p's data size if there are adjspaces
        /*Step 2- see if there is space(from an adjacent block clearing up maybe from the garbage coll*/
        if(p->data_size >= size){
            /*Shrink  new blk then next->blk=isFree*/
            split_hdrs(&p, &blk_free, size);
            return ptr;
        /*Step 3 - no space*/
        }else{
            free(ptr);
            return malloc(org+p->data_size);
        }
    }
}

/*========================calloc helpers====================================*/
void *calloc(size_t nmemb, size_t size){
    int i;
    struct hdr *head;
    head=(struct hdr *)malloc(size);
    for ( i = 0; i < size; i++) {
        head->data[i] = nmemb;
    }
    return head->data;
}


int main(){


    /*
    
        =================== testing===============================
    TC 1 - Testing freeing hdr1->hdr2->hdr3 (hdr2)
                            free(hdr2) : hdr1->free->hdr3
             Description: because ptr4 size is 21 and hdr2's was 100 thus(0<diff < OPENING_DIFF_SIZE):
             Expected: malloc(21) : hdr1->hdr4->hdr3
                            


    TC 2 - Testing freeing hdr1->hdr2->hdr3 (hdr2)
                            free(hdr2) : hdr1->free->hdr3

             Description: because ptr4 size is 100 and hdr2's was 22 thus isnt (0<diff < OPE

             Expected: malloc(21) : hdr1->free->hdr3->hdr4



    TC 3 - Testing freeing hdr1->hdr2->hdr3 (hdr2)
             free(hdr2) : hdr1->free->hdr3

             Description: because ptr4 size is 20 and hdr2's was 200 thus (diff > OPENING_DI

             Expected: malloc(20) : hdr1->free->hdr3->hdr4

                            
    TC 4 - Testing freeing hdr1->hdr2->hdr3->hdr4
             Description: let hdr4->size = GIVE_UP_SPACE + 20; 
             Expected: free(ptr4) : hdr1->free->hdr3 (not there os took it)

    TC 5 - Testing multiple of 16 sizesu
             Description: let hdr4->size = 10u
             Expected: size to be 16u
u
u
    /* Free test cases (with pending)u
    TC 6 - Testing free where ptr3 is freed before ptr2
             Description: hdr1->hdr2->hdr3->hdr4->NULL
             Expected: hdr1->[hdr2|hdr3]->hdr4->NULL;

    TC 7 - testing free when ptr hdr4, then hdr3
            Description: hdr1->hdr2->hdr3->hdr4->NULL
             Expected: hdr1->hdr2->[free]->NULL;

    TC 8 - testing free when ptr hdr3, then hdr4
            Description: hdr1->hdr2->hdr3->hdr4->NULL
             Expected: hdr1->hdr2->(free)->NULL;
    
    TC 9 - testing malloc(11) = ptr5, freeing hdr3, then hdr4(for 100 = bytes for ptr3)
            Description: hdr1->hdr2->hdr3->hdr4->NULL
             Expected: hdr1->hdr2->(ptr5)->(free)->NULL; 

    TC 10 - testing malloc(11) = ptr5, freeing hdr3, then hdr4 (for 1000 = bytes for ptr3 (splitting))
            Description: hdr1->hdr2->hdr3->hdr4->NULL
             Expected: hdr1->hdr2->(ptr5)->(free)->(free)->NULL; 

    */
   /*
         ===================realloc testing===============================u
     TC 11 - 
            Description: testing realloc(ptr2, 100);
                    hdr1->hdr2->hdr3->NULL
             Expected: 
                    hdr1->free->hdr3->hdr4->NULL
     TC 12 - 
            Description: testing realloc(ptr3, 100);
                    hdr1->hdr2->hdr3->NULL
             Expected: 
                    hdr1->hdr2>hdr3->free->hdr4->NULL
   
   */


    int *ptr1 = (int*)malloc(100);
    int *ptr2 = (int*)malloc(25);
    int *ptr3 = (int*)malloc(1000);
    *ptr1=1;
    *ptr2=2;
    *ptr3=3;
    ptr3 = (int*)realloc(ptr3, 100);


    

    return 0;
}
