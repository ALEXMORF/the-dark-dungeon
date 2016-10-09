/*
 *TODO LIST:
 
 1. Procedure map generation
 2. Robust asset loading routine
 3. Async sound playback API with platform layer
 
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
load_assets(Game_State *game_state, Platform_Load_Image *platform_load_image,
            Platform_Load_Audio *platform_load_audio)
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

    game_state->pistol_sound = load_audio(platform_load_audio, "../data/pistol.wav");
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
        
        load_assets(game_state, memory->platform_load_image, memory->platform_load_audio);
        
        Render_Context *render_context = &game_state->render_context;
        render_context->z_buffer = Push_Array(&game_state->permanent_allocator, buffer->width, real32);
        render_context->floorcast_table_count = buffer->height/2;
        render_context->floorcast_table = Push_Array(&game_state->permanent_allocator, render_context->floorcast_table_count, real32);
        for (int32 i = 0; i < render_context->floorcast_table_count; ++i)
        {
            real32 inverse_aspect_ratio = (real32)buffer->width / (real32)buffer->height;
            real32 real_scan_y = ((real32)buffer->height/2 + (real32)i / inverse_aspect_ratio);
            render_context->floorcast_table[i] = (real32)buffer->height / (2*real_scan_y - buffer->height);
        }
        
        memory->is_initialized = true;
    }
    game_state->transient_allocator.used = 0;
    
    simulate_world(game_state, input);

    //output sound
    if (game_state->player.has_fired)
    {
        game_state->audio_task_list.add_task(&game_state->pistol_sound);
    }
    
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
                                                         0, 
                                                         &game_state->wall_textures,
                                                         sprite_list.content, sprite_list.count,
                                                         memory->platform_eight_async_proc);
    
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
}

//////////////////////////////////////////////////
// Sound stuff
//////////////////////////////////////////////////
void Audio_Task_List::add_task(Loaded_Audio *loaded_audio)
{
    assert(length < AUDIO_TASK_MAX);
    
    content[length].loaded_audio = loaded_audio;
    content[length].current_position = 0;
    content[length].is_finished = false;
    ++length;
}

void Audio_Task_List::remove_task(int index)
{
    assert(index >= 0 && index < AUDIO_TASK_MAX);
    assert(length > 0);
    
    content[index] = content[--length];
}

extern "C" GAME_PROCESS_SOUND(game_process_sound)
{
    Game_State *game_state = (Game_State *)memory->permanent_storage;
    Audio_Task_List *audio_task_list = &game_state->audio_task_list;
    real32 audio_volume = 0.5f;
        
    //clear buffer
    {
        int16 *sample_out = (int16 *)buffer->memory;
            for (int i = 0; i < buffer->sample_count; ++i)
        {
            *sample_out++ = 0;
            *sample_out++ = 0;
        }
    }
    
    for (int i = 0; i < audio_task_list->length; ++i)
    {
        //grab stuff
        Audio_Task *current_task = &audio_task_list->content[i];
        Loaded_Audio *current_audio = current_task->loaded_audio;
        
        //safety check output format
        assert(current_audio->channels == 2);
        assert(current_audio->byte_per_sample == 2);
        
        //advance current task cursor and calculate amount of samples to write
        int16 *audio_samples = ((int16 *)current_audio->memory +
                                (current_task->current_position*current_audio->channels));
        int32 samples_to_write = buffer->sample_count;
        int32 sample_count = current_audio->byte_size / current_audio->byte_per_sample / 2;
        int32 samples_left = sample_count - current_task->current_position;
        if (samples_left < samples_to_write)
        {
            samples_to_write = samples_left;
            current_task->is_finished = true;
        }
        current_task->current_position += samples_to_write;
        
        //TODO(chen):clip/interpolate the sound when it exceeds 16 bit
        int16 *sample_out = (int16 *)buffer->memory;
        for (int sample_index = 0; sample_index < samples_to_write; ++sample_index)
        {
            *sample_out++ += (int16)(*audio_samples++ * audio_volume);
            *sample_out++ += (int16)(*audio_samples++ * audio_volume);
        }
    }

    //remove finished tasks
    for (int i = 0; i < audio_task_list->length; ++i)
    {
        if (audio_task_list->content[i].is_finished)
        {
            audio_task_list->remove_task(i);
            --i;
        }
    }
    
    //sine wave generation
#if 0    
    int32 sample_per_second = 48000;
    int16 tone_hz = 256;
    int16 tone_volume = 1000;

    uint32 sine_wave_period = sample_per_second / tone_hz;

    int16 *sample_out = (int16 *)buffer->memory;
    for (int i = 0; i < buffer->sample_count; ++i)
    {
        real32 sine_percentage = (real32)buffer->running_sample_index++ / (real32)sine_wave_period;
        if (buffer->running_sample_index > sine_wave_period)
        {
            buffer->running_sample_index -= sine_wave_period;
        }
        
        int16 sample_value = (int16)(sinf(sine_percentage * pi32 * 2) * tone_volume);
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;
    }
#endif
}
