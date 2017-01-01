# the-dark-dungeon
a game with the application of raycasting.

The entire game is coded by hand without any library (well except one just for asset loading). The entire program is written without any external libraries, but the asset loading uses stb_image, because it's boring stuff and I don't feel like doing from scratch. 

P.S.: because I don't want to initialize OpenGL through Win32 to get the v-sync, I cheated a little by using SDL. but that's just platform layer code; the actual renderer, physics engine, gameplay code is still written from scratch. 

#videos

TTD early demo (11/26/2016):
https://www.youtube.com/watch?v=rgWLmUx7Chw

TTD physics engine demo (11/28/2016):
https://www.youtube.com/watch?v=ABcA5nmE6Eg&feature=youtu.be

#Features and TODOs

things I implemented:
 1. texture mapping for wall rendering. 
 2. floor & ceiling casting along with texture mapping.
 3. supports multiple wall textures (8/31/2016)
 4. sprite sorting & rendering (9/3/2016)
 5. graphics-based bullet detection & basic entity system (9/3/2016)
 6. perspective-oriented sprite generation (9/9/2016)
 7. collision checking against walls (simulate both body collision and shoulder collision), gliding made possible (9/11/2016)
 8. set up internal entity clock (9/20/2016)
 9. basic AI (9/21/2016)
 10. Faster platform layer with SDL (9/23/2016)
 11. Asnychronous sound playback with task-based system (10/8/2016)
 12. One variant memory block per entity, added enemies' firing state (10/28/2016)
 13. Real collision detection, player hitpoints visual representation (11/10/2016)
 14. Big code cleanup and replaced graphically-based collision detection with real collision detection (11/19/2016)
 15. Bitmap font rendering (11/20/2016)
 16. Basic ammo/reload system for pistol (11/23/2016)
 17. Added rifle and minigun into the game (11/24/2016)
 18. Extracted out a physics simulation layer to centralize all physics simulations (11/28/2016)
 19. Added force to bullets and tuned a force constant for a nice bullet recoil (11/28/2016)
 20. Added collectables (ammo, health pack, etc) (12/21/2016) 
 21. Procedural world generation (12/30/2016)
 
TODO List:
 1. Replace hacks in physics simulation, like lerp(), with real motion equations to make
    the movement delta-time-dependent
 2. UI and multiple states
 3. fix the bug with cast_ray()
 
#Things I took away from this project
  
  first, follow my own gut.
    
  (UPDATE: Either these solutions are going to make the code worse. It really is just about tradeoffs; we must assess the situation we are in and determine whether fragmenting the code would be a better solution or having it being big bulk is a better one.)
  
  The urge to apply abstractions/OOP, in most cases, originates from the inability to read code. Then in that case, what abstractions achieve is not making the code readable, but conceiving the programmer that it is readable, which really leaves the program more complicated and fragmented than it should have been. 

  A rule to assess the readability of a program:    
    Good code should actually take a bit longer to read, but very easily understandable, without 1000 layers of "abstraction"s. 
    
    
    player.update();         //NOTE: okay... don't know what it really does but I will take your word for it... I guess?
    
    //player update          //NOTE: okay, I understand.
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
        player->velocity = lerp(player->velocity, player_d_velocity, lerp_constant);
        
        //orientation
        real32 player_delta_angle = -input->mouse.dx / 500.0f * pi32/3.0f * mouse_sensitivity; 
        player->angle += player_delta_angle;
        recanonicalize_angle(&player->angle);
    }                        

NOTE:
 
 Functions and data type definitions, in some way, are trees. The point of "struct" in C was to create multiple layers for a complex data type. Imagine if a data type have 100 fields; it's very hard to understand it. However, if the data type itself is composed of other 5 data types, and those five data types are also composed of five data types, and so on. It's almost as if the data type hiearchy is a penta search tree, and its purpose is to make time that programmer has to spend to reason about the code O(logn), n being the number of fields that's actually there. So I had one random thought: what if there is a program that can balance the "data type tree" and maximize its understandibility, like a B-tree or something. 

#screenshots

 screenshot 1 (8/25/2016) 
![image](https://cloud.githubusercontent.com/assets/16845654/17989412/e3b28ef6-6ae1-11e6-8c19-44c8a2f1dd0e.png)

 screenshot 2 (8/31/2016)
![image](https://cloud.githubusercontent.com/assets/16845654/18156915/dc756310-6fce-11e6-9cf2-fa83e0385250.png)

 screenshot 3 (9/3/2016)
![image](https://cloud.githubusercontent.com/assets/16845654/18228426/77800290-7202-11e6-807d-63ed5401eb38.png)

 screenshot 4 (9/3/2016)
![image](https://cloud.githubusercontent.com/assets/16845654/18229402/494355e6-722d-11e6-9a59-25f2fd9712a1.png)

 screenshot 5 (9/4/2016)
![image](https://cloud.githubusercontent.com/assets/16845654/18237184/50d07c56-72e3-11e6-9a7c-7d94bdc4c2e4.png)
 
 screenshot 6 (9/4/2016)
![image](https://cloud.githubusercontent.com/assets/16845654/18238928/53187fbe-72f6-11e6-820b-8b0a7e1b3a96.png)

 screenshot 7 (9/10/2016)
![image](https://cloud.githubusercontent.com/assets/16845654/18414551/8ea1fe62-7782-11e6-9fbd-174f868bc1f5.png)

 screenshot 8 (11/10/2016)
![image](https://cloud.githubusercontent.com/assets/16845654/20205660/72523014-a78f-11e6-8367-f26a653b72d2.png)

 screenshot 9 (11/23/2016)
![image](https://cloud.githubusercontent.com/assets/16845654/20609341/1aa08602-b240-11e6-87e7-60f83c34d885.png)
