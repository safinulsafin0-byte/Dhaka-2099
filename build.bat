@echo off
setlocal
rem  DHAKA 2099: URBAN WARFARE  -  Windows build (raylib + w64devkit)
rem  Usage:  build.bat            debug build with console, then runs
rem          build.bat release    optimised, no console window
if "%RAYLIB_DIR%"=="" set RAYLIB_DIR=C:\raylib\raylib
if "%W64_DIR%"=="" set W64_DIR=C:\raylib\w64devkit
set PATH=%W64_DIR%\bin;%PATH%
if not exist build mkdir build
if not exist data mkdir data
set FLAGS=-std=gnu11 -O2 -Wall -Wno-misleading-indentation
if /I "%1"=="release" set FLAGS=-std=gnu11 -O3 -mwindows
echo [BUILD] Compiling DHAKA 2099: URBAN WARFARE ...
gcc src\main.c src\ui.c src\hud.c src\game.c src\city.c src\world.c src\chars.c src\audio.c src\save.c -o build\Dhaka2099.exe -I%RAYLIB_DIR%\src -L%RAYLIB_DIR%\src -lraylib -lopengl32 -lgdi32 -lwinmm %FLAGS%
if errorlevel 1 (
  echo.
  echo BUILD FAILED.
  pause
  exit /b 1
)
echo.
echo BUILD SUCCESSFUL: build\Dhaka2099.exe
echo Launching...
build\Dhaka2099.exe
endlocal
