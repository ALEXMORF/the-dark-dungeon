#include "game_memory.h"

#define Copy_Array(source, dest, count, type) copy_memory(source, dest, count*sizeof(type))
inline void
copy_memory(void *source_in, void *dest_in, uint32 size)
{
    uint8 *source = (uint8 *)source_in;
    uint8 *dest = (uint8 *)dest_in;
    for (uint32 i = 0; i < size; ++i)
    {
        dest[i] = source[i];
    }
}

inline void
clear_memory(void *memory, uint32 size)
{
    memset(memory, 0, size);
}

inline void
initialize_linear_allocator(Linear_Allocator *allocator, void *base_ptr, uint32 size)
{
    allocator->base_ptr = (uint8 *)base_ptr;
    allocator->size = size;
    allocator->used = 0;
}

#define Push_Array(allocator, array_length, type) (type *)linear_allocate(allocator, array_length*sizeof(type))
inline void *
linear_allocate(Linear_Allocator *allocator, uint32 wanted_size)
{
    assert(allocator->used + wanted_size <= allocator->size);
    void *result = (uint8 *)allocator->base_ptr + allocator->used;
    allocator->used += wanted_size;
    return result;
}

