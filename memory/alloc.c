#include "alloc.h"

// main manage var
Heap heap = {NULL, NULL, 0};
chunk *chunk_list[16] = {NULL};
int Min_size = 8;
int Max_size = 128;
int Nnum = 20;

void *lalloc(int size, int n){
	// behavior is like calloc
	assert (size > 0 && n > 0);
	// for contain the size
	int alloc_size = size*n + 2;
	if (alloc_size > Max_size) {
		void *p = calloc(size, n);
		(*(char *)p) = alloc_size;
		return ((char *)p) + 2;
	}
	void *res = chunk_alloc(alloc_size);
	//最开始的2个字节存储size大小
	*((char *)res) = alloc_size;
	return ((char *)res) + 2;
}

void lfree(void *p) {
	p = (short *)p - 1;
	short size= *(short *)p;
	if (size > Max_size) {
		free(p);
	}
	else {
		int indx = Index(round_up(size));
		// head insert the p
		chunk *next = chunk_list[indx];
		chunk_list[indx] = (chunk *)p;
		chunk_list[indx]->next_chunk = next;
	}
}


int round_up(int size) {
	return (size+Min_size-1) & ~(Min_size-1);
}

int Index(int size) {
	assert(size%8 == 0);
	return (size >> 3) - 1;
}

void *chunk_alloc(int size) {
	size = round_up(size);
	int indx = Index(size);
	if (chunk_list[indx] != NULL) {
		chunk* res = chunk_list[indx];
		chunk_list[indx] = res->next_chunk;
		memset(res, 0, size);
		return res;
	}
	if (heap_refill(indx)) {
		//refill from heap successfully
		return chunk_alloc(size);
	}
	else {
		return NULL;
	}
}


bool heap_refill(int index) {
	int left_size = heap.end - heap.start;
	int size = Min_size * (index + 1);
	int need_size = size*Nnum;
	int num = Nnum;
	// mem: to be link into chunk_list
	// heap.start: next tobe used memory
	char *mem = NULL;
	//not enough for Nnums
	if (left_size < need_size) {
		// enough for one
		if (left_size > size) {
			num = left_size/size;
			mem = heap.start;
			heap.start += num*size;
		}
		else {
			// not enough for one so use malloc for new and deal left
			if (left_size > 0) {
				int indx = Index(left_size);
				// head insert into chunk_list
				chunk *next = chunk_list[indx];
				chunk_list[indx] = (chunk*)heap.start;
				chunk_list[indx]->next_chunk = next;
			}
			int get_size = 2*need_size + round_up(heap.total_size>>1);
			// start += left_size;
			heap.start = (char *)malloc(get_size);
			if (heap.start == NULL) {
				// malloc failed, so get some from behind
				int i = Index(size) + 1;
				// start + add_sz == end
				int add_sz = size + Min_size;
				bool flg = false;
				for (; i<16; ++i, add_sz+=Min_size) {
					if (chunk_list[i] != NULL) {
						num = add_sz/size;
						mem = (char *)chunk_list[i];
						heap.start = mem + num*size;
						heap.end = mem + add_sz;
						///////////////////////////
						chunk_list[i] = chunk_list[i]->next_chunk;
						flg = true;
						break;
					}
				}
				if (!flg) {
					return false;
				}
			}
			else {
				mem = heap.start;
				heap.start += num*size;
				heap.end = mem + get_size;
				heap.total_size += get_size;
			}
		}
	}
	else {
		mem = heap.start;
		heap.start += need_size;
	}
	// link for chunk_list
	chunk *current = (chunk *)mem;
	chunk *next = (chunk *)((char *)current + size);
	chunk_list[index] = current;
	//1->2 2->3 3->4 19->20 next(20)->NULL
	int i = 0;
    for (; i<num-1; ++i) {
		current->next_chunk = next;

		current = next;
		next = (chunk *)((char *)next + size);
	}	
	next->next_chunk = NULL;
	return true;
}
