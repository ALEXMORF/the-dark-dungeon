@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86
     
pushd ..\release\build

set sdl_lib_path=C:\SDL_vs\lib\x86
set sdl_include_path=C:\SDL_vs\include
set compiler_flags=-nologo -Z7 -MT -Gm- -GR- -EHa- -O2 -Oi -WX -W4 -wd4100 -wd4189 -wd4505 -wd4127 -wd4002
set precompile_flags=-Ycstb_image.h -Yustb_image.h
set macro_flags=-DPOOR_DEBUG=1 -DRELEASE_BUILD=0 -DVSYNC=1
set linker_flags=-incremental:no -SUBSYSTEM:windows,5.1 user32.lib gdi32.lib winmm.lib 

del *.pdb 1> NUL 2> NUL
cl %compiler_flags% %macro_flags% ..\..\code\game.cpp -LD /link -PDB:game_%random%.pdb %linker_flags% -EXPORT:game_update_and_render -EXPORT:game_process_sound
cl -Femain %compiler_flags% %precompile_flags% %macro_flags%  -I %sdl_include_path% ..\..\code\fast_main.cpp /link -SUBSYSTEM:windows,5.1 -LIBPATH:%sdl_lib_path% %linker_flags% SDL2.lib SDL2main.lib
