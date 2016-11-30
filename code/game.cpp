/*
 *TODO LIST:
 
 Code cleanness:
 . Extract out a physics system as service component??? YES!!
 . Replace unnecessary dynamic buffer with static buffer
 . Replace the sprite generation duplicate code with an animation system
 
 Gameplay: 
 . Add static entity initialization code -> finish make_static_entity() (decorations, collectables)
 . Add advanced entities (bosses)
 . Show the direction from where the damage comes from
 . Physics: add entity vs entity & entity vs player collision
 . Add more interactiviy (screen turns red when shot, enemies pushed back when shot, etc)
 . Make the difference between penetrating bullets and non-penetrating bullet
 
 Optimization:
 . Multithreaded rendering (simutaniously drawing 8 portions of the screen)
 
 Future Features:
 . Procedure map generation
 . add ui and multiple game states
 . Robust asset loading routine

 TODO BUGS:

 . cast_ray() function sometimes returns non-valid result,
   tentative fix by inclusively determining quadrants, see how it helps.
   
 TODO LINGERING:
 
 . A bug where game freezes for 3 seconds then slowly recovers (sometimes even crashes)
 . Fix the audio engine's clipping issue
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

#include "game_simulate.cpp"
#include "game_player.cpp"
#include "game_entity.cpp"
#include "game_ui.cpp"

inline void
load_assets(Game_Asset *game_asset, Linear_Allocator *allocator,
            Platform_Load_Image *platform_load_image, Platform_Load_Audio *platform_load_audio)
{
    DBuffer(Loaded_Image) *wall_texture_buffer = &game_asset->wall_texture_buffer;
    wall_texture_buffer->capacity = 6;
    wall_texture_buffer->e = Push_Array(allocator, wall_texture_buffer->capacity, Loaded_Image);
    
    char *texture_filenames[6] = {
        "../data/redbrick.png", "../data/bluestone.png", "../data/colorstone.png",
        "../data/eagle.png", "../data/purplestone.png", "../data/wood.png"
    };
    for (int i = 0; i < array_count(texture_filenames); ++i)
    {
        add_Loaded_Image(wall_texture_buffer, load_image(platform_load_image, texture_filenames[i]));
    }
    
    game_asset->floor_texture = load_image(platform_load_image, "../data/greystone.png");
    game_asset->ceiling_texture = load_image(platform_load_image, "../data/greystone.png");
    
    game_asset->weapon_texture_sheet = load_image_sheet(platform_load_image, "../data/weapons.png");
    config_image_sheet(&game_asset->weapon_texture_sheet, 5, 5, 1, 64, 64);
    
    game_asset->guard_texture_sheet = load_image_sheet(platform_load_image, "../data/guard.png");
    config_image_sheet(&game_asset->guard_texture_sheet, 8, 7, 1, 64, 64);

    game_asset->ss_texture_sheet = load_image_sheet(platform_load_image, "../data/ss.png");
    config_image_sheet(&game_asset->ss_texture_sheet, 8, 7, 1, 63, 63);
    
    game_asset->entity_sheet = load_image_sheet(platform_load_image, "../data/objects.png");
    config_image_sheet(&game_asset->entity_sheet, 5, 10, 1, 64, 64);

    game_asset->font_bitmap_sheet = load_image_sheet(platform_load_image, "../data/font_8x8.png");
    auto_config_image_sheet(&game_asset->font_bitmap_sheet, 77, 1);
    
    game_asset->pistol_sound = load_audio(platform_load_audio, "../data/pistol.wav");
    game_asset->pistol2_sound = load_audio(platform_load_audio, "../data/pistol2.wav");
    game_asset->pistol_reload_sound = load_audio(platform_load_audio, "../data/pistol_reload.wav");
    game_asset->rifle_sound = load_audio(platform_load_audio, "../data/rifle.wav");
    game_asset->minigun_sound = load_audio(platform_load_audio, "../data/minigun.wav");
    game_asset->background_music = load_audio(platform_load_audio, "../data/background1.wav");
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

internal void
update_game_state(Game_State *game_state, Game_Input *input)
{
    real32 dt = input->dt_per_frame;
    Player *player = &game_state->player;
    
    //update player
    {
        player_input_process(player, input);
        if (player->get_weapon()->cd_counter != 0)
        {
            player->get_weapon()->cd_counter = reduce(player->get_weapon()->cd_counter, dt);
        }
    }
    
    //update entities
    for (int32 i = 0; i < game_state->entity_buffer.count; ++i)
    {
        Entity *entity = &game_state->entity_buffer.e[i];
        switch (entity->type)
        {
            case guard:
            case ss:
            {
                update_basic_entity(entity, &game_state->tile_map, player->body.position, dt);
            } break;
        }
    }
    
    //check for player being hit by bullets
    for (int32 i = 0; i < game_state->entity_buffer.count; ++i)
    {
        Entity *entity = &game_state->entity_buffer.e[i];
        if (entity->state == aiming_state)
        {
            Aiming_State *aiming_state = (Aiming_State *)entity->variant_block.storage;
            if (aiming_state->just_fired)
            {
                Line_Segment bullet_line = {};
                bullet_line.start = entity->body.position;
                bullet_line.end = cast_ray(&game_state->tile_map, entity->body.position, entity->angle).hit_position;
                
                Circle player_hitbox = {};
                player_hitbox.position = player->body.position;
                player_hitbox.radius = player->body.collision_radius;
                
                if (line_vs_circle(bullet_line, player_hitbox))
                {
                    if (player->hp > 0)
                    {
                        player->hp -= 1;
                    }

                    v2 bullet_direction = normalize(bullet_line.end - bullet_line.start);
                    player->body.force_to_apply = bullet_direction * entity->weapon_force;
                }
            }
        }
    }
    
    //check which entities is damaged by player
    if (player->has_fired)
    {
        Line_Segment bullet_line = {};
        bullet_line.start = player->body.position;
        bullet_line.end = cast_ray(&game_state->tile_map, player->body.position, player->angle).hit_position;
        
        for (int i = 0; i < game_state->entity_buffer.count; ++i)
        {
            Entity *entity = &game_state->entity_buffer.e[i];
            
            Circle entity_hitbox = {};
            entity_hitbox.position = entity->body.position;
            entity_hitbox.radius = entity->body.collision_radius;
            
            if (line_vs_circle(bullet_line, entity_hitbox))
            {
                entity->hp -= 1;
                entity->is_damaged = true;

                v2 bullet_direction = normalize(bullet_line.end - bullet_line.start);
                entity->body.force_to_apply = bullet_direction * player->get_weapon()->force;
            }
        }
    }
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    assert(sizeof(Game_State) <= memory->permanent_memory.size);
    Game_State *game_state = (Game_State *)memory->permanent_memory.storage;
    Game_Asset *game_asset = &game_state->asset;
    
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
        
        load_assets(game_asset, &game_state->permanent_allocator,
                    memory->platform_load_image, memory->platform_load_audio);
        
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
        
        game_state->audio_system.push_task_looped(&game_asset->background_music);
        
        memory->is_initialized = true;
    }
    game_state->transient_allocator.used = 0;
    
    update_game_state(game_state, input);

    //physics backend
    {
        Tile_Map *tile_map = &game_state->tile_map;
        DBuffer(Entity) *entity_buffer = &game_state->entity_buffer;
        Player *player = &game_state->player;
        
        for (int i = 0; i < entity_buffer->count; ++i)
        {
            simulate_body(&entity_buffer->e[i].body, tile_map);
        }
        
        simulate_body(&player->body, tile_map);
    }
    
    //output sound
    {
        Audio_System *audio_system = &game_state->audio_system;
        
        //player fire sound
        if (game_state->player.has_fired)
        {
            switch (game_state->player.get_weapon()->type)
            {
                case pistol:
                {
                    audio_system->push_task(&game_asset->pistol2_sound, 1.0f);
                } break;
                
                case rifle:
                {
                    audio_system->push_task(&game_asset->rifle_sound, 1.0f);
                } break;

                case minigun:
                {
                    audio_system->push_task(&game_asset->minigun_sound, 1.0f);
                } break;

                default:
                {
                    assert(!"unknown weapon type");
                };
            }
        }
        
        //player reload sound
        if (game_state->player.get_weapon()->reload_time == game_state->player.get_weapon()->max_reload_time)
        {
            audio_system->push_task(&game_asset->pistol_reload_sound, 1.0f);
        }

        //enemy fire sound
        for (int i = 0; i < game_state->entity_buffer.count; ++i)
        {
            Entity *entity = (Entity *)&game_state->entity_buffer.e[i];
            if (entity->state == aiming_state)
            {
                Aiming_State *aiming_state = (Aiming_State *)entity->variant_block.storage;
                if (aiming_state->just_fired)
                {
                    audio_system->push_task(&game_asset->pistol_sound, 0.3f);
                }
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
    generate_sprite_list(game_asset, &sprite_list, &game_state->entity_buffer,
                         game_state->player.angle);
    sort_sprites(sprite_list.content, sprite_list.count, game_state->player.body.position);
    render_3d_scene(buffer, &game_state->render_context, &game_state->tile_map,
                    game_state->player.body.position, game_state->player.angle, 
                    &game_asset->floor_texture, &game_asset->ceiling_texture,
                    &game_asset->wall_texture_buffer, sprite_list.content, sprite_list.count,
                    memory->platform_eight_async_proc);
    
    //animate first-person weapon
    {
        Player *player = &game_state->player;

        //minigun animation
        if (player->get_weapon()->type == minigun)
        {
            real32 animation_firing_period = player->get_weapon()->cd - 0.02f;
            real32 animation_cooling_period = player->get_weapon()->cd;

            real32 firing_base_index = 1.0f;
            real32 firing_end_index = 3.0f;
            real32 cooling_base_index = 4.0f;
            real32 cooling_end_index = 0.0f;
            int32 total_index_count = 5;
            
            if (player->get_weapon()->cd_counter) //still firing
            {
                //set the animation index per frame
                real32 time_passed = player->get_weapon()->cd - player->get_weapon()->cd_counter;
                real32 firing_index_count = firing_end_index - firing_base_index + 1.0f;
                real32 animation_index_frequency = animation_firing_period / firing_index_count;
                int32 index_offset = (int32)(time_passed / animation_index_frequency);
                player->get_weapon()->animation_index =
                    (int32)(firing_base_index + index_offset - firing_base_index) %
                    ((int32)firing_end_index) + (int32)firing_base_index;
            }
            else 
            {
                player->get_weapon()->animation_index = 1;
            }
        }
        //default weapon animation
        else
        {
            real32 animation_period = player->get_weapon()->cd - 0.02f;
            int32 animation_ending_index = 1;
            real32 animation_index_count = 4.0f;
        
            if (player->get_weapon()->cd_counter)
            {
                real32 time_passed = player->get_weapon()->cd - player->get_weapon()->cd_counter;
                if (time_passed < animation_period)
                {
                    real32 animation_index_interval = animation_period / animation_index_count;
                    player->get_weapon()->animation_index =
                        (int32)(time_passed / animation_index_interval +
                                (animation_ending_index + 1)) % ((int32)animation_index_count + 1);
                }
                else
                {
                    player->get_weapon()->animation_index = animation_ending_index;
                }
            }
            else
            {
                player->get_weapon()->animation_index = animation_ending_index;
            }
        }
    }
    
    //draw first-person weapon
    {
        Player *player = &game_state->player;
        
        //calculate offset for reloading animation
        real32 animation_lerp = 0.1f;
        real32 max_reload_offset = 200.0f;
        if (player->get_weapon()->is_reloading)
        {
            player->weapon_reload_offset = lerp(player->weapon_reload_offset, max_reload_offset, animation_lerp);
        }
        else
        {
            player->weapon_reload_offset = lerp(player->weapon_reload_offset, 0.0f, animation_lerp);
        }
        
        //bob and render weapon sprite
        real32 y_scale = (real32)buffer->height / 50;
        real32 x_scale = (real32)buffer->width / 40;
        game_state->player.pace += len(game_state->player.body.velocity);
        int32 bob_x = (int32)(sinf(game_state->player.pace * 2.5f) * x_scale);
        int32 bob_y = (int32)(cosf(game_state->player.pace * 5.0f) * y_scale) + (int32)y_scale;
        
        v2 weapon_sprite_size = {(real32)buffer->height, (real32)buffer->height};
        int32 weapon_upper_left = bob_x + (buffer->width - (int32)weapon_sprite_size.x) / 2;
        int32 weapon_upper_top = 0 + bob_y + (int32)player->weapon_reload_offset;
        int32 weapon_lower_right = weapon_upper_left + (int32)weapon_sprite_size.x;
        
        Loaded_Image weapon_image = extract_image_from_sheet(&game_asset->weapon_texture_sheet,
                                                             player->get_weapon()->animation_index,
                                                             player->get_weapon()->type);
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
        String_Drawer str_drawer = {};
        str_drawer.font_sheet = &game_asset->font_bitmap_sheet;
        str_drawer.buffer = buffer;
            
        //hp-bar
        {
            v2 min = {100, 10};
            v2 max = {500, 40};
            
            real32 width_per_hp = (real32)((max.x - min.x) / PLAYER_MAX_HP);
            real32 lerp_ratio = 3.0f * input->dt_per_frame;
            real32 hp_count = clamp((real32)game_state->player.hp, 0.0f, (real32)PLAYER_MAX_HP);
            
            real32 target_hp_display_width = width_per_hp * hp_count;
            game_state->hp_display_width = lerp(game_state->hp_display_width, target_hp_display_width, lerp_ratio);
            
            draw_string(&str_drawer, 10, 10, 120, 40, "HP: ");
            draw_rectangle(buffer, min.x, min.y, max.x, max.y, 0x00550000);
            draw_rectangle(buffer, min.x, min.y, min.x + game_state->hp_display_width, max.y, 0x00ff0000);
        }
        
        //ammo count
        {
            int32 min_x = 650;
            int32 min_y = 500;
            int32 font_size = 20;
            
            draw_string_autosized(&str_drawer, min_x, min_y, font_size, font_size,
                                  "ammo: %d / %d", game_state->player.get_weapon()->cache_ammo, game_state->player.get_weapon()->bank_ammo);
        }

        //performance layout
        {
            int32 font_size = 13;
            int32 layout_x = 10;
            int32 layout_height = 80;
            int32 layout_dheight = 30;
            
            print("DEBUG:");
            print(" process time: %.2fms", debug_state->last_frame_process_time);
            print(" mtsc: %lld cycles", debug_state->last_frame_mtsc);
            
            print("Audio System:");
            print(" task count/capacity: %d/%d", game_state->audio_system.length, AUDIO_TASK_MAX);
            
            print("Entity System:");
            print(" entity count/capacity: %d/%d", game_state->entity_buffer.count, game_state->entity_buffer.capacity);
        }
    }
}

extern "C" GAME_PROCESS_SOUND(game_process_sound)
{
    assert(sizeof(Game_State) <= memory->permanent_memory.size);
    Game_State *game_state = (Game_State *)memory->permanent_memory.storage;
    Audio_System *audio_system = &game_state->audio_system;
    real32 master_audio_volume = 0.3f;
    
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
