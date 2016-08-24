#pragma once

#include "game_platform.h"

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
};
