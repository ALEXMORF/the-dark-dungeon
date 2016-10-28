#pragma once

struct Linear_Allocator
{
    uint8 *base_ptr;
    uint32 size;
    uint32 used;
};
