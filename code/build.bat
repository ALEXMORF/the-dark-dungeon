
@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86

pushd ..\build

set compiler_flags=-nologo -Z7 -MT -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4100 -wd4189 -wd4505 -wd4127 
set macro_flags=-DPOOR_DEBUG=1 -DRELEASE_BUILD=0
set linker_flags=-incremental:no user32.lib gdi32.lib winmm.lib

del *.pdb 1> NUL 2> NUL
cl %compiler_flags% %macro_flags% ..\code\game.cpp -LD /link -PDB:game_%random%.pdb %linker_flags% -EXPORT:game_update_and_render
cl %compiler_flags% %macro_flags% ..\code\main.cpp /link %linker_flags%
