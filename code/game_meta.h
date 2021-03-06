#pragma once

//////////////////////////////////////////////////
// Define all the macros for generating code
//////////////////////////////////////////////////

//NOTE(chen): SBuffer stands for static buffer (buffer resides inside struct itself)
#define Def_SBuffer(type, cap)                   \
    struct type##_SBuffer                        \
    {                                            \
        type e[cap];                             \
        int32 count;                             \
        int32 capacity;                          \
    };
//TODO(chen): add the functionality to be generated for SBuffers

//NOTE(chen): DBuffer stands for dynamic buffer (buffer holds a pointer to memory somewhere else)
#define Def_DBuffer(Type)                                       \
    struct Type;                                                \
    struct Type##_DBuffer                                       \
    {                                                           \
        Type *e;                                                \
        int32 count;                                            \
        int32 capacity;                                         \
    };                                                          \
    void add_##Type(DBuffer(Type) *buffer, Type item)           \
    {                                                           \
        assert(buffer->count < buffer->capacity);               \
        buffer->e[buffer->count++] = item;                      \
    }                                                           \
    void remove_##Type(DBuffer(Type) *buffer, int index)        \
    {                                                           \
        assert(index < buffer->count && index >= 0);            \
        buffer->e[index] = buffer->e[--buffer->count];          \
    }

#define DBuffer(Type) Type##_DBuffer

//////////////////////////////////////////////////
// Code generation
//////////////////////////////////////////////////

Def_DBuffer(Entity);
Def_DBuffer(Loaded_Image);
