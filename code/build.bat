
@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64

pushd ..\build

set sdl_lib_path=C:\SDL_vs\lib\x64
set sdl_include_path=C:\SDL_vs\include
set compiler_flags=-nologo -Z7 -MD -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4100 -wd4189 -wd4505 -wd4127 
set macro_flags=-DPOOR_DEBUG=1 -DRELEASE_BUILD=0
set linker_flags=-incremental:no user32.lib gdi32.lib winmm.lib opengl32.lib

del *.pdb 1> NUL 2> NUL
cl %compiler_flags% %macro_flags% ..\code\game.cpp -LD /link -PDB:game_%random%.pdb %linker_flags% -EXPORT:game_update_and_render
cl -Femain %compiler_flags% %macro_flags% -I %sdl_include_path% ..\code\fast_main.cpp /link -LIBPATH:%sdl_lib_path% %linker_flags% SDL2.lib SDL2main.lib
