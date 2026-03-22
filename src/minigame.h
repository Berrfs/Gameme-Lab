// minigame.h
#ifndef MINIGAME_H
#define MINIGAME_H

#include "raylib.h"

#define MAX_INVENTORY 5      // 工具栏格子数
#define MAX_ITEMS 6         // 场景中物品总数
#define WALL_COUNT 3         // 三面墙

// 物品结构
typedef struct {
    int id;                  // 物品唯一标识
    const char* name;        // 物品名称（用于显示）
    Texture2D texture;       // 物品图标（拾取后显示在工具栏）
    int wallIndex;           // 物品出现在哪面墙（0,1,2）
    Rectangle interactRect;  // 在该墙上的点击区域（相对屏幕坐标）
    bool isPickedUp;         // 是否已被拾取
    bool visible;            // 是否在当前墙可见（未被拾取且处于正确墙面）
} Item;

// 对话行（小游戏内简单对话）
typedef struct {
    const char* speaker;
    const char* text;
} MiniDialogue;

// 小游戏主上下文（外部不可直接访问，内部由 minigame.c 维护）
typedef struct MinigameContext {
    int currentWall;                     // 当前墙面索引 0~2
    Texture2D wallTextures[WALL_COUNT];  // 三面墙的背景图
    Texture2D arrowLeft, arrowRight;     // 左右箭头纹理
    Texture2D inventorySlot;              // 工具栏格子背景（可选）
    
    Item items[MAX_ITEMS];               // 所有物品定义
    int itemCount;
    
    int inventory[MAX_INVENTORY];         // 当前持有的物品ID，-1表示空
    int selectedSlot;                     // 当前选中的格子索引，-1表示无
    
    // 对话相关
    MiniDialogue currentDialogue;
    bool showDialogue;
    float dialogueTimer;                   // 用于自动隐藏，或手动点击关闭
    
    // 其他状态……
} MinigameContext;

// 全局小游戏上下文（由 minigame.c 自己维护，game.c 不直接访问内部）
void InitMinigame(void);
void UpdateMinigame(void);
void DrawMinigame(void);
void UnloadMinigame(void);

#endif
