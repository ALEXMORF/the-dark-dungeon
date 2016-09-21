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
 7. collision checking against walls (simulate both body collision and shoulder collision), gliding made possible (9/11/2016)
 8. set up internal entity clock (9/20/2016, tentative)
currently working on: enemy AI

things on my mind: bilinear filtering???

things to test out: global game event queue(?)
                    FSM for dynamic entity behavior. 

#Techniques for drawing clear boundaries between subsystems: interfaces between subsystems

Say if your game is composed of many small subsystems that work together. You would imagine a streamline that transforms your data: first user input is parsed, then that information is used to simulate entities, then you pass entities into your renderer to put some graphics on the screen, blah blah blah. Let's pick two particular subsystems to demonstrate my point: your entities and your renderer. 
Can you pass your entities right into your renderer? absolutely, in fact, it is probably the most straight forward way to do sutff, such as:

...stuff...

for entities in entity system {

   entity.simulate(user.input);
   
}

my_renderer.render_entity_list(entity_system.content);

...stuff...

Sweet, now we have a tight and robust chain of subsystems, right?  With some good intuition, you can probably spot the danger of this; they are too deeply coupled. This kind of code encourages further intermangling of your entity, in fact, they are already intermangled. 

Say, you want to change some stuff inside your entities, and suddenly you have to change your renderer as well; since the entity system changed, there is a chance that your renderer must be adjusted accordingly. Hold on, I'm not done yet! This deep coupling effect will propogate; it probably is going to occur more than one layer; one subsystem fails after another due to domino effect. The larger your program is, the more frequent this kind of thing will happen. 

Damn, that was really nasty! So what do we do? give up programming and become an artist? Don't worry, I came up with something called "interface" (which I named it myself).

Interfaces are the middle-grounds; they are what different subsystems use to talk to each other. Let's revise the code we had before:

...stuff...

for entities in entity system {
   
   entity.simulate(user.input);
   
}

sprite_list = generate_sprites(entity_system.content);
my_renderer.render_sprites(sprite_list);

...stuff...

Before, if one subsystems changes, the other subsystem probably will change as well; because their communication was very barebone and direct. Some changes on one end will possibly mean there must be some changes on the other end as well, and it propogates like that to other subsystems. However, with a interface like "sprites", only the communication between entity system and sprite system are affected. You have prevented it from furthe propogation into other subsystems; you have effectively limited the changes inside one subsystem and one interface. 

#what I have found in this experience: The cost of context switching is the worst enemy of productivity

 I like to program in a linear and straightforward fashion, because it allows me to reason about my code and 
detect bugs in the smallest amount of time possible. However, whenever I try to write non-trivial programs, 
I sort of get depressed by the way I am doing stuff: it seemed like my way of programming is actually hindering 
my productivity, and the OOP style is truly the salvation, which, let's be real, switch between headers 
and source files is already enough context switching to induce suicidal thoughts, not to mention how much speculations
about code reuse and abstraction you put into designing the class ended up not being useful at all; just a huge 
waste of time. 

 That is one of the reasons I am doing this project; to find out what is damaging my productivity when 
working with non-trivial amount of code (1K lines and above). For this game, after gotten the basic 3D rendering engine 
done, the same happened again: it became very difficult to reason about my code, and the worst part is, that is 
not due to the size of my code. 

 My rendering code in total is about 500 lines, and I am perfectly fine  with both reading it and maintaining it. 
On the other hand, my game code is only about 100 lines or less,  however it's extremely hard to reason about. 
Upon closer inspection, it seems like I have found the culprit: boundary-less subsystems that leads to frequent 
context-switching. What is damaging my productivity is not my 300-line long functions or not following C++ guidlines; 
it was because I had to constantly switch context while dealing with the same piece of code. Since each subsystem has
no boundary and pretty much has bearing on another, which also has a bearing on something else. The intermangled mess 
I get from having no clear boundary is the real cause of my productivity depression. 

 I did many things in a hacky way; I played sound while processing input, I returned collision detection result 
 while rendering, and my animation code is somehow inside my simulation code. Therefore, whenver I try to reason about 
 my code, I had to consider multiple things at once, and that usually feels like juggling; it was possible to reason about
 my code, but it was also extremely exhausting, despiting its small size. It is this constant context switching 
 that costs me productivity. As soon as I decoupled all these processes, my code becomes much clearer and easier to reason 
 about and it soon became a pleasure to code again, inside this codebase. 

 I simply put this block of letters here, as a reminder on how to write "reasonable" code.

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

 I dislike OOP because of the boilerplate code I have to write; I find myself spending more time writing boilerplate and speculating what I need for the class than writing the actual code that actually does something. In addition, OOP-style progrmaming usually results in non-linear code, which is hard to follow. However, there is nothing wrong with objects or encapsulation; I think they help programmers to enforce the idea of decoupling subsystems and prevent one from having a bearing on another, and therefore helps reduce context-switching cost. On the other hand, programming in C style can be very fast, but it also blurs the boundaries between subsystems and tempt me to intermangle them to save time. The important thing is how to strike balance between these two different styles. 
 
 My Speculation is: write the low-level code first, once the code accomplishes a task and has evolved to a state that's hard to manage (which, you should know when you see it), pull them out and put them into different subsystems with clear boundaries (classes if you feel like it). Therefore, the process of speculating what functionalities or abstractions a subsystem (or class) need wouldn't occur, because they are all presented by the low-level code that's already been written (and the good part is that the low-level code actually does something). However, the usage of class here is not for OOP: separation of subsystem and drawing clear boundaries between them can be done without classes just as easily. What I classes might provide is readability. I'd rather see player.update() than player_update(player), it makes the code easier to reason about and reads more like English.
 
 NOTE: however, use objects sparsely. Use class only for where seems fit. Trying to fit everything into an object will result in a disaster: data and procedure (or behavior, if you want to call it that) will be completely mixed together and the code no longer follows a linear pattern. 
 
 don't be distracted by objects, either. Only focus on the actual data that you are actually transforming, and make your approaches oriented around the data and the problem, not the OOP practices.
 
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
