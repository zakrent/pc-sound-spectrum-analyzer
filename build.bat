@echo off
mkdir build
pushd build
cl /Zi ..\src\win32_main.c user32.lib dsound.lib opengl32.lib gdi32.lib kernel32.lib /link /subsystem:WINDOWS
popd
