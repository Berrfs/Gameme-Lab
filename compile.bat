@echo off
set PATH=C:\msys64\ucrt64\bin;%PATH%
echo === Compiling No Way! ===
gcc src/main.c src/game.c src/scene.c src/save.c src/cJSON/cJSON.c -o game.exe -Isrc -IC:\msys64\ucrt64\include -LC:\msys64\ucrt64\lib -lraylib -lopengl32 -lgdi32 -lwinmm
if %errorlevel%==0 (
    echo === Compile berhasil! ===
    echo Jalankan: .\game.exe
) else (
    echo === Compile GAGAL ===
)
