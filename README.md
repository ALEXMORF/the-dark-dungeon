# the-dark-dungeon
a game with the application of raycasting.

All the game code and render code is coded by hand with only one library. The render code is written without any external libraries, but the asset loading uses stb_image, because it's boring stuff and I don't feel like doing from scratch. 

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
 
TODO List: 
 1. apply effects like shadow, fog, etc. 
 2. destructable obstacles. 
 3. refine the sound engine 
 4. canonicalize asset loading routine
 5. procedurally generated map
 6. expanded entity types (enemies, decoratives, ammo, health packs, tc)
 
things to test out: global game event queue(?)
 
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
