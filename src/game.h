/* game.h — Public interface for the game module.
   Defines the GameState enum, GameContext struct, and core lifecycle functions.
   Code updated by 周沐格, at 10:30AM 2026/04/01 */

#ifndef GAME_H
#define GAME_H

#include "scene.h"
#include "raylib.h"
#include "minigame.h"  
#include <string.h>  /* For string operations (strcpy, strcmp, strlen) */

/* Base resolution for UI scaling */
#define BASE_SCREEN_WIDTH 1280
#define BASE_SCREEN_HEIGHT 720

/* Enum representing all possible game states */
typedef enum GameState {
    STATE_TITLE,       /* Title / splash screen */
    STATE_NAME_INPUT,  /* Player name input screen */
    STATE_PLAYING,     /* Story dialogue playback */
    STATE_CHOICE,      /* Branching choice overlay */
    STATE_SETTINGS,     /* Settings / options menu */
    STATE_MINIGAME,      /* Minigame 1 mode */
    STATE_MINIGAME2      /* Minigame 2 mode */
    STATE_WAREHOUSE,   /* 【新增】仓库场景 */
    STATE_MINIGAME3   /* 【新增】飞机大战小游戏 */
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

    /* Window settings */
    bool fullscreen;        /* true = fullscreen, false = windowed */
    int window_width;       /* Window width in windowed mode */
    int window_height;      /* Window height in windowed mode */

    /* Title screen UI textures */
    Texture2D titleBackground;
    Texture2D titleLogo;
    Texture2D gamemeLabLogo;
    Texture2D btnStart;
    Texture2D btnMenu;
    Texture2D btnExit;
    Texture2D forestBackground;   /* Forest background */
    Texture2D computerImage;      /* Computer image */

    /* Background texture of the current scene */
    Texture2D currentBackground;
    /* Portrait texture of the current speaker */
    Texture2D currentPortrait;
    /* Record of the loaded background file path to prevent redundant loading */
    char currentBackgroundPath[256];
    /* Record of the loaded portrait speaker name to prevent redundant loading */
    char currentSpeaker[64];

    struct MinigameContext* minigame;   /* Pointer to internal minigame state */
} GameContext;

/* Global game context — single instance shared across modules */
extern GameContext game;

/* Core lifecycle functions */
void InitGame(void);    /* Load assets and set initial state */
void UpdateGame(void);  /* Per-frame logic dispatch */
void DrawGame(void);    /* Per-frame render dispatch */
void UnloadGame(void);  /* Release all loaded resources */

#endif
