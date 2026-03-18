/* main.c — Entry point. Opens the game window, runs the main loop, and shuts down.
   (入口文件。打开游戏窗口，运行主循环，然后关闭。)
   Kept intentionally minimal as all game logic lives in game.c and beyond.
   (保持简洁，所有游戏逻辑在 game.c 及其他模块中处理。)
   Code updated by Louis, at 09:24PM 2026/03/18 */

#include "raylib.h"
#include "game.h"

int main(void) {
    /* Create a 1280x720 window titled "No way!" (创建一个 1280x720 的窗口，标题为 "No way!") */
    InitWindow(1280, 720, "No way!");
    /* Lock the frame rate to 60 FPS (将帧率锁定为 60 FPS) */
    SetTargetFPS(60);

    /* Initialize all game resources and state (初始化所有游戏资源和状态) */
    InitGame();

    /* Main game loop — runs until user closes the window (主游戏循环——直到用户关闭窗口) */
    while (!WindowShouldClose()) {
        UpdateGame();
        DrawGame();
    }

    /* Free loaded resources before exiting (退出前释放所有已加载的资源) */
    UnloadGame();
    CloseWindow();

    return 0;
}
