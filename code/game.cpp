/*
 *TODO LIST:
 
 . Fix the sprite generation duplicate code
 . Procedure map generation
 . add ui and multiple game states
 . Robust asset loading routine

 TODO BUGS:

 . cast_ray() function sometimes returns non-valid result
 
 TODO LINGERING:

 . A bug where game freezes for 3 seconds then slowly recovers (sometimes even crashes)
 . Fix the audio engine's clipping issue
 . Fix the audio engine's temporal issue (place it on a separate thread)
*/

#include "game.h"
 
#include "game_math.cpp"
#include "game_tiles.cpp"
#include "game_memory.cpp"

#include "game_asset.cpp"
#include "game_render.cpp"
#include "game_sprite.cpp"
#include "game_raycaster.cpp"
#include "game_audio.cpp"

#include "game_entity.cpp"
#include "game_simulate.cpp"

//
//
//Game Code

inline void
load_assets(Game_State *game_state, Platform_Load_Image *platform_load_image,
            Platform_Load_Audio *platform_load_audio)
{
    DBuffer(Loaded_Image) *wall_texture_buffer = &game_state->wall_texture_buffer;
    wall_texture_buffer->capacity = 6;
    wall_texture_buffer->e = Push_Array(&game_state->permanent_allocator, wall_texture_buffer->capacity, Loaded_Image);
    add_Loaded_Image(wall_texture_buffer, load_image(platform_load_image, "../data/redbrick.png"));
    add_Loaded_Image(wall_texture_buffer, load_image(platform_load_image, "../data/bluestone.png"));
    add_Loaded_Image(wall_texture_buffer, load_image(platform_load_image, "../data/colorstone.png"));
    add_Loaded_Image(wall_texture_buffer, load_image(platform_load_image, "../data/eagle.png"));
    add_Loaded_Image(wall_texture_buffer, load_image(platform_load_image, "../data/purplestone.png"));
    add_Loaded_Image(wall_texture_buffer, load_image(platform_load_image, "../data/wood.png"));
    
    game_state->floor_texture = load_image(platform_load_image, "../data/greystone.png");
    game_state->ceiling_texture = load_image(platform_load_image, "../data/greystone.png");
    game_state->barrel_texture = load_image(platform_load_image, "../data/barrel.png");
    game_state->pillar_texture = load_image(platform_load_image, "../data/pillar.png");
    game_state->light_texture = load_image(platform_load_image, "../data/greenlight.png");

    game_state->weapon_texture_sheet = load_image_sheet(platform_load_image, "../data/weapons.png");
    config_image_sheet(&game_state->weapon_texture_sheet, 5, 5, 1, 64, 64);

    game_state->guard_texture_sheet = load_image_sheet(platform_load_image, "../data/guard.png");
    config_image_sheet(&game_state->guard_texture_sheet, 8, 7, 1, 64, 64);

    game_state->ss_texture_sheet = load_image_sheet(platform_load_image, "../data/ss.png");
    config_image_sheet(&game_state->ss_texture_sheet, 8, 7, 1, 63, 63);

    game_state->font_bitmap_sheet = load_image_sheet(platform_load_image, "../data/font_8x8.png");
    auto_config_image_sheet(&game_state->font_bitmap_sheet, 77, 1);

    game_state->pistol_sound = load_audio(platform_load_audio, "../data/pistol.wav");
    game_state->pistol2_sound = load_audio(platform_load_audio, "../data/pistol2.wav");
    game_state->background_music = load_audio(platform_load_audio, "../data/background1.wav");
}

inline void
fill_entities(Linear_Allocator *allocator, DBuffer(Entity) *entity_buffer)
{
    add_Entity(entity_buffer, make_dynamic_entity(allocator, guard, {6.0f, 15.5f}));
    add_Entity(entity_buffer, make_dynamic_entity(allocator, guard, {15.0f, 7.0f}, pi32));
    add_Entity(entity_buffer, make_dynamic_entity(allocator, guard, {6.0f, 7.0f}));
    add_Entity(entity_buffer, make_dynamic_entity(allocator, ss, {15.0f, 6.0f}, pi32));
    add_Entity(entity_buffer, make_dynamic_entity(allocator, ss, {15.0f, 8.0f}, pi32));
    add_Entity(entity_buffer, make_dynamic_entity(allocator, ss, {15.0f, 9.0f}));
    add_Entity(entity_buffer, make_dynamic_entity(allocator, ss, {15.0f, 15.0f}, pi32));
    add_Entity(entity_buffer, make_dynamic_entity(allocator, ss, {14.0f, 15.0f}));
    add_Entity(entity_buffer, make_dynamic_entity(allocator, ss, {16.0f, 15.0f}));
}

inline int
get_string_length(char *str)
{
    int result = 0;
    while (*str++)
    {
        result += 1;
    }
    return result;
}

inline int
get_font_index(char character)
{
    if (character >= 'a' && character <= 'z')
    {
        character += 'A' - 'a';
    }
    assert(character >= '(' && character <= 'Z');
    
    char initial_character = '(';
    int start_index_offset = 8;
    
    return start_index_offset + (character - initial_character);
}

#define draw_string(buffer, font_sheet, min_x, min_y, max_x, max_y, string_format, ...) { char string[255] = {}; snprintf(string, sizeof(string), string_format, ##__VA_ARGS__); draw_string_unformatted(buffer, font_sheet, min_x, min_y, max_x, max_y, string); } 
#define draw_string_autosized(buffer, font_sheet, min_x, min_y, font_width, font_height, string_format, ...) { char string[255] = {}; snprintf(string, sizeof(string), string_format, ##__VA_ARGS__); draw_string_unformatted(buffer, font_sheet, min_x, min_y, min_x + font_width * get_string_length(string), min_y + font_height, string); } 
internal void
draw_string_unformatted(Game_Offscreen_Buffer *buffer, Loaded_Image_Sheet *font_sheet, int min_x, int min_y, int max_x, int max_y,
                        char *string)
{
    if_do(min_x < 0, min_x = 0);
    if_do(min_y < 0, min_y = 0);
    if_do(max_x > buffer->width, max_x = buffer->width);
    if_do(max_y > buffer->height, max_y = buffer->height);

    int string_length = get_string_length(string);
    int bitmap_width = (int32)(((real32)max_x - min_x) / string_length);
    
    for (int i = 0; i < string_length; ++i)
    {
        if (string[i] == ' ')
        {
            continue;
        }
        
        int bitmap_index = get_font_index(string[i]);
        int bitmap_min_x = min_x + bitmap_width * i;
        
        Loaded_Image font_image = extract_image_from_sheet(font_sheet, bitmap_index, 0);
        draw_bitmap(buffer, &font_image, bitmap_min_x, min_y, bitmap_min_x + bitmap_width, max_y);
    }
}
    
extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    assert(sizeof(Game_State) <= memory->permanent_memory.size);
    Game_State *game_state = (Game_State *)memory->permanent_memory.storage;
    
    if (!memory->is_initialized)
    {
        initialize_linear_allocator(&game_state->permanent_allocator,
                                    (uint8 *)memory->permanent_memory.storage + sizeof(Game_State),
                                    memory->permanent_memory.size - sizeof(Game_State));
        initialize_linear_allocator(&game_state->transient_allocator,
                                    (uint8 *)memory->transient_memory.storage,
                                    memory->transient_memory.size);
        
//TODO(chen): this is some improv tile-map init code, replace this with procedural generation later
#define map_width 20
#define map_height 20
        uint32 temp_tiles[map_width*map_height] =
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
        Copy_Array(temp_tiles, tile_map->tiles, tile_count, uint32);

        initialize_player(&game_state->player);

        game_state->entity_buffer.capacity = ENTITY_COUNT_LIMIT;
        game_state->entity_buffer.e = Push_Array(&game_state->permanent_allocator, game_state->entity_buffer.capacity, Entity);
        fill_entities(&game_state->permanent_allocator, &game_state->entity_buffer);
        
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
        
        Audio_System *audio_system = &game_state->audio_system;
        audio_system->push_task_looped(&game_state->background_music);

        memory->is_initialized = true;
    }
    game_state->transient_allocator.used = 0;
    
    simulate_world(game_state, input);
    
    //output sound
    if (game_state->player.has_fired)
    {
        game_state->audio_system.push_task(&game_state->pistol2_sound, 1.0f);
    }
    for (int i = 0; i < game_state->entity_buffer.count; ++i)
    {
        Entity *entity = (Entity *)&game_state->entity_buffer.e[i];
        if (entity->state == aiming_state)
        {
            Aiming_State *aiming_state = (Aiming_State *)entity->variant_block.storage;
            if (aiming_state->just_fired)
            {
                game_state->audio_system.push_task(&game_state->pistol_sound, 0.3f);
            }
        }
    }
    
    //
    //
    //Render game
    fill_buffer(buffer, 0);
    
    Sprite_List sprite_list = {};
    sprite_list.capacity = game_state->entity_buffer.capacity;
    sprite_list.content = Push_Array(&game_state->transient_allocator, sprite_list.capacity, Sprite);

    //draw 3d scene and sprites
    generate_sprite_list(game_state, &sprite_list, game_state->entity_buffer.e, game_state->entity_buffer.count);
    sort_sprites(sprite_list.content, sprite_list.count, game_state->player.position);
    render_3d_scene(buffer, &game_state->render_context, &game_state->tile_map,
                    game_state->player.position, game_state->player.angle, 
                    &game_state->floor_texture, &game_state->ceiling_texture,
                    &game_state->wall_texture_buffer, sprite_list.content, sprite_list.count,
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
                                                             game_state->player.weapon_type);
        real32 texture_x = 0.0f;
        real32 texture_mapper = (real32)weapon_image.width / weapon_sprite_size.x;
        for (int32 dest_x = weapon_upper_left; dest_x < weapon_lower_right; ++dest_x)
        {
            copy_slice(buffer, &weapon_image, (int32)texture_x, dest_x, weapon_upper_top,
                       (int32)weapon_sprite_size.y, 0);
            texture_x += texture_mapper;
        }
    }

    //HUD overlay
    {
        //hp-bar
        {
            real32 min_x = 100;
            real32 max_x = 500;
            real32 min_y = 10;
            real32 max_y = 40;
            
            real32 width_per_hp = (real32)((max_x - min_x) / PLAYER_MAX_HP);
            real32 lerp_ratio = 3.0f * input->dt_per_frame;
            real32 hp_count = clamp((real32)game_state->player.hp, 0.0f, (real32)PLAYER_MAX_HP);
            
            real32 target_hp_display_width = width_per_hp * hp_count;
            game_state->hp_display_width = lerp(game_state->hp_display_width, target_hp_display_width, lerp_ratio);

            draw_string(buffer, &game_state->font_bitmap_sheet, 10, 10, 120, 40, "HP: ");
            draw_rectangle(buffer, (int32)min_x, (int32)min_y, (int32)max_x, (int32)max_y, 0x00550000);
            draw_rectangle(buffer, (int32)min_x, (int32)min_y, (int32)min_x + (int32)game_state->hp_display_width, (int32)max_y, 0x00ff0000);
        }
        
        //debug info
        {
            int32 layout_height = 80;
            int32 layout_dheight = 30;
            
            draw_string_autosized(buffer, &game_state->font_bitmap_sheet, 10, layout_height, 15, 15, "DEBUG:");
            layout_height += layout_dheight;
            
            draw_string_autosized(buffer, &game_state->font_bitmap_sheet, 10, layout_height, 15, 15,
                                  "process time: %.2fms", debug_state->last_frame_process_time);
            layout_height += layout_dheight;
            
            draw_string_autosized(buffer, &game_state->font_bitmap_sheet, 10, layout_height, 15, 15,
                                  "mtsc: %lld cycles", debug_state->last_frame_mtsc);
            layout_height += layout_dheight;
        }
    }
}

extern "C" GAME_PROCESS_SOUND(game_process_sound)
{
    Game_State *game_state = (Game_State *)memory->permanent_memory.storage;
    Audio_System *audio_system = &game_state->audio_system;
    real32 master_audio_volume = 0.1f;
        
    //clear buffer
    {
        int32 *sample_out = (int32 *)buffer->memory;
        for (int i = 0; i < buffer->sample_count; ++i)
        {
            *sample_out++ = 0;
        }
    }
    
    for (int i = 0; i < audio_system->length; ++i)
    {
        //grab stuff
        Audio_Task *current_task = &audio_system->content[i];
        Loaded_Audio *current_audio = current_task->loaded_audio;
        int16 *audio_samples = ((int16 *)current_audio->memory +
                                (current_task->current_position*current_audio->channels));

        //safety check output format
        assert(current_audio->channels == 2 || current_audio->channels == 1);
        assert(current_audio->byte_per_sample == 2);
        
        //advance current task cursor and calculate amount of samples to write
        int32 samples_to_write = buffer->sample_count;
        {
            int32 sample_count = current_audio->byte_size / current_audio->byte_per_sample / 2;
            int32 samples_left = sample_count - current_task->current_position;
            if (samples_left < samples_to_write)
            {
                samples_to_write = samples_left;
                current_task->is_finished = true;
            }
            current_task->current_position += samples_to_write;
        }
        
        //TODO(chen):clip/interpolate the sound when it exceeds 16 bit
        //TODO(chen):interploate for audio clips that are lower than 44.1k sample frequency
        int16 *sample_out = (int16 *)buffer->memory;
        real32 volume = master_audio_volume * current_task->volume;
        for (int sample_index = 0; sample_index < samples_to_write; ++sample_index)
        {
            *sample_out++ += (int16)(*audio_samples * volume);
            *sample_out++ += (int16)(*audio_samples * volume);

            if (current_audio->channels == 2)
            {
                audio_samples += 2;
            }
            else if (current_audio->channels == 1)
            {
                audio_samples += 1;
            }
            else
            {
                assert(!"unexpected channel count");
            }
        }
    }

    //remove finished tasks
    for (int i = 0; i < audio_system->length; ++i)
    {
        Audio_Task *current_task = &audio_system->content[i];
        if (current_task->is_finished)
        {
            if (current_task->is_looping) 
            {
                current_task->is_finished = false;
                current_task->current_position = 0;
            }
            else 
            {
                audio_system->remove_task(i);
                --i;
            }
        }
    }

    ++buffer->running_sample_index;
}
