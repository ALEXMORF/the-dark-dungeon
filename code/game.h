#pragma once

#include "game_platform.h"

#define abs(value) ((value) > 0? (value): -(value))
#define swap(x, y) do {x ^= y; y ^= x; x ^= y;} while (0)
#define if_do(condition, action) do { if (condition) action; } while (0)

#include "game_math.h"
#include "game_tiles.h"

#include "game_asset.h"
#include "game_render.h"
#include "game_raycaster.h"

struct Game_State
{
    v2 player_position;
    real32 player_angle;
    
    Loaded_Image wall_texture;
    Loaded_Image floor_texture;
    Loaded_Image ceiling_texture;
};
