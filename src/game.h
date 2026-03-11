#ifndef GAME_H
#define GAME_H

#include "scene.h"
#include "raylib.h"
#include <string.h>  // 新增：用于字符串操作

// 游戏状态枚举
typedef enum GameState {
    STATE_TITLE,
    STATE_NAME_INPUT,   // 新增：输入名字
    STATE_PLAYING,
    STATE_CHOICE,
    STATE_SETTINGS
} GameState;

// 游戏上下文
typedef struct GameContext {
    GameState state;
    Scene *current_scene;   // 当前场景
    int dialogue_index;     // 当前显示的对话索引

    // 玩家名字（最多20字符 + 结尾'\0'）
    char player_name[21];

    // 设置项
    float master_volume;    // 主音量 (0.0 ~ 1.0)
    bool auto_mode;         // true = 自动翻页, false = 手动点击
    float auto_interval;    // 自动翻页间隔（秒）
    float auto_timer;       // 自动翻页计时器

    // Texture UI 用于标题画面
    Texture2D titleBackground;
    Texture2D titleLogo;
    Texture2D gamemeLabLogo;
    Texture2D btnStart;
    Texture2D btnMenu;
    Texture2D btnExit;
} GameContext;

// 全局游戏上下文
extern GameContext game;

// 函数声明
void InitGame(void);
void UpdateGame(void);
void DrawGame(void);
void UnloadGame(void);

#endif