#ifndef __MALLOC_H__
#define __MALLOC_H__

#include "lib_malloc.h"
#include "lib_string.h"

#define kmalloc(size, flags)    kmalloc(&kmempool,(size),0)
#define kfree(ptr)              kfree(&kmempool,(ptr))

#define vmalloc(size)           kmalloc((size),0)
#define vfree(ptr)              kfree(ptr);

#define kzalloc(size, flags)    calloc(size)

static inline void *calloc(unsigned int size)
{
    unsigned char *ptr;

    ptr = kmalloc((size),0);
    if  (ptr)
        memset(ptr, 0, size);

    return ptr;
}

#endif /* __MALLOC_H__ */
