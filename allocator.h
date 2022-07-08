/**
 * @file
 *
 * Function prototypes and structures for our memory allocator implementation.
 */

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <pthread.h>
#include <stddef.h>
#include <stdbool.h>

/* -- Helper functions -- */
// size_t align(size_t orig_size, size_t alignment);
// void set_free(struct mem_block *block);
// void set_used(struct mem_block *block);
// bool is_free(struct mem_block *block);
// void add_free(struct mem_block *block);
struct mem_block *split_block(struct mem_block *block, size_t size);
struct mem_block *merge_block(struct mem_block *block);
void *reuse(size_t size);
void *first_fit(size_t size);
void *worst_fit(size_t size);
void *best_fit(size_t size);
bool leak_check(void);
void print_memory(void);

/* -- C Memory API functions -- */
void *malloc_impl(size_t size, char *name);
void free_impl(void *ptr);
void *calloc_impl(size_t nmemb, size_t size, char *name);
void *realloc_impl(void *ptr, size_t size, char *name);

/**
 * Defines metadata structure for memory blocks. This structure is prefixed
 * before each allocation's data payload.
 */
struct mem_block {
    /**
     * Region this block is a part of. This should point to the first block in
     * the region.
     */
    struct mem_block *region;

    /**
     * The name of this memory block. If the user doesn't specify a name for the
     * block, it should be left empty (a single null byte).
     */
    char name[32];

    /** Size of the block */
    size_t size;

    /** Links for our doubly-linked list of blocks: */
    struct mem_block *next_block;
    struct mem_block *prev_block;
} __attribute__((packed));

struct free_block {
    struct mem_block block;
    struct free_block *next_free;
    struct free_block *prev_free;
} __attribute__((packed));

#endif
