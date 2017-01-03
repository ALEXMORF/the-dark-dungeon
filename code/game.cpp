/*
 *TODO LIST:
 
 Future Features:
 . finish up the world generation 
 . add ui and multiple game states
 
 TODO BUGS:
 . cast_ray() function sometimes returns non-valid result,
   tentative fix by inclusively determining quadrants, see how it helps.

   bullet detection might be the cause of bug, since every lag is preceded by
   player firing at enemey
*/

#include "game.h"

#include "game_math.cpp"
#include "game_random.cpp"
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

#include "game_world.cpp"

inline void
load_assets(Game_Asset *game_asset, Linear_Allocator *allocator,
            Platform_Load_Image *platform_load_image, Platform_Load_Audio *platform_load_audio)
{
    DBuffer(Loaded_Image) *wall_texture_buffer = &game_asset->wall_texture_buffer;
    wall_texture_buffer->capacity = 6;
    wall_texture_buffer->e = Push_Array(allocator, wall_texture_buffer->capacity, Loaded_Image);
    
    char *texture_filenames[] = {
        "../data/redbrick.png",
        "../data/bluestone.png",
        "../data/colorstone.png",
        "../data/eagle.png",
        "../data/purplestone.png",
        "../data/wood.png",
        "../data/mossy.png",
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

internal void
update_game_state(World *world, Game_Input *input)
{
    real32 dt = input->dt_per_frame;
    Player *player = &world->player;
    
    //update player
    {
        player_input_process(player, input);
        if (player->get_weapon()->cd_counter != 0)
        {
            player->get_weapon()->cd_counter = reduce(player->get_weapon()->cd_counter, dt);
        }
    }
    
    //update entities
    for (int32 i = 0; i < world->entity_buffer.count; ++i)
    {
        Entity *entity = &world->entity_buffer.e[i];
        if (entity->is_static) continue;
        
        switch (entity->type)
        {
            case ENTITY_TYPE_GUARD:
            case ENTITY_TYPE_SS:
            {
                update_basic_entity(entity, &world->tile_map, player->body.position, dt);
            } break;
        }
    }
    
    //check for player being hit by bullets
    for (int32 i = 0; i < world->entity_buffer.count; ++i)
    {
        Entity *entity = &world->entity_buffer.e[i];
        if (entity->state == aiming_state)
        {
            Aiming_State *aiming_state = (Aiming_State *)entity->variant_block.storage;
            if (aiming_state->just_fired)
            {
                Line_Segment bullet_line = {};
                bullet_line.start = entity->body.position;
                bullet_line.end = cast_ray(&world->tile_map, entity->body.position, entity->angle).hit_position;
                
                Circle player_hitbox = {};
                player_hitbox.position = player->body.position;
                player_hitbox.radius = player->body.collision_radius;
                
                if (line_vs_circle(bullet_line, player_hitbox))
                {
                    if (player->hp > 0)
                    {
                        player->transient_flags |= PLAYER_FLAG_IS_DAMAGED;
                        player->hp -= 1;
                    }

                    v2 bullet_direction = normalize(bullet_line.end - bullet_line.start);
                    player->body.force_to_apply = bullet_direction * entity->weapon_force;
                }
            }
        }
    }
    
    //check which entities is shot by player
    if (player->transient_flags & PLAYER_FLAG_HAS_FIRED)
    {
        Line_Segment bullet_line = {};
        bullet_line.start = player->body.position;
        bullet_line.end = cast_ray(&world->tile_map, player->body.position, player->angle).hit_position;
        
        for (int i = 0; i < world->entity_buffer.count; ++i)
        {
            Entity *entity = &world->entity_buffer.e[i];
            if (entity->is_static) continue;
            
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

    //check player vs collectables
    for (int i = 0; i < world->entity_buffer.count; ++i)
    {
        Entity *entity = &world->entity_buffer.e[i];

        real32 collision_dist = entity->body.collision_radius + player->body.collision_radius;
        collision_dist *= collision_dist;
        
        real32 dist = len_squared(entity->body.position - player->body.position);
        
        if (dist < collision_dist)
        {
            bool32 remove_entity = true;

            switch (entity->type)
            {
                case ENTITY_TYPE_HEALTHPACK:
                {
                    player->transient_flags |= PLAYER_FLAG_IS_HEALED;
                    
                    player->hp += (player->hp < PLAYER_MAX_HP - 4?
                                   4: PLAYER_MAX_HP - player->hp );
                } break;
                
                case ENTITY_TYPE_PISTOL_AMMO:
                {
                    player->transient_flags |= PLAYER_FLAG_GET_AMMO;
                    
                    player->weapons[pistol].bank_ammo += 16;
                } break;
                
                case ENTITY_TYPE_RIFLE_AMMO:
                {
                    player->transient_flags |= PLAYER_FLAG_GET_AMMO;
                    
                    player->weapons[rifle].bank_ammo += 60;
                } break;

                case ENTITY_TYPE_MINIGUN_AMMO:
                {
                    player->transient_flags |= PLAYER_FLAG_GET_AMMO;
                    
                    player->weapons[minigun].bank_ammo += 100;
                } break;

                default:
                {
                    remove_entity = false;
                } break;
            }

            if (remove_entity)
            {
                remove_Entity(&world->entity_buffer, i--);
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
         
         //system stuff
         {
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
         }
         
         World *world = &game_state->world;
         {
             world->entity_buffer.capacity = 1000;
             world->entity_buffer.e = Push_Array(&game_state->permanent_allocator, world->entity_buffer.capacity, Entity);
             generate_world(world, &world->entity_buffer,
                            &game_state->asset,
                            &game_state->permanent_allocator,
                            &game_state->transient_allocator);
         }
         
         memory->is_initialized = true;
     }
     game_state->transient_allocator.used = 0;
     World *world = &game_state->world;
     
     update_game_state(world, input);
     
     //physics backend
     {
         Tile_Map *tile_map = &world->tile_map;
         DBuffer(Entity) *entity_buffer = &world->entity_buffer;
         Player *player = &world->player;

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
         if (world->player.transient_flags & PLAYER_FLAG_HAS_FIRED)
         {
             switch (world->player.get_weapon()->type)
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
         if (world->player.get_weapon()->reload_time == world->player.get_weapon()->max_reload_time)
         {
             audio_system->push_task(&game_asset->pistol_reload_sound, 1.0f);
         }

         //enemy fire sound
         for (int i = 0; i < world->entity_buffer.count; ++i)
         {
             Entity *entity = (Entity *)&world->entity_buffer.e[i];
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
     sprite_list.capacity = world->entity_buffer.capacity;
     sprite_list.content = Push_Array(&game_state->transient_allocator, sprite_list.capacity, Sprite);

     //draw 3d scene and sprites
     generate_sprite_list(game_asset, &sprite_list, &world->entity_buffer, world->player.angle);
     sort_sprites(sprite_list.content, sprite_list.count, world->player.body.position);
     render_3d_scene(buffer, &game_state->render_context, &world->tile_map,
                     world->player.body.position, world->player.angle, 
                     &game_asset->floor_texture, &game_asset->ceiling_texture,
                     &game_asset->wall_texture_buffer, sprite_list.content, sprite_list.count,
                     memory->platform_eight_async_proc);

     //animate first-person weapon
     {
         Player *player = &world->player;

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
         Player *player = &world->player;

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
         world->player.pace += len(world->player.body.velocity);
         int32 bob_x = (int32)(sinf(world->player.pace * 2.5f) * x_scale);
         int32 bob_y = (int32)(cosf(world->player.pace * 5.0f) * y_scale) + (int32)y_scale;

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
         //draw effect (hurt, healed, etc)
         if (game_state->hud_effect_last_time == 0.0f)
         {
             game_state->hud_effect_last_time = HUD_EFFECT_LAST_TIME;
             
             if (world->player.transient_flags & PLAYER_FLAG_IS_DAMAGED)
             {
                 game_state->hud_effect_color = 0x00ff0000;
             }
             else if (world->player.transient_flags & PLAYER_FLAG_IS_HEALED)
             {
                 game_state->hud_effect_color = 0x0000ff00;
             }
             else if (world->player.transient_flags & PLAYER_FLAG_GET_AMMO)
             {
                 game_state->hud_effect_color = 0x00ffff00;
             }
             //NOTE(chen): if no effect should take place
             else
             {
                 game_state->hud_effect_last_time = 0.0f;
             }
         }
         else
         {
             fill_screen(buffer, game_state->hud_effect_color, 50);
             game_state->hud_effect_last_time = reduce(game_state->hud_effect_last_time, input->dt_per_frame);
         }
         
         String_Drawer str_drawer = {};
         str_drawer.font_sheet = &game_asset->font_bitmap_sheet;
         str_drawer.buffer = buffer;

         //hp-bar
         {
             v2 min = {100, 10};
             v2 max = {500, 40};

             real32 width_per_hp = (real32)((max.x - min.x) / PLAYER_MAX_HP);
             real32 lerp_ratio = 3.0f * input->dt_per_frame;
             real32 hp_count = clamp((real32)world->player.hp, 0.0f, (real32)PLAYER_MAX_HP);
             
             real32 target_hp_display_width = width_per_hp * hp_count;
             game_state->hp_display_width = lerp(game_state->hp_display_width, target_hp_display_width, lerp_ratio);

             draw_string(&str_drawer, 10, 10, 120, 40, "HP: ");
             draw_rectangle(buffer, min.x, min.y, max.x, max.y, 0x00550000);
             draw_rectangle(buffer, min.x, min.y, min.x + game_state->hp_display_width, max.y, 0x00ff0000);
         }
         
         //player stat report
         {
             int32 min_x = 650;
             int32 min_y = 500;
             int32 font_size = 20;

             draw_string_autosized(&str_drawer, min_x, min_y, font_size, font_size,
                                   "ammo: %d / %d", world->player.get_weapon()->cache_ammo, world->player.get_weapon()->bank_ammo);
         }
         
         int32 font_size = 13;
         int32 layout_x = 10;
         int32 layout_height = 80;
         int32 layout_dheight = 30;
         
         {   //performance layout    
             print("Performance:");
             print(" process time: %.2fms", debug_state->last_frame_process_time);
             print(" mtsc: %lld cycles", debug_state->last_frame_mtsc);
         }
         print("");
         
         {   //world info
             print("World:");

             int32 enemy_total_count = 0;
             int32 enemy_alive_count = 0;
             {
                 for (int32 i = 0; i < game_state->world.entity_buffer.count; ++i)
                 {
                     Entity *entity = &game_state->world.entity_buffer.e[i];
                     if (entity->type == ENTITY_TYPE_GUARD || entity->type == ENTITY_TYPE_SS)
                     {
                         ++enemy_total_count;
                         if (entity->hp > 0)
                         {
                             ++enemy_alive_count;
                         }
                     }
                 }
             }
             
             print(" Enemy count: %d/%d", enemy_alive_count, enemy_total_count);
             print(" Player Info:");
             print("  position:(%.1f, %.1f)", game_state->world.player.body.position.x,game_state->world.player.body.position.y); 
         }
     }
 }

 extern "C" GAME_PROCESS_SOUND(game_process_sound)
 {
     assert(sizeof(Game_State) <= memory->permanent_memory.size);
     Game_State *game_state = (Game_State *)memory->permanent_memory.storage;
     Audio_System *audio_system = &game_state->audio_system;
     real32 master_audio_volume = 0.07f;

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
