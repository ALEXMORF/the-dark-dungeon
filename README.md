# the-dark-dungeon
a game with the application of raycasting.

All the game code and render code is coded by hand with only one library. The render code is written without any external libraries, but the asset loading uses stb_image, because it's boring stuff and I don't feel like doing from scratch. 

existing features:
 1. texture mapping for wall rendering. 
 2. floor & ceiling casting along with texture mapping.
 3. supports multiple wall textures (8/31/2016)
 4. sprite sorting & rendering (9/3/2016)
 5. graphics-based bullet detection & basic entity system (9/3/2016)
 6. perspective-oriented sprite generation (9/9/2016)

currently working on: enemy AI

things on my mind: bilinear filtering???

#what I have found in this experience: The cost of context switching is the worst enemy of productivity

 I like to program in a linear and straightforward fashion, because it allows me to inspect my program and 
detect bugs in the smallest amount of time possible. However, whenever I try to write non-trivial programs, 
I sort of get depressed by the way I am doing stuff: it seemed like my way of programming is actually hindering 
my productivity, and the OOP style is truly the salvation, which, let's be real, switch between headers 
and source files is already enough context switching to induce suicidal thoughts. 

 That is one of the reasons I am doing this project; to find out what is damaging my productivity when 
working with large amount of code (1K lines and above). For this game, after gotten the basic 3D rendering engine 
done, the same happened again: it became very difficult to reason about my code, and the worst part is, that is 
not due to the size of my code. 

 My rendering code in total is about 500 lines, and I am perfectly fine  with both reading it and maintaining it. 
On the other hand, my game code is only about 100 lines or less,  however it's extremely hard to reason about. 
Upon closer inspection, it seems like I have found the culprit: careless programming that leads to frequent 
context-switching. What is damaging my productivity is not the  way I program, nor the way I inline functions. 
It was because I had to constantly switch context while dealing with the same piece of code. 

 I did many things in a hacky way; I played sound while processing input, I returned collision detection result 
 while rendering, and my animation code is somehow inside my simulation code. Therefore, whenver I try to reason about 
 my code, I had to consider multiple things at once, and that usually feels like juggling; it was possible to reason about
 my code, but it was also extremely exhausting, despiting its small size. It is this constant context switching 
 that costs me productivity. As soon as I decoupled all these processes, my code becomes much clearer and easier to reason 
 about and it soon became a pleasure to code again, inside this codebase. 

 I just got started programming, barely knows how to program, but I hope this discovery is worth a note and I am 
not deluded. I simply put this block of letters here, as a reminder on how to write "reasonable" code.

#So, how to prevent this from happening next time?
 
 Do not misplace the emphasis. The emphasis should always be on the problem-solving rather than the code, but when code is hindering your problem solving, then it's time to use this strategy.
 
 NOTE: Doing this is the same as locking down your game's architecture. If anything is wrong with the way you put them together, it becomes very difficult to change around. Therefore, makes sure separation of code is the last thing you do. Write the actual code that does stuff first. Do no fall into the big agenda like "one class does one thing" or "function can't exceed 25 lines". Make sure there are only sparse separations and do them only when the context-switching is constantly happening.
 
 When the context-switch cost becomes too big, what to do?
 
 Separate your code if they do different things, make sure each portion have no bearing on another.
 
 Think about yourself as an urban planner (or whatever it's called). Plan how you are going to transform your data from 
the beginning to the end. Keep each subsystems separated if their functionalities are different. Make it in such a way that
no portion of code has a bearing on another portion. By doing this, you minimalized the frequency of context switching and have
your productivity maximized. 
 
# My speculation on good programming

 I dislike OOP because of the boilerplate code I have to write; I find myself spending more time writing boilerplate and considering about abstraction than writing the actual code that actually does something. In addition, OOP-style progrmaming usually results in non-linear code, which is hard to follow. However, there is nothing wrong with objects or encapsulation; I think they help programmers to enforce the idea of decoupling subsystems and prevent one from having a bearing on another, and therefore helps reduce context-switching cost. On the other hand, programming in C style can be very fast, but it also blurs the boundaries between subsystems and tempt me to intermangle them to save time. The important thing is how to strike balance between these two different styles. 
 
 My Speculation is: write the low-level code first, once the code accomplishes a task and has evolved to a state that's hard to manage (which, you should know when you see it), pull them out and put them in a class. Therefore, the process of speculating what a functionalities or abstractions a class need wouldn't occur, because by the time the low-level code is done, it should be very clear what the class should look like. However, the usage of class here is not for OOP, but rather semantic compression. I'd rather see player.update() than player_update(player), it makes the code easier to reason about and reads more like English.

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
