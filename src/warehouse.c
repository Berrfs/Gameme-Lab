/* warehouse.c — Warehouse hub scene implementation.
   Three-room navigable hub: left room (cockpit) triggers Minigame 3 (shooter),
   right room triggers Minigame 4 (platformer), center room is the storage area.
   Both sub-games must be cleared to unlock the exit and continue the main story.
   Code updated by 周沐格, at 09:24PM 2026/04/03 */

#include "warehouse.h"
#include "game.h"
#include "scene.h"
#include "raylib.h"
#include <string.h>

extern GameContext game;

// 仓库子状态
typedef enum {
    WH_INTRO_DIALOGUE,    // 初始剧情
    WH_FREE_ROAM,         // 自由探索（点击箭头切换房间）
    WH_LEFT_PROMPT,       // 左边房间的 YES/NO 选项
    WH_RIGHT_DIALOGUE,    // 右边房间剧情
    WH_FINAL_DIALOGUE     // 全部通关后的离开剧情
} WHState;

static struct {
    Texture2D bgLeft;       // 左侧房间背景 (驾驶舱)
    Texture2D bgMiddle;     // 中间房间背景 (仓库)
    Texture2D bgRight;      // 右侧房间背景 (寻找钥匙的区域)
    Texture2D bgSkyLeft;    // 左侧通关后的蓝天
    Texture2D bgSkyRight;   // 右侧通关后的蓝天
    
    Texture2D arrowLeft;
    Texture2D arrowRight;

    int currentRoom; // 0 = 左, 1 = 中, 2 = 右
    WHState state;

    // 通关标记 
    bool combatCleared;
    bool mazeCleared;

    // 对话系统简易管理
    int dialogueStep;
    
    // 标记右侧剧情是否已经看过了，防止重复触发
    bool rightIntroSeen; 
} wh = {0};

/* 辅助函数声明 */
static void DrawDialogBox(const char* speaker, const char* text);
static void ExitWarehouseTo(const char* nextScene);

void InitWarehouse(void) {
    memset(&wh, 0, sizeof(wh));

    // 加载不同房间的独立背景
    wh.bgLeft     = LoadTexture("UI/cockpit_bg.jpg"); 
    wh.bgMiddle   = LoadTexture("UI/cosmic.png");     
    wh.bgRight    = LoadTexture("UI/maze_bg.png");    
    
    // 加载两侧通关后的不同蓝天背景
    wh.bgSkyLeft  = LoadTexture("UI/sky_left.jpg");   
    wh.bgSkyRight = LoadTexture("UI/sky_right.jpg");  
    
    wh.arrowLeft  = LoadTexture("UI/arrow_left.png");
    wh.arrowRight = LoadTexture("UI/arrow_right.png");

    wh.currentRoom = 1; // 初始在中间房间
    wh.state = WH_INTRO_DIALOGUE;
    wh.dialogueStep = 0;

    wh.combatCleared = false;
    wh.mazeCleared = false;
    wh.rightIntroSeen = false;
}

void UpdateWarehouse(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    Vector2 mouse = GetMousePosition();

    // 1. 开场对话逻辑 (位于中间房间)
    if (wh.state == WH_INTRO_DIALOGUE) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            wh.dialogueStep++;
            if (wh.dialogueStep >= 4) { // 开场一共4句话
                wh.state = WH_FREE_ROAM;
            }
        }
        return;
    }

    // 2. 最终离场对话逻辑
    if (wh.state == WH_FINAL_DIALOGUE) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            ExitWarehouseTo("scene7"); // 跳转到正常剧情场景
        }
        return;
    }

    // 3. 右侧房间开场对话逻辑 -> 触发 Minigame 4 (双相跳跃)
    if (wh.state == WH_RIGHT_DIALOGUE) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            wh.rightIntroSeen = true;
            // 调用 Minigame4
            extern void InitMinigame4(void);
            InitMinigame4();
            game.state = STATE_MINIGAME4; // 暂停仓库，切换到小游戏
        }
        return;
    }

    // 4. 左侧房间的 YES / NO 选项逻辑 -> 触发 Minigame 3 (飞机大战)
    if (wh.state == WH_LEFT_PROMPT) {
        Rectangle btnYes = { sw/2.0f - 150, sh/2.0f, 100, 50 };
        Rectangle btnNo  = { sw/2.0f + 50,  sh/2.0f, 100, 50 };

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mouse, btnYes)) {
                // 跳转到小游戏
                extern void InitMinigame3(void); // 声明
                InitMinigame3();
                game.state = STATE_MINIGAME3; // 切换状态
            } else if (CheckCollisionPointRec(mouse, btnNo)) {
                wh.state = WH_FREE_ROAM;
            }
        }
        return;
    }

    // 5. 自由探索逻辑 (WH_FREE_ROAM)
    if (wh.state == WH_FREE_ROAM) {
        
        // --- 检查是否全部通关 ---
        if (wh.combatCleared && wh.mazeCleared) {
            wh.state = WH_FINAL_DIALOGUE;
            return;
        }

        // --- 箭头切换房间检测 ---
        Rectangle leftArrowRect = { 20, sh/2.0f - 50, 80, 100 };
        Rectangle rightArrowRect = { sw - 100, sh/2.0f - 50, 80, 100 };

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            // 点击左箭头 (只能在房间1和2点击)
            if (wh.currentRoom > 0 && CheckCollisionPointRec(mouse, leftArrowRect)) {
                wh.currentRoom--;
            }
            // 点击右箭头 (只能在房间0和1点击)
            else if (wh.currentRoom < 2 && CheckCollisionPointRec(mouse, rightArrowRect)) {
                wh.currentRoom++;
                
                // 如果刚进入右房间，且还没触发过右侧剧情，且没通关
                if (wh.currentRoom == 2 && !wh.rightIntroSeen && !wh.mazeCleared) {
                    wh.state = WH_RIGHT_DIALOGUE;
                }
            }
            // --- 左边房间的操作 (点击面板按钮) ---
            else if (wh.currentRoom == 0 && !wh.combatCleared) {
                // 操作台按钮区域
                Rectangle dashboardBtn = { sw/2.0f - 100, sh/2.0f + 50, 200, 80 };
                if (CheckCollisionPointRec(mouse, dashboardBtn)) {
                    wh.state = WH_LEFT_PROMPT;
                }
            }
        }
    }
}

void DrawWarehouse(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // --- 1. 绘制独立的背景逻辑 ---
    Texture2D currentBg;
    if (wh.currentRoom == 0) {
        currentBg = wh.combatCleared ? wh.bgSkyLeft : wh.bgLeft;
    } else if (wh.currentRoom == 1) {
        currentBg = wh.bgMiddle;
    } else { 
        currentBg = wh.mazeCleared ? wh.bgSkyRight : wh.bgRight;
    }

    if (currentBg.id > 0) {
        DrawTexturePro(currentBg, 
            (Rectangle){0, 0, (float)currentBg.width, (float)currentBg.height}, 
            (Rectangle){0, 0, (float)sw, (float)sh}, 
            (Vector2){0,0}, 0.0f, WHITE);
    } else {
        // 兜底的纯色背景
        if (wh.currentRoom == 0) {
            ClearBackground(wh.combatCleared ? LIGHTGRAY : DARKBLUE);
        } else if (wh.currentRoom == 1) {
            ClearBackground(DARKPURPLE);
        } else {
            ClearBackground(wh.mazeCleared ? SKYBLUE : DARKGRAY);
        }
    }

    // --- 2. 绘制房间专属陈设 ---
    if (wh.currentRoom == 0) {
        // 左侧操作台与按钮
        if (!wh.combatCleared) {
            Rectangle dashboardBtn = { sw/2.0f - 100, sh/2.0f + 50, 200, 80 };
            DrawRectangleRec(dashboardBtn, RED);
            DrawRectangleLinesEx(dashboardBtn, 3, MAROON);
            DrawText("COMBAT", dashboardBtn.x + 40, dashboardBtn.y + 25, 30, WHITE);
        } else {
            DrawText("SYSTEM CLEARED", sw/2 - 120, sh/2 + 70, 30, GREEN);
        }
    } 

    // --- 3. 绘制左右导航箭头 (自由模式下才显示) ---
    if (wh.state == WH_FREE_ROAM) {
        Rectangle leftArrowRect = { 20, sh/2.0f - 50, 80, 100 };
        Rectangle rightArrowRect = { sw - 100, sh/2.0f - 50, 80, 100 };

        if (wh.currentRoom > 0) { // 中、右房间显示左箭头
            if (wh.arrowLeft.id > 0) {
                DrawTexturePro(wh.arrowLeft, (Rectangle){0,0,wh.arrowLeft.width,wh.arrowLeft.height}, leftArrowRect, (Vector2){0,0}, 0.0f, WHITE);
            } else {
                DrawTriangle((Vector2){100, sh/2}, (Vector2){100, sh/2-50}, (Vector2){20, sh/2}, WHITE); 
            }
        }
        if (wh.currentRoom < 2) { // 左、中房间显示右箭头
            if (wh.arrowRight.id > 0) {
                DrawTexturePro(wh.arrowRight, (Rectangle){0,0,wh.arrowRight.width,wh.arrowRight.height}, rightArrowRect, (Vector2){0,0}, 0.0f, WHITE);
            } else {
                DrawTriangle((Vector2){sw-100, sh/2}, (Vector2){sw-100, sh/2-50}, (Vector2){sw-20, sh/2}, WHITE); 
            }
        }
    }

    // --- 4. 绘制弹窗和对话框 ---
    if (wh.state == WH_INTRO_DIALOGUE) {
        if (wh.dialogueStep == 0) DrawDialogBox("Game", "Well…Look what you've done!");
        else if (wh.dialogueStep == 1) DrawDialogBox(game.player_name, "Where is this place?");
        else if (wh.dialogueStep == 2) DrawDialogBox("Game", "This is my warehouse. Everything you might need for the game is stored here.");
        else if (wh.dialogueStep == 3) DrawDialogBox("Game", "Look, here's the wooden plank I used to lock the door in the last room...\nNo, wait, I didn't lock the door. Definitely not.");
    } 
    else if (wh.state == WH_RIGHT_DIALOGUE) {
        DrawDialogBox("Game", "Oops, I seem to have misplaced the key to get out.\nCould you help me look for it?");
    }
    else if (wh.state == WH_FINAL_DIALOGUE) {
        DrawDialogBox("Game", "Oh, thank you. We were accidentally teleported here just now,\nand now we can leave.\n\n[Click to exit Warehouse]");
    }
    else if (wh.state == WH_LEFT_PROMPT) {
        // YES / NO 询问框
        DrawRectangle(0, 0, sw, sh, (Color){0,0,0,150});
        
        int boxW = 500, boxH = 200;
        DrawRectangle(sw/2 - boxW/2, sh/2 - boxH/2, boxW, boxH, DARKGRAY);
        DrawText("Commence combat operations?", sw/2 - 200, sh/2 - 60, 26, WHITE);

        Rectangle btnYes = { sw/2.0f - 150, sh/2.0f, 100, 50 };
        Rectangle btnNo  = { sw/2.0f + 50,  sh/2.0f, 100, 50 };
        
        DrawRectangleRec(btnYes, GREEN);
        DrawText("YES", btnYes.x + 25, btnYes.y + 15, 20, BLACK);

        DrawRectangleRec(btnNo, GRAY);
        DrawText("NO", btnNo.x + 35, btnNo.y + 15, 20, BLACK);
    }
}

void UnloadWarehouse(void) {
    if (wh.bgLeft.id > 0)     UnloadTexture(wh.bgLeft);
    if (wh.bgMiddle.id > 0)   UnloadTexture(wh.bgMiddle);
    if (wh.bgRight.id > 0)    UnloadTexture(wh.bgRight);
    
    if (wh.bgSkyLeft.id > 0)  UnloadTexture(wh.bgSkyLeft);
    if (wh.bgSkyRight.id > 0) UnloadTexture(wh.bgSkyRight);
    
    if (wh.arrowLeft.id > 0)  UnloadTexture(wh.arrowLeft);
    if (wh.arrowRight.id > 0) UnloadTexture(wh.arrowRight);
    
    memset(&wh, 0, sizeof(wh));
}

// --- 辅助函数实现 ---

static void DrawDialogBox(const char* speaker, const char* text) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int boxY = sh - 200;

    DrawRectangle(20, boxY, sw - 40, 180, (Color){0,0,0,220});
    DrawText(speaker, 50, boxY + 20, 30, MAROON);
    DrawText(text, 50, boxY + 70, 24, WHITE);
    DrawText("Click to continue", sw - 250, boxY + 140, 20, GRAY);
}

static void ExitWarehouseTo(const char* nextScene) {
    char target[32];
    strncpy(target, nextScene, 31);
    
    UnloadWarehouse();

    game.current_scene = GetSceneByID(target);
    if (game.current_scene) {
        game.state = STATE_PLAYING;
        game.dialogue_index = 0;
        game.auto_timer = 0.0f;
    } else {
        game.state = STATE_TITLE;
    }
}

// 接收minigame3结果的回调函数
void NotifyWarehouseMinigame3(bool success) {
    wh.state = WH_FREE_ROAM; 
    if (success) {
        wh.combatCleared = true; 
    }
}

// 接收minigame4结果的回调函数
void NotifyWarehouseMinigame4(bool success) {
    wh.state = WH_FREE_ROAM; 
    if (success) {
        wh.mazeCleared = true; 
    }
}
