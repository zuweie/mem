#ifndef MEM_H
#define MEM_H
#include <stdlib.h>

#define ALLOC_DEBUG 1

#define __ALIGN 8
#define __MAX_BYTES 128

#define FREELIST_SIZE  (__MAX_BYTES)/(__ALIGN)
#define ROUND_UP(x) (((x) +  __ALIGN-1) & ~(__ALIGN - 1))
#define FREELIST_INDEX(x) (((x) + __ALIGN-1)/__ALIGN -1)

typedef union oz_node
{
	union node_t* free_list_link;
	char client_data[1];
}oz_node;


typedef struct oz_alloc
{
	char * start_free;
	char * end_free;
	size_t heap_size;
	oz_node * volatile free_list[FREELIST_SIZE];

}oz_alloc;

extern void oz_init_alloc(oz_alloc * palloc);
extern void * oz_allocate (oz_alloc * palloc, size_t n);
extern void  oz_deallocate(oz_alloc * palloc, void * p, size_t n);

#if ALLOC_DEBUG
extern size_t oz_freelist_size (oz_alloc * palloc, size_t n);
#endif

#endif

