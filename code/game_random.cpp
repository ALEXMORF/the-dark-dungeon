#include "game_random.h"

void seed_rand(int32 seed)
{
    random_index = seed % array_count(random_table);
}

int quick_rand()
{
    if (random_index == array_count(random_table)-1)
    {
        random_index = 0;
    }
    return random_table[random_index++];
}

int ranged_rand(int32 lo, int32 hi)
{
    int32 result = (quick_rand() % (hi - lo)) + lo;
    return result;
}

inline real32
real_quick_rand(real32 lo, real32 hi)
{
    real32 result;
    if ((int32)(hi - lo) != 0)
    {
        result = (real32)(quick_rand() % (int32)(hi - lo) + (int32)lo);
    }
    else
    {
        result = lerp(lo, hi, 0.5f);
    }
    
    return result;
}

inline bool32
one_in(uint32 base_number)
{
    assert(base_number != 0);

    bool32 result = false;
    
    int32 gen_number = quick_rand() % base_number;
    if (gen_number == gen_number/2)
    {
        result = true;
    }

    return result;
}
