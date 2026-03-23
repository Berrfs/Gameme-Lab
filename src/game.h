/* game.h — Public interface for the game module.
   Defines the GameState enum, GameContext struct, and core lifecycle functions.
   Code updated by Louis, at 11:20AM 2026/03/23 */

#ifndef GAME_H
#define GAME_H

#include "scene.h"
#include "raylib.h"
#include "minigame.h"  
#include <string.h>  /* For string operations (strcpy, strcmp, strlen) */

/* Base resolution for UI scaling (UI缩放的基础分辨率) */
#define BASE_SCREEN_WIDTH 1280
#define BASE_SCREEN_HEIGHT 720

/* Enum representing all possible game states */
typedef enum GameState {
    STATE_TITLE,       /* Title / splash screen */
    STATE_NAME_INPUT,  /* Player name input screen */
    STATE_PLAYING,     /* Story dialogue playback */
    STATE_CHOICE,      /* Branching choice overlay */
    STATE_SETTINGS,     /* Settings / options menu */
    STATE_MINIGAME      // 新增
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

    /* Window settings (窗口设置) */
    bool fullscreen;        /* true = fullscreen, false = windowed */
    int window_width;       /* Window width in windowed mode (窗口模式下的宽度) */
    int window_height;      /* Window height in windowed mode (窗口模式下的高度) */

    /* Title screen UI textures */
    Texture2D titleBackground;
    Texture2D titleLogo;
    Texture2D gamemeLabLogo;
    Texture2D btnStart;
    Texture2D btnMenu;
    Texture2D btnExit;
    Texture2D forestBackground;   // 森林背景
    Texture2D computerImage;      // 电脑图片

    /* 当前场景的背景纹理 */
    Texture2D currentBackground;
    /* 当前说话者的立绘纹理 */
    Texture2D currentPortrait;
    /* 记录已加载的背景文件名，用于判断是否需要重新加载 */
    char currentBackgroundPath[256];
    /* 记录已加载的立绘对应的说话者，用于判断是否需要重新加载 */
    char currentSpeaker[64];

    struct MinigameContext* minigame;   // 指向小游戏内部状态的指针
} GameContext;

/* Global game context — single instance shared across modules */
extern GameContext game;

/* Core lifecycle functions */
void InitGame(void);    /* Load assets and set initial state */
void UpdateGame(void);  /* Per-frame logic dispatch */
void DrawGame(void);    /* Per-frame render dispatch */
void UnloadGame(void);  /* Release all loaded resources */

#endif
