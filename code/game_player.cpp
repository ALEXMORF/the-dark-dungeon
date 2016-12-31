#include "game_player.h"

//////////////////////////////////////////////////
// Weapons
//////////////////////////////////////////////////

internal void
initialize_weapon(Weapon *weapon, Weapon_Type weapon_type)
{
    weapon->animation_index = 1;
    weapon->type = weapon_type;

    switch (weapon->type)
    {
        case pistol:
        {
            weapon->animation_index = 1;
            weapon->cd = 0.3f;
    
            weapon->max_reload_time = 1.0f;
            weapon->cache_max_ammo = weapon->cache_ammo = 8;
            weapon->bank_ammo = 32;

            weapon->force = 100.0f;
        } break;
        
        case rifle:
        {
            weapon->animation_index = 1;
            weapon->cd = 0.1f;
    
            weapon->max_reload_time = 1.5f;
            weapon->cache_max_ammo = weapon->cache_ammo = 30;
            weapon->bank_ammo = 90;

            weapon->force = 100.0f;
        } break;

        case minigun:
        {
            weapon->animation_index = 1;
            weapon->cd = 0.08f;
    
            weapon->max_reload_time = 2.0f;
            weapon->cache_max_ammo = weapon->cache_ammo = 100;
            weapon->bank_ammo = 300;

            weapon->force = 250.0f;
        } break;
        
        default:
        {
            assert(!"weapon type exhuasted");
        } break;
    }
}

internal void
weapon_start_reload(Weapon *weapon)
{
    if (weapon->bank_ammo > 0)
    {
        weapon->is_reloading = true;
        weapon->reload_time = weapon->max_reload_time;
    }
}

//////////////////////////////////////////////////
// Player logic
//////////////////////////////////////////////////

internal void
initialize_player(Player *player, v2 position)
{
    player->hp = PLAYER_MAX_HP;
    
    player->body = default_rigid_body(position, 0.3f);
    player->angle = 0.0f;
    
    initialize_weapon(&player->weapons[pistol], pistol);
    initialize_weapon(&player->weapons[rifle], rifle);
    initialize_weapon(&player->weapons[minigun], minigun);
    player->weapon_index = rifle;
}

internal void
player_input_process(Player *player, Game_Input *input)
{
    player->transient_flags = 0;
    
    //movement
    {
        real32 player_speed = 3.0f;
        real32 lerp_constant = 0.15f;
        real32 mouse_sensitivity = 0.7f;
        
        real32 forward = 0.0f;
        real32 left = 0.0f;
        if_do(input->keyboard.left, left = 1.0f);
        if_do(input->keyboard.right, left = -1.0f);
        if_do(input->keyboard.up, forward = 1.0f);
        if_do(input->keyboard.down, forward = -1.0f);
        
        v2 player_d_velocity = {};
        player_d_velocity += {cosf(player->angle) *forward, sinf(player->angle) * forward};    
        player_d_velocity += {cosf(player->angle + pi32/2.0f) * left, sinf(player->angle + pi32/2.0f) * left};    
        player_d_velocity = normalize(player_d_velocity);
        player_d_velocity *= player_speed * input->dt_per_frame;
        player->body.velocity_to_apply = player_d_velocity;
        
        //orientation
        real32 player_delta_angle = -input->mouse.dx / 500.0f * pi32/3.0f * mouse_sensitivity; //NOTE(chen): I don't know what this crap is, fix that maybe?
        player->angle += player_delta_angle;
        recanonicalize_angle(&player->angle);
    }
    
    //weapon switch
    for (int i = 0; i < 4; ++i)
    {
        if (input->keyboard.number[i] && player->get_weapon()->type != i)
        {
            if (player->get_weapon()->is_reloading)
            {
                //cancel reload
                player->get_weapon()->is_reloading = false;
                player->get_weapon()->reload_time = 0.0f;
            }
            player->weapon_index = i;
            break;
        }
    }
    
    //firing system
    {
        Weapon *weapon = player->get_weapon();
        
        if (weapon->is_reloading == false && input->keyboard.R)
        {
            weapon_start_reload(weapon);
            return;
        }
        
        if (weapon->is_reloading)
        {
            weapon->reload_time = reduce(weapon->reload_time, input->dt_per_frame);
            if (weapon->reload_time <= 0.0f)
            {
                //finish reloadng
                weapon->is_reloading = false;
                
                int32 ammo_to_refill = weapon->cache_max_ammo - weapon->cache_ammo;
                if (ammo_to_refill > weapon->bank_ammo)
                {
                    ammo_to_refill = weapon->bank_ammo;
                }
    
                weapon->bank_ammo -= ammo_to_refill;
                weapon->cache_ammo += ammo_to_refill;
                weapon->reload_time = 0.0f;
            }
            return;
        }
        
        if (input->mouse.down && weapon->cd_counter == 0.0f)
        {
            if (weapon->cache_ammo > 0)
            {
                weapon->cd_counter = weapon->cd;
                player->transient_flags |= PLAYER_FLAG_HAS_FIRED;
                weapon->cache_ammo -= 1;
            }
            //reload
            else
            {
                weapon_start_reload(weapon);
            }
        }
    }
}
