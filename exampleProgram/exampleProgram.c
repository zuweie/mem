/*
 ============================================================================
 Name        : exampleProgram.c
 Author      : zuweie
 Version     :
 Copyright   : Your copyright notice
 Description : Uses shared library to print greeting
               To run the resulting executable the LD_LIBRARY_PATH must be
               set to ${project_loc}/libTest/.libs
               Alternatively, libtool creates a wrapper shell script in the
               build directory of this program which can be used to run it.
               Here the script will be called exampleProgram.
 ============================================================================
 */
#include <stdio.h>
#include "mem/mem.h"

void testalloc_index (){
	size_t byte7_index = FREELIST_INDEX(7);
	size_t byte15_index = FREELIST_INDEX(15);
	size_t byte99_index = FREELIST_INDEX(99);
	size_t byte125_index = FREELIST_INDEX(125);
	printf("7:%d, 15:%d, 99:%d 125:%d\n", byte7_index, byte15_index, byte99_index, byte125_index);
}

void testalloc_roundup() {
	size_t byte7_round = ROUND_UP(7);
	size_t byte32_round = ROUND_UP(32);
	size_t byte23_round = ROUND_UP(23);
	size_t byt97_round = ROUND_UP(97);

	printf("7:%d, 32:%d, 23:%d 97:%d\n", byte7_round, byte32_round, byte23_round, byt97_round);
}

void testalloc_freelist(oz_alloc * palloc) {
	printf("size of freelist is  %d\n", sizeof(palloc->free_list) );
	printf("size of oz_node is %d\n", sizeof (oz_node*));
	int i;
	for(i=0; i<FREELIST_SIZE; ++i) {
		printf("%p\n", palloc->free_list[i]);
	}
}

void testalloc_alloc(oz_alloc * palloc)
{
#if ALLOC_DEBUG
	int i;
	for (i=0; i<3; ++i) {
		size_t size = rand() % 128;
		printf("take size of %d......\n", size);
		char * p1 = (char*) oz_allocate(palloc, size);
		printf("%d size of free list is %d\n", size, oz_freelist_size(palloc, size));
		char * p2 = oz_allocate(palloc, size);
		printf("%d size of free list is %d\n", size, oz_freelist_size(palloc, size));

		printf(" put back size of  %d ... \n", size);
		oz_deallocate(palloc, p1, size);
		printf("%d size of free list is %d\n", size, oz_freelist_size(palloc, size));
		oz_deallocate(palloc, p2, size);
		printf("%d size of free list is %d\n", size, oz_freelist_size(palloc, size));
	}
#endif
}

int main(void) {
  //print_hello();
  oz_alloc alloc;
  oz_init_alloc(&alloc);
  //testalloc_index();
  //testalloc_roundup();
 // testalloc_freelist(&alloc);
  //testalloc_alloc(&alloc);
  char * p = (char *) oz_allocate(&alloc, 100);
  oz_deallocate(&alloc, p, 100);

  return 0;
}
