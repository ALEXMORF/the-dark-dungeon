#pragma once

struct Entity;
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
    Sprite *content;
    int32 count;
    int32 capacity;
};

