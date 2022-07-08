/**
 * @file
 *
 * Contains stubs that call into our custom allocator library. This is necessary
 * during testing to allow us to call both our own allocator and the system
 * allocator without having things crash in spectacular ways.
 *
 * TL;DR: you do not have to modify this file.
 *
 * To use (for one specific command):
 * LD_PRELOAD=$(pwd)/allocator.so command
 * ('command' will run with your allocator)
 *
 * To use (for all following commands):
 * export LD_PRELOAD=$(pwd)/allocator.so
 * (Everything after this point will use your custom allocator -- be careful!)
 */

#include "allocator.h"

void *malloc(size_t size)
{
    return malloc_impl(size, "");
}

void free(void *ptr)
{
    free_impl(ptr);
}

void *calloc(size_t nmemb, size_t size)
{
    return calloc_impl(nmemb, size, "");
}

void *realloc(void *ptr, size_t size)
{
    return realloc_impl(ptr, size, "");
}
