#ifndef GAME_H
#define GAME_H

#include "scene.h"

// 游戏状态枚举
typedef enum GameState {
    STATE_TITLE,        // 标题画面
    STATE_PLAYING,      // 剧情进行中
    STATE_SETTINGS      // 设置界面
} GameState;

// 游戏上下文
typedef struct GameContext {
    GameState state;
    Scene *current_scene;   // 当前场景
    int dialogue_index;     // 当前显示的对话索引
    // 设置项
    float master_volume;    // 主音量 (0.0 ~ 1.0)
    int text_speed;         // 文字速度（字符/秒，简单起见我们用延迟帧数表示）
    // Texture UI untuk title screen
    Texture2D titleBackground;  // Background merah
    Texture2D titleLogo;        // Papan kayu "NO WAY!"
    Texture2D gamemeLabLogo;    // Logo "GamemeLab"
    Texture2D btnStart;         // Tombol "Touch to start"
    Texture2D btnMenu;          // Tombol "Menu"
    Texture2D btnExit;          // Tombol "Exit"
} GameContext;

// 全局游戏上下文
extern GameContext game;

// 函数声明
void InitGame(void);
void UpdateGame(void);
void DrawGame(void);
void UnloadGame(void);

#endif