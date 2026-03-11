#include "raylib.h"
#include "game.h"

int main(void) {
    InitWindow(1600, 1200, "No way!");
    SetTargetFPS(60);

    // 初始化游戏
    InitGame();

    // 主循环
    while (!WindowShouldClose()) {
        UpdateGame();
        DrawGame();
    }

    // 清理
    UnloadGame();
    CloseWindow();

    return 0;
}