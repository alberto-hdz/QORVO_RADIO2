/*
 * Copyright (c) 2024, Qorvo Inc
 *
 * platform_memory.c
 *   OpenThread platform heap allocation implementation for ThreadBleDoorbell.
 *   Required when OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE is set.
 */

#include <stdlib.h>

void *otPlatCAlloc(size_t aNum, size_t aSize)
{
    return calloc(aNum, aSize);
}

void otPlatFree(void *aPtr)
{
    free(aPtr);
}
