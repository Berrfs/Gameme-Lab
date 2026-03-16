/* game.h — Public interface for the game module.
   Defines the GameState enum, GameContext struct, and core lifecycle functions.
   Code updated by 周沐格, at 07:34PM 2026/03/14 */

#ifndef GAME_H
#define GAME_H

#include "scene.h"
#include "raylib.h"
#include <string.h>  /* For string operations (strcpy, strcmp, strlen) */

/* Enum representing all possible game states */
typedef enum GameState {
    STATE_TITLE,       /* Title / splash screen */
    STATE_NAME_INPUT,  /* Player name input screen */
    STATE_PLAYING,     /* Story dialogue playback */
    STATE_CHOICE,      /* Branching choice overlay */
    STATE_SETTINGS     /* Settings / options menu */
} GameState;

/* Master struct that holds the entire runtime state of the game */
typedef struct GameContext {
    GameState state;
    Scene *current_scene;   /* Pointer to the currently active scene */
    int dialogue_index;     /* Index of the dialogue line being displayed */

    /* Player name — max 20 characters + null terminator */
    char player_name[21];

    /* Settings */
    float master_volume;    /* Master volume level (0.0 – 1.0) */
    bool auto_mode;         /* true = auto-advance dialogue, false = manual click */
    float auto_interval;    /* Seconds between auto-advance steps */
    float auto_timer;       /* Accumulator for auto-advance timing */

    /* Title screen UI textures */
    Texture2D titleBackground;
    Texture2D titleLogo;
    Texture2D gamemeLabLogo;
    Texture2D btnStart;
    Texture2D btnMenu;
    Texture2D btnExit;
} GameContext;

/* Global game context — single instance shared across modules */
extern GameContext game;

/* Core lifecycle functions */
void InitGame(void);    /* Load assets and set initial state */
void UpdateGame(void);  /* Per-frame logic dispatch */
void DrawGame(void);    /* Per-frame render dispatch */
void UnloadGame(void);  /* Release all loaded resources */

#endif