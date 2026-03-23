/* main.c — Entry point. Opens the game window, runs the main loop, and shuts down.
   (入口文件。打开游戏窗口，运行主循环，然后关闭。)
   Kept intentionally minimal as all game logic lives in game.c and beyond.
   (保持简洁，所有游戏逻辑在 game.c 及其他模块中处理。)
   Code updated by Louis, at 11:20AM 2026/03/23 */

#include "raylib.h"
#include "game.h"
#include "cJSON/cJSON.h"
#include <stdio.h>

/* Load only window settings from data/settings.json (仅从 data/settings.json 加载窗口设置) */
void LoadWindowSettings(int *width, int *height, bool *fullscreen) {
    *width = 1280;
    *height = 720;
    *fullscreen = false;

    FILE *file = fopen("data/settings.json", "r");
    if (!file) return;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = (char *)malloc(size + 1);
    fread(content, 1, size, file);
    content[size] = '\0';
    fclose(file);

    cJSON *root = cJSON_Parse(content);
    free(content);

    if (!root) return;

    cJSON *width_obj = cJSON_GetObjectItem(root, "window_width");
    if (width_obj && width_obj->type == cJSON_Number) {
        *width = width_obj->valueint;
    }

    cJSON *height_obj = cJSON_GetObjectItem(root, "window_height");
    if (height_obj && height_obj->type == cJSON_Number) {
        *height = height_obj->valueint;
    }

    cJSON *fullscreen_obj = cJSON_GetObjectItem(root, "fullscreen");
    if (fullscreen_obj && fullscreen_obj->type == cJSON_True) {
        *fullscreen = true;
    }

    cJSON_Delete(root);
}

int main(void) {
    /* Load window settings before initializing (初始化前加载窗口设置) */
    int window_width = 1280;
    int window_height = 720;
    bool fullscreen = false;
    LoadWindowSettings(&window_width, &window_height, &fullscreen);

    /* Set fullscreen flag if needed (如果需要，设置全屏标志) */
    if (fullscreen) {
        SetConfigFlags(FLAG_FULLSCREEN_MODE);
    }

    /* Create a 1280x720 window titled "No way!" (创建一个窗口，标题为 "No way!") */
    InitWindow(window_width, window_height, "No way!");
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
