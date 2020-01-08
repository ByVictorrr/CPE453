#include <stdio.h>

/* Requirements:
    1.) Returns NULL, if cant allocate more space using Sbrk; also setting errno to ENOMEM
    2.) Returns address of starting data section
*/
void *malloc(size_t size);

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



	
}
