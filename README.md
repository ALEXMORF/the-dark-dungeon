# the-dark-dungeon
a game with the application of raycasting.

The entire game is coded by hand without any library (well except one just for asset loading). The render code is written without any external libraries, but the asset loading uses stb_image, because it's boring stuff and I don't feel like doing from scratch. 

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

things I learned:

 1. The urge to apply abstractions/OOP, in most cases, originates from the inability to read code. Then in that case, what abstractions achieve is not making the code readable, but conceiving the programmer that it is readable, which really leaves the program more complicated and fragmented than it should have been. 

 2. A rule to assess the readability of a program:
    Code shouldn't be necesarry very easily readable, but after an iteration of reading it, the programmer must be able to UNDERSTAND the code, but just "read" it and forget. 
    
    player.update();         //NOTE: okay you update the player, but I don't know what it is!! I dont understand!!
    
    //player update
    {
        player.x += dx;
        player.y += dy;
    }                        //NOTE: okay, I understand. 
 
TODO List: 
 1. apply effects like shadow, fog, etc. 
 2. destructable obstacles. 
 3. refine the sound engine 
 4. canonicalize asset loading routine
 5. procedurally generated map
 6. expanded entity types (enemies, decoratives, ammo, health packs, tc)
 
things to test out: global game event queue(?)
 
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
