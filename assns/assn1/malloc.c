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


/*======================Helper functions for malloc============================*/
/*h1 - not free, h2 is free (has to have a valid address), size is size of h1 block*/
size_t round_mult16(size_t numBytes){
    bool_t isMult16;
    if(!(numBytes%16)){ /*if number is already mult of 16*/
        return numBytes;
    }else{
        return (numBytes-(numBytes%16))+16;
    }
}

void split_hdrs(struct hdr **h1, struct hdr **h2, size_t size){
    size_t mult_16 = round_mult16(size);
    *h1 = *h2;
    *h2 = *h1+OFFSET+mult_16;        
    (*h2)->data_size = (*h1)->data_size - (2*sizeof(struct hdr) + mult_16);
    (*h2)->isFree = TRUE;
    (*h2)->data = (uint8_t*)(*h1) + sizeof(struct hdr);
    (*h1)->data_size = mult_16;
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
/* Objective: to call sbrk as minimal as possible (that is if pend_mem == pend_end, get more mem make call to sbrk)
    return: NULL if srbrk error
    return: new spot address normall
    Assumption: size is mult of 16
*/
void *update_pending(size_t size){
    void *start;
    // Case 1 - if we dont have any more pending space
    if(pending_start >= pending_end | size + sizeof(struct hdr)+pending_start >= pending_end){
        size_t blk_size = size >= NEW_MEM_BLK ? size+OFFSET : NEW_MEM_BLK; //mult of 16 (how much given)
        start = sbrk(blk_size);
        pending_start=start+size + sizeof(struct hdr); //points to the next open space
        pending_end= start+blk_size; // points to end of pending
        return start;
    // Case 2 - if we still have space in the pending block
    }else{
        start = pending_start;
        pending_start = pending_start + size + sizeof(struct hdr);
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
/*Getting: if curr == start then returns NULL*/
struct hdr *get_prev_give_up_space(struct hdr *curr){
    struct hdr*temp=start,*prev;
    while(temp){
        if(temp == curr){
            return prev;
        }
        prev=temp;
        temp=temp->next;
    }
    return NULL;
}
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
size_t num_hdrs(){
    struct hdr *temp = start;
    size_t size=0;
    while(temp){
        size++;
        temp=temp->next;
    }
    return size;
}



/* TODO: see if I need to change the contents of ptr (maybe i can use **ptr_adr = &ptr)
/**/
/* Requirements:
    0.) Try to give up space if a lot is free sbrk(negative value)
*/
void free(void *ptr){ //*ptr points to the data section
    uint8_t * start_block, *next_block;
    if(ptr && inHeap(ptr)){
        // step 1 - free the pointer
        start_block = ((uint8_t*)ptr)-sizeof(struct hdr);
        struct hdr *blk = start_block, *next_open_blk, *prev;
        blk->isFree = TRUE;
        size_t pending_diff = pending_end - pending_start;

        // Case 1 - indicates if we are at the top of the heap (end of linked list)
        if((next_open_blk = get_last_adj_open_blk(blk))->next == NULL){
            if((blk->data_size=size_merged_next_open_blks(blk)-OFFSET) + pending_diff > GIVE_UP_SPACE){
                 /*TODO: ALSO NEED TO get PREV one to set next to NULL*/
                 if(num_hdrs()>1){
                        prev = get_prev_give_up_space(blk);
                        (*prev).next=(struct hdr*)NULL;
                 }
                 /*Add additional for infront next_open_blk to blk*/
               pending_end=pending_start=blk; // TODO : CHECK OVER THIS

               if(!sbrk((blk->data_size + pending_diff + OFFSET)*-1)){
                    fputc("giving back memory error", stderr);
               }
            }
        // Case 2 - blk isnt at the end (therefore we cant give os back data)
        }else{
                // To see if the next block is free (merge if so)
                if(next_open_blk->isFree && blk != next_open_blk){
                    blk->data_size = size_merged_next_open_blks(blk) - OFFSET;
                    blk->next = next_open_blk->next; // delete the in between
                }
                // otherwise dont do anything because its fine by itself
        }
        gargbage_collect();
    }else{
        fputs("Cant free ptr", stderr);
    }
    // CASE - Garbage collect (i.e) check if the curr then next pattern is being freed
}
/*===============================================================*/

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
    uintptr_t *ptr;
    struct hdr *head;
    head=(struct hdr *)(malloc(size*nmemb) - sizeof(struct hdr));
    for(ptr=head->data; ptr != head->data+head->data_size; ptr+=size){
        *ptr=0;
    }
    return head->data;
}

/*
int main(){
*/
    /* TC 1 - testing freeing big spot, then mallocing small spot(so testing the split)
        Function Under Test: free, malloc
        parms: 
        Description: (1) Freeing the first hdr in the list
                     (2) Then malloc a to test the split_hdrs parms 
                        (hdr1.size = 200(freed)) - initally
                        (hdr5.size = 16(not freed)), (hdr1.size=212-(16+32)(free))
        Case: Where the first in the list is freed, then mallocing 
              a smaller block st: (hd5.size=16) is looking for a spot
        Expected Output: (hdr5)->(free)->hdr2->hdr3->hdr4->NULL 

        After Testing: Sucess
    
    TC 2 - testing freeing small spot, then mallocing big spot (testing split)
        Function Under Test: free, malloc
        parms: 
        Description: (1) Freeing the first hdr in the list
                     (2) Then malloc a to test the split_hdrs parms 
                        (hdr1.size = 16(freed)) - initally
                        (hdr5.size = 200(not freed))
        Case: Where the first in the list is freed, and a bigger space is to be malloced
        Expected Output: (free)->hdr2->hdr3->hdr4->hdr5
        After Testing: Sucess    

    */
    /* TC 3 - get more memory from sbrk
        Under Test: pending_start, pending_end (malloc)
        Description: Malloc a very big mem spot see if pendings will ask sbrk
    
        Expected Output: call sbrk after mallocing twice with two 64000 sizes (blksize=64032)
                         pending_start=pending_end
        After Testing: pending_start=pending_end
    
    */
    /*TC 4 - get more memory from pending_start
        Under Test: pending_start, pending_end (malloc)
        Parmaters: malloc(10->16), malloc(3200)
        Description: Malloc two very small sizes and see if pending_start moves
        Expected Output: pending_start_f - pend_start_i = (3232)
        After Testing: 3232
    */
    
    /*
     TC 5 - testing merging adj blocks(garbage collector);
        Function Under Test: free, malloc
        parms: hdr2.size=3200, hdr3.size=112
        Description: Freeing hdr2 then hdr3 (freeing curr before next)
        Case: case where freeing prev before next
        Expected Output: (1)hdr1->(free)->hdr3->hdr4->NULL
                         (2)hdr1->[(free)->(free)]->hdr4->NULL
                         hdr2.size=(sizof(struct hdr)+hdr3.size+hdr2.size) =144+3200=3344
        After Testing: Sucess    

     TC 6 - testing merging adj blocks(free);
        Function Under Test: free, malloc
        parms: hdr2.size=3200, hdr3.size=112
        Description: Freeing hdr3 then hdr2 (freeing next before current)
        Case: case where freeing prev before next
        Expected Output: (1)hdr1->hdr2->(free)->hdr4->NULL
                         (2)hdr1->[(free)->(free)]->hdr4->NULL
                         hdr2.size=(sizof(struct hdr)+hdr3.size+hdr2.size) =112+3232=3344
        After Testing: Sucess    

    TC 7 - testing merging and giving back to os by negative value in os;
            Function Under Test: free, malloc
            parms: hdr2.size = 10,000, hdr3.size=20
            Description: They should merge from TC6 and as well as give the data back to os
            Case: Os should recieve more data
            Expected Output: (1)hdr1->NULL

            After Testing: 




    */

/*
    int *ptr1 = (int*)malloc(16);
    int *ptr2 = (int*)malloc(10000);
    int *ptr3 = (int*)malloc(20);
    *ptr1=1;
    *ptr2=2;
    *ptr3=3;
    free(ptr3);
    free(ptr2);
    int var;
    int *ptr4 = (int *)calloc(6,sizeof(int));





    return 0;
}

*/


void part1_tests(){
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
    int *ptr4 = (int *)calloc(6, 100);


}