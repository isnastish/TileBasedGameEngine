@echo off

rem NOTE(alexey): gdi32.lib is only for GetStockObject.

if not exist ./build (mkdir build)

pushd build

cl ..\os.cpp -nologo -FC -Zi -Oi -W3 -DINTERNAL_BUILD /LD /link /out:game.dll opengl32.lib
cl ..\win32_game.cpp -nologo -FC -Zi -W3 -DINTERNAL_BUILD /link user32.lib gdi32.lib opengl32.lib winmm.lib

popd