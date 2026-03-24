/* main.c — Entry point. Opens the game window, runs the main loop, and shuts down.
   (Entry file. Opens the game window, runs the main loop, then exits.)
   Kept intentionally minimal as all game logic lives in game.c and beyond.
   (Keep it simple; all game logic is handled in game.c and other modules.)
   Code updated by Louis, at 11:20AM 2026/03/23 */

#include "raylib.h"
#include "game.h"
#include "cJSON/cJSON.h"
#include <stdio.h>

/* Load only window settings from data/settings.json */
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
    /* Load window settings before initializing */
    int window_width = 1280;
    int window_height = 720;
    bool fullscreen = false;
    LoadWindowSettings(&window_width, &window_height, &fullscreen);

    /* Set fullscreen flag if needed */
    if (fullscreen) {
        SetConfigFlags(FLAG_FULLSCREEN_MODE);
    }

    /* Create a 1280x720 window titled "No way!" */
    InitWindow(window_width, window_height, "No way!");
    /* Lock the frame rate to 60 FPS */
    SetTargetFPS(60);

    /* Initialize all game resources and state */
    InitGame();

    /* Main game loop — runs until user closes the window */
    while (!WindowShouldClose()) {
        UpdateGame();
        DrawGame();
    }

    /* Free loaded resources before exiting */
    UnloadGame();
    CloseWindow();

    return 0;
}
