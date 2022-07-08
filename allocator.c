/**
 * @file
 *
 * Implementations of allocator functions.
 */

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

#include "allocator.h"
#include "logger.h"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

#define ALIGNMENT 16

struct mem_block *blist_head = NULL;
struct mem_block *blist_tail = NULL;

struct free_block *free_head = NULL;
struct free_block *free_tail = NULL;

/**
 * Rounds stuff up to the nearest dividend.
 * 
 * align(7, 8) -> 8
 * align(1, 8) -> 8
 * align(8, 8) -> 8
 * align(8, 8) -> 8
 * align(9, 8) -> 16
 * 
 */
size_t align(size_t orig_size, size_t alignment) 
{
    size_t new_size = (orig_size / alignment) * alignment;
    if (orig_size % alignment != 0) {
        new_size += alignment;
    }
    return new_size;
}

void set_free(struct mem_block *block)
{
    block->size = block->size | 0x01;
}

void set_used(struct mem_block *block)
{
    block->size = block->size & ~(0x01);
}

size_t real_size(size_t size)
{
    return size & ~(0x01);
}

bool is_free(struct mem_block *block) 
{
    return (block->size & 0x01) == 0x01;
}

void add_free(struct mem_block *block) 
{
    set_free(block);
    struct free_block *fblock = (struct free_block *) block;

    if (free_head == NULL && free_tail == NULL) {
        free_head = fblock;
        free_tail = fblock;
    } else {
        fblock->next_free = free_head;
        free_head->prev_free = fblock;
        free_head = fblock;
    }
}

/**
 * Given a free block, this function will split it into two blocks (if
 * possible).
 *
 * @param block Block to split
 * @param size Size of the newly-created block. This block should be at the
 * "end" of the original block:
 * 
 *     +----------------------+-----+
 *     | (old block)          | new |
 *     +----------------------+-----+
 *     ^                      ^
 *     |                      |
 *     |                      +-- pointer to beginning of new block
 *     |
 *     +-- original block pointer (unchanged)
 *         update its size: old_block_size - new_block_size
 *
 *
 * @return address of the resulting second block (the original address will be
 * unchanged) or NULL if the block cannot be split.
 */
struct mem_block *split_block(struct mem_block *block, size_t size)
{
    // TODO block splitting algorithm
    // check the block is:
    // * not null
    // * actually has enough space for the request
    // * after subtracting whatever the request is, still has enough space for two complete blocks (min_size(80))

    size_t min_size = 80;
    if (block == NULL || size < min_size) {
        // LOGP("block == null or size < 80");
        return NULL;
    } 

    size_t block_size = real_size(block->size);
    // LOG("size here = %lu\n", block_size);
    struct mem_block *new_block = (struct mem_block *) ((char *) block + block_size - size);

    if (block_size < min_size) {
        // LOGP("block size < min size");
        return NULL;
    }

    if (!is_free(block)) {
        // LOGP("not free");
        return NULL;
    }

    if (block->next_block == NULL) {
        // LOGP("block too small/not enough space");
        return NULL;
    } else {
        // LOGP("splitting");
        new_block->next_block = block->next_block; // result->next_block == next_block [  OK  ]
        // LOG("block->next_block = %lu\n", block->next_block);
        // LOG("new_block->next_block = %lu\n", new_block->next_block);

        block->next_block = new_block; // block->next_block == result [ OK ]
        // LOG("block->next_block = %lu\n", block->next_block);
        // LOG("new_block = %lu\n", new_block);

        new_block->prev_block = block; // result->prev_block == block [ OK ]
        // LOG("block->prev_block = %lu\n", block->prev_block);
        // LOG("new_block = %lu\n", new_block);

        new_block->next_block->prev_block = new_block; // next_block->prev_block == result [ OK ]
        // LOG("new_block->next_block->prev_block = %lu\n", new_block->next_block->prev_block);
        // LOG("new_block = %lu\n", new_block);

        ssize_t new_size = block->size - size;
        // LOG("new_size: %lu\n", new_size);
        block->size = new_size; // block->size == 7265 [  OK  ]

        new_block->size = block_size;
        // LOG("new_block->size %lu\n", new_block->size); // 8000
        // LOG("result->size %lu\n", new_block->size); // 735
    
        ssize_t new_block_new_size = new_block->size - new_size;
        // LOG("new_block_new_size: %lu\n", new_block_new_size);
        new_block->size = new_block_new_size + 2; // result->size == 737 [  OK  ]

        return new_block;
    }

    new_block->region = block->region;
}

/**
 * Given a free block, this function attempts to merge it with neighboring
 * blocks --- both the previous and next neighbors --- and update the linked
 * list accordingly.
 *
 * For example,
 *
 * merge(b1)
 *   -or-
 * merge(b2)
 *
 *     +-------------+--------------+-----------+
 *     | b1 (free)   | b2 (free)    | b3 (used) |
 *     +-------------+--------------+-----------+
 *
 *                   |
 *                   V
 *
 *     +----------------------------+-----------+
 *     | b1 (free)                  | b3 (used) |
 *     +----------------------------+-----------+
 *
 * merge(b3) = NULL
 *
 * @param block the block to merge
 *
 * @return address of the merged block or NULL if the block cannot be merged.
 */
struct mem_block *merge_block(struct mem_block *block)
{
    // TODO block merging algorithm

    /**
     * Big assumption: We make sure there are never two...
     * 
     * 
     * 1. is the block free?
     * 2. who are my neighbors? (look at next and previous blocks (not next_free))
     *      merging = block list not free list
     * 3. are my neighbors in the same region?
     *      if not, can't merge with them
     * 4. are my neighbors free?
     *      if they are, we can merge with them
     * 
     * assuming we actully need to merge, start doing this:
     * 1. start w/ merge to the right
     *      go to the next block, and if it's free, combine with the current block
     *      increase the size
     *      update the linked list
     *      remove the neighbor from the free list
     * 2. next do a merge "to the left"
     *      this works exactly the same as merge to the right, except for:
     *          we merge the previous block to the right into our current block
     *      make a function that takes in two blocks, left and right
     *      use this function like:
     *          right_merge(current, next);
     *          right_merge(previous, current);
     *          you will eventually use this for realloc()
     * 3. at the very end after we've done our merges, return the leftmost block
     *      so if the first right merge worked and the second didn't, then we return the current
     *      and if the second right merge worked, return the previous block
     **/

    return NULL;
}

/**
 * Given a block size (header + data), locate a suitable location in the free
 * list using the first fit free space management algorithm.
 *
 * @param size size of the block (header + data)
 */
void *first_fit(size_t size)
{
    // TODO: first fit FSM implementation

    struct free_block *free = free_head;
    while (free != NULL) {
        LOG("FF checking [%p]\n", free);
        if (real_size(free->block.size) >= size) {
            return free;
        }
        free = free->next_free;
    }
    return NULL;
}

/**
 * Given a block size (header + data), locate a suitable location in the free
 * list using the worst fit free space management algorithm. If there are ties
 * (i.e., you find multiple worst fit candidates with the same size), use the
 * first candidate found in the list.
 *
 * @param size size of the block (header + data)
 */
void *worst_fit(size_t size)
{
    // TODO: worst fit FSM implementation
    return NULL;
}

/**
 * Given a block size (header + data), locate a suitable location in the free
 * list using the best fit free space management algorithm. If there are ties
 * (i.e., you find multiple best fit candidates with the same size), use the
 * first candidate found in the list.
 *
 * @param size size of the block (header + data)
 */
void *best_fit(size_t size)
{
    // TODO: best fit FSM implementation
    return NULL;
}

void *reuse(size_t size)
{
    // TODO: using free space management (FSM) algorithms, find a block of
    // memory that we can reuse. Return NULL if no suitable block is found.
    return NULL;

    // char *algo = getenv("ALLOCATOR_ALGORITHM");
    // if (algo == NULL) {
    //     algo = "first_fit";
    // }

    // struct mem_block *reused_block = NULL;
    // if (strcmp(algo, "first_fit") == 0) {
    //     reused_block = first_fit(size);
    // } else if (strcmp(algo, "best_fit") == 0) {
    //     reused_block = best_fit(size);
    // } else if (strcmp(algo, "worst_fit") == 0) {
    //     reused_block = worst_fit(size);
    // }

    // if (reused_block != NULL) {
    //     // do stuff = potentially split the block if it can be split
    //     LOG("Found a block to reuse: %p\n", reused_block);

    //     // problems:
    //     // 1. we don't remove anything from the free list
    //     // 2. we don't update the free flags (could be done in #1)
    //     // 3. we don't split the blocks we find
    //     // 4. we don't merge blocks
    //     // 5. we don't munmap them when the blocks represent a single
    // }

    // return reused_block;
}

void *malloc_impl(size_t size, char *name)
{
    // TODO: allocate memory. You'll first check if you can reuse an existing
    // block. If not, map a new memory region.
    // return NULL;

    pthread_mutex_lock(&lock);

    size_t actual_size = size + sizeof(struct mem_block);
    size_t aligned_size = align(actual_size, ALIGNMENT);

    struct mem_block *reused_block = reuse(aligned_size);
    if (reused_block != NULL) {
        set_used(reused_block);
        pthread_mutex_unlock(&lock);
        return reused_block + 1;
    }

    // size_region_size = align();
    struct mem_block *block = mmap(
        NULL,
        actual_size, // replace
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0);
    
    if (block == MAP_FAILED) {
        perror("mmap");
        pthread_mutex_unlock(&lock);
        return NULL;
    }

    strcpy(block->name, name);
    block->region = block;
    // block->size = region_size;

    set_free(block);
    split_block(block, real_size(block->size) - aligned_size);
    set_used(block);

    //todo here
    // 1. block list contains a region with two blocks
    // 2. free list contains one block (the one we just split off from first block)

    block->size = actual_size;
    pthread_mutex_unlock(&lock);

    // scribble
    char *scribble = getenv("ALLOCATOR_SCRIBBLE");
    if (scribble != NULL) {
        memset(block + 1, 0xAA, actual_size);
    }

    return block + 1;

}

void free_impl(void *ptr)
{
    if (ptr == NULL) {
        /* Freeing a NULL pointer does nothing */
        return;
    }
    
    struct mem_block *block = (struct mem_block *)ptr - 1;
    // LOG("free request on %p; header: %p; block size: %zu\n", ptr, block, block->size);

    add_free(block);

    // struct mem_block *merged = merge_block(block);
    // if (merged->next_block is in a different region)
    //     and merged->prev_block is in a different region {
    //         we are alone!!
    //         update the linked list again since our block is gone
    //         unmap
    //     }

    // LOGP("not munmappping for later - fix later\n");
    // if (munmap(header, header->size) == -1) {
    //     perror("munmap");
    // }
    // LOGP("done freeing\n");

    // TODO: free memory. If the containing region is empty (i.e., there are no
    // more blocks in use), then it should be unmapped.
}

void *calloc_impl(size_t nmemb, size_t size, char *name)
{
    // TODO: hmm, what does calloc do?
    void *ptr = malloc_impl(nmemb * size, name);
    memset(ptr, 0, nmemb * size);
    return ptr;

    // return NULL;
}

void *realloc_impl(void *ptr, size_t size, char *name)
{
    // if (ptr == NULL) {
    //     /* If the pointer is NULL, then we simply malloc a new block */
    //     return malloc_impl(size, name);
    // }

    // if (size == 0) {
    //     /* Realloc to 0 is often the same as freeing the memory block... But the
    //      * C standard doesn't require this. We will free the block and return
    //      * NULL here. */
    //     free_impl(ptr);
    //     return NULL;
    // }

    // // // TODO: reallocation logic

    // return NULL;

    void *new_ptr = malloc(size);
    if (new_ptr == NULL) {
        // return NULL;
        return malloc_impl(size, name);
    }

    // If NULL ptr passed into realloc, then it acts just like malloc:
    if (ptr == NULL) {
        return new_ptr;
    }

    // We know the ptr is not NULL at this point:
    if (size == 0) {
        free(new_ptr);
        return NULL;
    }

    memcpy(new_ptr, ptr, size);
    free(ptr);
    return new_ptr;
}

/**
 * Prints out the current memory state, including both the regions and blocks,
 * followed by the list of free blocks (in the order they were freed).
 *
 * Since entries are printed in order, there is an implied link from the topmost
 * entry to the next, and so on. Print to stdout in the following format:
 *
 * -- Current Memory State --
 * [REGION 0x7f0d774e7000]
 *   [BLOCK 0x7f0d774e7000-0x7f0d774e70a8] 168     [USED]  'First Allocation'
 *   [BLOCK 0x7f0d774b0000-0x7f0d774b0050] 80      [USED]  'Second Allocation'
 *   [BLOCK 0x7f0d774af000-0x7f0d774af0a8] 168     [USED]  'Third Allocation'
 *      ...
 *   (list continues)
 * 
 * -- Free List --
 * [0x7f0d774e70a8] -> [0x7f0d774b0050] -> [0x7f0d774af0a8] -> (...) -> NULL
 */
void print_memory(void)
{
    // TODO implement memory printout
    puts("-- Current Memory State --\n");
    // struct mem_block *mem = blist_head;
    // while (mem != NULL) {
    //     printf("[REGION %p]\n", mem->region);
    // }
    
    puts("-- Free List --");
    // struct free_block *free = free_head;
    // while (free != NULL) {
    //     printf("[%p] -> ", free);
    //     free = free->next_free;
    // }
    puts("NULL");
}

/**
 * Scans through the current memory state and finds leaks (blocks that are not
 * free). This function should generally be called at the end of a program's
 * execution. Each leaked block should be printed to stdout as shown below:
 *
 * -- Leak Check --
 * [BLOCK 0x7f0d774e7000] 168     'First Allocation'
 * [BLOCK 0x7f0d774b0000] 80      'Second Allocation'
 *      ...
 *   (list continues)
 *
 * -- Summary --
 * 542 blocks lost (892412 bytes)
 *
 * @return true if there are memory leaks, false otherwise
 */
bool leak_check(void)
{
    return true;
}

// int main(void) 
// {
//     void *a = malloc_impl(300, "bob");
//     void *b = malloc_impl(100, "matthew");
//     void *c = malloc_impl(500, "bobby");

//     free_impl(a);
//     free_impl(b);
//     free_impl(c);

//     print_memory();
// }

