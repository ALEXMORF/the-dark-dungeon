/*
 *TODOLIST:

 Integrate SDL to get hardare-acceleration on blitting and v-sync
 Implement a sound buffer with SDL
 Procedure map generation
 clean up old sound code
 
 */

#include "game.h"

#include "game_math.cpp"
#include "game_tiles.cpp"

#include "game_asset.cpp"
#include "game_render.cpp"
#include "game_sprite.cpp"
#include "game_raycaster.cpp"

#include "game_entity.cpp"
#include "game_simulate.cpp"

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

//
//
//Game Code

inline void
load_assets(Game_State *game_state, Platform_Load_Image *platform_load_image)
{
#define Load_Wall_Tex(index, filename)                                  \
    game_state->wall_textures.E[index] = load_image(platform_load_image, filename)
    
    game_state->wall_textures.count = 6;
    game_state->wall_textures.E = Push_Array(&game_state->permanent_allocator, game_state->wall_textures.count, Loaded_Image);
    Load_Wall_Tex(0, "../data/redbrick.png");
    Load_Wall_Tex(1, "../data/bluestone.png");
    Load_Wall_Tex(2, "../data/colorstone.png");
    Load_Wall_Tex(3, "../data/eagle.png");
    Load_Wall_Tex(4, "../data/purplestone.png");
    Load_Wall_Tex(5, "../data/wood.png");
        
    game_state->floor_texture = load_image(platform_load_image, "../data/greystone.png");
    game_state->ceiling_texture = load_image(platform_load_image, "../data/greystone.png");
    game_state->barrel_texture = load_image(platform_load_image, "../data/barrel.png");
    game_state->pillar_texture = load_image(platform_load_image, "../data/pillar.png");
    game_state->light_texture = load_image(platform_load_image, "../data/greenlight.png");

    game_state->weapon_texture_sheet = load_image_sheet(platform_load_image, "../data/weapons.png");
    game_state->weapon_texture_sheet.image_count_x = 5;
    game_state->weapon_texture_sheet.image_count_y = 5;
    game_state->weapon_texture_sheet.stride_offset = 1;
    game_state->weapon_texture_sheet.image_width = 64;
    game_state->weapon_texture_sheet.image_height = 64;

    game_state->guard_texture_sheet = load_image_sheet(platform_load_image, "../data/guard.png");
    game_state->guard_texture_sheet.image_count_x = 8;
    game_state->guard_texture_sheet.image_count_y = 7;
    game_state->guard_texture_sheet.stride_offset = 1;
    game_state->guard_texture_sheet.image_width = 64;
    game_state->guard_texture_sheet.image_height = 64;

    game_state->ss_texture_sheet = load_image_sheet(platform_load_image, "../data/ss.png");
    game_state->ss_texture_sheet.image_count_x = 8;
    game_state->ss_texture_sheet.image_count_y = 7;
    game_state->ss_texture_sheet.stride_offset = 1;
    game_state->ss_texture_sheet.image_width = 63;
    game_state->ss_texture_sheet.image_height = 63;
}

inline void
fill_entities(Entity_List *entity_list)
{
    add_entity(entity_list, make_dynamic_entity(guard, {6.0f, 15.5f}));
    add_entity(entity_list, make_dynamic_entity(guard, {15.0f, 7.0f}, pi32));
    add_entity(entity_list, make_dynamic_entity(guard, {6.0f, 7.0f}));
    add_entity(entity_list, make_dynamic_entity(ss, {15.0f, 6.0f}, pi32));
    add_entity(entity_list, make_dynamic_entity(ss, {15.0f, 8.0f}, pi32));
    add_entity(entity_list, make_dynamic_entity(ss, {15.0f, 9.0f}));
    add_entity(entity_list, make_dynamic_entity(ss, {15.0f, 15.0f}, pi32));
    add_entity(entity_list, make_dynamic_entity(ss, {14.0f, 15.0f}));
    add_entity(entity_list, make_dynamic_entity(ss, {16.0f, 15.0f}));
}

internal void
initialize_player(Player *player)
{
    player->position = {3.0f, 3.0f};
    player->angle = 0.0f;
    player->collision_radius = 0.3f;
    player->weapon_animation_index = 1;
    player->weapon = pistol;
    player->weapon_cd = 0.5f;
}
                 
//
//
//Brain
extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    assert(sizeof(Game_State) <= memory->permanent_storage_size);
    Game_State *game_state = (Game_State *)memory->permanent_storage;
    
    if (!memory->is_initialized)
    {
        //prepare memory allocators
        initialize_linear_allocator(&game_state->permanent_allocator,
                                    (uint8 *)memory->permanent_storage + sizeof(Game_State),
                                    memory->permanent_storage_size - sizeof(Game_State));
        initialize_linear_allocator(&game_state->transient_allocator,
                                    (uint8 *)memory->transient_storage,
                                    memory->transient_storage_size);
        
        //game
//TODO(chen): this is some improv tile-map init code, replace this with procedural generation later
#define map_width 20
#define map_height 20
        uint32 tiles[map_width*map_height] =
            {
                3, 3, 3, 3, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1,
                3, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                3, 0, 0, 0, 4, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                3, 0, 0, 0, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,
                3, 1, 4, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 0, 0, 2, 1, 1, 1,
                1, 1, 1, 2, 0, 0, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1,
                1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,
                4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1
            };
        Tile_Map *tile_map = &game_state->tile_map;
        tile_map->tile_count_x = map_width;
        tile_map->tile_count_y = map_height;
        tile_map->exception_tile_value = 1;
        int32 tile_count = tile_map->tile_count_x * tile_map->tile_count_y;
        tile_map->tiles = Push_Array(&game_state->permanent_allocator, tile_count, uint32);
        Copy_Array(tiles, tile_map->tiles, tile_count, uint32);

        initialize_player(&game_state->player);

        game_state->entity_list.capacity = ENTITY_COUNT_LIMIT;
        game_state->entity_list.content = Push_Array(&game_state->permanent_allocator, game_state->entity_list.capacity, Entity);
        fill_entities(&game_state->entity_list);
        
        load_assets(game_state, memory->platform_load_image);
        
        Render_Context *render_context = &game_state->render_context;
        render_context->z_buffer = Push_Array(&game_state->permanent_allocator, buffer->width, real32);
        render_context->floorcast_table_count = buffer->height/2;
        render_context->floorcast_table = Push_Array(&game_state->permanent_allocator, render_context->floorcast_table_count, real32);
        for (int32 i = 0; i < render_context->floorcast_table_count; ++i)
        {
            real32 inverse_aspect_ratio = (real32)buffer->width / buffer->height;
            real32 real_scan_y = ((real32)buffer->height/2 + (real32)i / inverse_aspect_ratio);
            render_context->floorcast_table[i] = (real32)buffer->height / (2*real_scan_y - buffer->height);
        }
        
        memory->is_initialized = true;
    }
    game_state->transient_allocator.used = 0;
    
    simulate_world(game_state, input);
    
    //
    //
    //Render game
    
    fill_buffer(buffer, 0);

    Sprite_List sprite_list = {};
    sprite_list.capacity = game_state->entity_list.capacity;
    sprite_list.content = Push_Array(&game_state->transient_allocator, sprite_list.capacity, Sprite);

    //draw 3d scene and sprites
    generate_sprite_list(game_state, &sprite_list, game_state->entity_list.content, game_state->entity_list.count);
    sort_sprites(sprite_list.content, sprite_list.count, game_state->player.position);
    game_state->currently_aimed_entity = render_3d_scene(buffer, &game_state->render_context,
                                                         &game_state->tile_map,
                                                         game_state->player.position,
                                                         game_state->player.angle, 
                                                         &game_state->floor_texture,
                                                         0, //&game_state->ceiling_texture,
                                                         &game_state->wall_textures,
                                                         sprite_list.content, sprite_list.count);
    
    //animate first-person weapon
    {
        Player *player = &game_state->player;
        
        real32 animation_cycle = player->weapon_cd - 0.02f;
        real32 animation_index_count = 4.0f;
        int32 animation_ending_index = 1;
        if (player->weapon_cd_counter)
        {
            real32 time_passed = player->weapon_cd - player->weapon_cd_counter;
            if (time_passed < animation_cycle)
            {
                real32 animation_index_interval = animation_cycle / animation_index_count;
                player->weapon_animation_index = (int32)(time_passed / animation_index_interval + (animation_ending_index + 1)) % ((int32)animation_index_count + 1);
            }
            else
            {
                player->weapon_animation_index = animation_ending_index;
            }
        }
    }
    
    //draw first-person weapon
    {
        real32 y_scale = (real32)buffer->height / 50;
        real32 x_scale = (real32)buffer->width / 40;
        game_state->player.pace += len(game_state->player.velocity);
        int32 bob_x = (int32)(sinf(game_state->player.pace * 2.5f) * x_scale);
        int32 bob_y = (int32)(cosf(game_state->player.pace * 5.0f) * y_scale) + (int32)y_scale;
        
        v2 weapon_sprite_size = {(real32)buffer->height, (real32)buffer->height};
        int32 weapon_upper_left = bob_x + (buffer->width - (int32)weapon_sprite_size.x) / 2;
        int32 weapon_upper_top = 0 + bob_y;
        int32 weapon_lower_right = weapon_upper_left + (int32)weapon_sprite_size.x;

        Loaded_Image weapon_image = extract_image_from_sheet(&game_state->weapon_texture_sheet,
                                                             game_state->player.weapon_animation_index,
                                                             game_state->player.weapon);
        real32 texture_x = 0.0f;
        real32 texture_mapper = (real32)weapon_image.width / weapon_sprite_size.x;
        for (int32 dest_x = weapon_upper_left; dest_x < weapon_lower_right; ++dest_x)
        {
            copy_slice(buffer, &weapon_image, (int32)texture_x, dest_x, weapon_upper_top,
                       (int32)weapon_sprite_size.y, 0);
            texture_x += texture_mapper;
        }
    }

#if 0
    //
    //
    //Play sound
    if (game_state->need_to_play_pistol_sound)
    {
        memory->platform_play_sound("../data/pistol.wav");
        game_state->need_to_play_pistol_sound = false;
    }
#endif
}
