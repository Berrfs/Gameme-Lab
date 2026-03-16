/* main.c — Entry point. Opens the game window, runs the main loop, and shuts down.
   Kept intentionally minimal as all game logic lives in game.c and beyond.
   Code updated by Louis , at 04:15PM 2026/03/15 */

#include "raylib.h"
#include "game.h"

int main(void) {
    /* Create a 1600x1200 window titled "No way!" */
    InitWindow(1600, 1200, "No way!");
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