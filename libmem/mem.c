#include <stdio.h>
#include <string.h>
#include "mem/mem.h"

static void * refill(oz_alloc * palloc, size_t n);
static char * chunk_alloc (oz_alloc * palloc, size_t size, int * nobjs);

extern void oz_init_alloc(oz_alloc * palloc)
{
	memset(palloc->free_list, 0, sizeof(palloc->free_list));
	palloc->start_free = 0;
	palloc->end_free  = 0;
	palloc->heap_size = 0;
	return;
}

extern void *oz_allocate(oz_alloc * palloc, size_t n)
{
	if (n <= (size_t)__MAX_BYTES){
		oz_node * volatile * my_free_list;
		oz_node * result;

		my_free_list = palloc->free_list + FREELIST_INDEX(n);
		result = *my_free_list;

		if (result == 0){
			void * r = refill(palloc, ROUND_UP(n));
			return r;
		}
		*my_free_list = result->free_list_link;
		return (result);
	}else{
		return malloc(n);
	}
}

extern void oz_deallocate(oz_alloc * palloc, void * p, size_t n)
{
	oz_node *q = (oz_node *) p;
	oz_node *volatile * my_free_list;
	if (n <= (size_t) __MAX_BYTES){
		my_free_list = palloc->free_list + FREELIST_INDEX(n);
		q->free_list_link = * my_free_list;
		*my_free_list = q;
	}else{
		free  (p);
	}
}

#if ALLOC_DEBUG
extern size_t oz_freelist_size (oz_alloc * palloc, size_t n)
{
	oz_node * volatile * my_free_list = palloc->free_list + FREELIST_INDEX(n);
	oz_node * current_obj;
	current_obj = *my_free_list;
	size_t count = 0;
	while(0!=current_obj && current_obj ->free_list_link != 0) {
		++count;
	}
	return count;
}
#endif

static void * refill (oz_alloc* palloc, size_t n)
{
	int nobjs = 20;
	char * chunk =  chunk_alloc(palloc, n, &nobjs);
	if (0 == chunk) {
		return chunk;
	}
	oz_node * volatile * my_free_list;
	oz_node * result;
	oz_node * current_obj, * next_obj;
	int i;
	if (1 == nobjs ) {
		return chunk;
	}

	my_free_list = palloc->free_list + FREELIST_INDEX(n);
	result = chunk;

	// 原来的代码可能有一个bug
	// 这里chunk + n 是指将chunk偏移n的size，也就是从下一个nsize的地址赋值给指针链表的
	// 应为第一个nsize是要返回给用户使用的。剩下的编入free_list
	// 原来的 chunk + n 的写法应该是有错误的，因为 chunk + n 则偏移了 n 个oz_node 的大小。而不是nsize的大小。
	// 稍候验证一下。
	*my_free_list = next_obj = (oz_node*) ((char*) chunk + n);

	// 建立链表

	for (i=1;;++i){
		current_obj = next_obj;
		// next_obj + n 是指将一个块完整的内存块，每个size个大小就切开来。
		//  这个n 可能是 8 16 24 32 。。。128
		next_obj = (oz_node *) ((char *) next_obj + n);
		if (nobjs - 1 == i) {
			current_obj ->free_list_link = 0;
		}else{
			current_obj ->free_list_link = next_obj;
		}
	}
	return (result);
}

/**
 *  整个思路就是
 *  1 看看mempool 中有没有适合的内存块，有即返回
 *  2 没有即给自己的内存池加水，加完了，递归调用自己看看mempool够不够返回，
 *  3 若是向系统申请失败了，内存没有内存了。向其他freelist借，有即将freelist的node还给mempool
 *  4 再递归调用自己看看内存池还有没有。
 */

static char * chunk_alloc(oz_alloc *palloc, size_t size, int * nobjs)
{
	char* result;
	size_t total_bytes = size * (*nobjs);
	size_t bytes_left   = palloc->end_free - palloc->start_free;

	if (bytes_left >= total_bytes) {
		result = palloc->start_free;
		palloc->start_free = palloc->start_free + total_bytes;
		return (result);
	}else if (bytes_left > size) {
		*nobjs = bytes_left / size;
		total_bytes = size * (*nobjs);
		result = palloc->start_free;
		palloc->start_free += total_bytes;
		return (result);
	}else{
		size_t bytes_to_get = 2 * total_bytes + ROUND_UP(palloc->heap_size >> 4);
		if (bytes_left > 0) {
			 oz_node * volatile * my_free_list = palloc->free_list + FREELIST_INDEX(bytes_left);
			 // j将mempool 的 残余内存编入freelist。插到最前面去。等于是在头头push入freelist。这样子就不用收尾。
			 ((oz_node *) (palloc->start_free))->free_list_link = *my_free_list;
			 *my_free_list = (oz_node*)(palloc->start_free);
		}
		// 收编完了就开始向系统申请加水。
		palloc->start_free = (char*) malloc(bytes_to_get);
		if (0 == palloc->start_free){
			// 当想系统申请内存失败的时候，将freelist其他还未用的内存拿回来，然后看看能不能组成一块，返回给用户。
			int i;
			oz_node * volatile * my_free_list, *p;
			for (i=size; i<__MAX_BYTES; i += __ALIGN) {
					my_free_list = palloc->free_list + FREELIST_INDEX(i);
					p = *my_free_list;
					if (0 != p) {
						// my_free_list 值向下一个这个，当前这个被归入内存池中。
						*my_free_list = p->free_list_link;
						palloc->start_free = (char*) p;
						palloc->end_free = palloc->start_free +i;
						// 释放完了，继续调用自己看看，mem pool 里面够不够内存返回。
						return (chunk_alloc(palloc, size, nobjs));
					}
			}
			//到了这里一点内存也没有了。只能返回空null
			palloc->end_free = 0;
			return (char*)0;
		}
		// 申请到内存,下次多申请一点
		palloc->heap_size += bytes_to_get;
		palloc->end_free = palloc->start_free + bytes_to_get;
		return (chunk_alloc(palloc, size, nobjs));
	}

}


