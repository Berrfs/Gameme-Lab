@echo off
set PATH=C:\msys64\ucrt64\bin;%PATH%
echo === Compiling No Way! ===
gcc src/main.c src/game.c src/scene.c src/save.c src/minigame.c src/minigame2.c src/minigame3.c src/minigame4.c src/bossbattle.c src/warehouse.c src/cJSON/cJSON.c -o game.exe -Isrc -IC:\msys64\ucrt64\include -LC:\msys64\ucrt64\lib -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows
if %errorlevel%==0 (
    echo === Compile berhasil! ===
    echo Bundling required DLLs...
    copy /Y "C:\msys64\ucrt64\bin\libraylib.dll" . >nul 2>&1
    copy /Y "C:\msys64\ucrt64\bin\glfw3.dll" . >nul 2>&1
    echo Jalankan: .\game.exe
) else (
    echo === Compile GAGAL ===
)
