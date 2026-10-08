@echo off
setlocal
rem  Browser / phone build (WebAssembly). Needs emsdk activated in this shell and a raylib web library:
rem    cd %RAYLIB_DIR%\src  &&  emmake make PLATFORM=PLATFORM_WEB -B   (then rename libraylib.a -> libraylib.web.a)
rem  Serve the web\ folder over http(s) (e.g.  python -m http.server 8080  inside web\ ) and open it on the phone.
if "%RAYLIB_DIR%"=="" set RAYLIB_DIR=C:\raylib\raylib
if not exist web mkdir web
emcc src\main.c src\ui.c src\hud.c src\game.c src\city.c src\world.c src\chars.c src\audio.c src\save.c -o web\index.html ^
  -Os -std=gnu11 -DPLATFORM_WEB -I%RAYLIB_DIR%\src %RAYLIB_DIR%\src\libraylib.web.a ^
  -s USE_GLFW=3 -s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=268435456 -s FORCE_FILESYSTEM=1 -lidbfs.js ^
  --shell-file %RAYLIB_DIR%\src\minshell.html
if errorlevel 1 ( echo WEB BUILD FAILED & pause & exit /b 1 )
echo WEB BUILD OK: web\index.html
endlocal
