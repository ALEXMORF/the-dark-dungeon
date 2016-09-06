#pragma once

struct Sprite
{
    Entity *owner;
    
    v2 size;
    v2 position;
    Loaded_Image texture;
    
    real32 distance_squared;
};

struct Sprite_List
{
    Sprite content[SPRITE_COUNT_MAX];
    int32 count;
};

