#include "minigame2.h"
#include "game.h"
#include "scene.h"
#include "raylib.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

extern GameContext game;

// 最大连续对话行数
#define MAX_DIALOGUE_LINES 10

// 工具栏常量与物品ID
#define MG2_MAX_INVENTORY 5
#define ITEM_BUCKET_EMPTY 1
#define ITEM_BUCKET_WATER 2
#define ITEM_SAPLING 3      // 将草叶改为树苗

// 游戏阶段状态机
typedef enum {
    MG2_INTRO,          // Game说话，给水桶
    MG2_PLAYING,        // 自由探索，交互
    MG2_GROWING,        // 树长高，屏幕震动
    MG2_BOSS_ANGRY,     // Game发怒
    MG2_QTE             // 黑洞与挣扎按钮
} MG2State;

// 人物朝向
typedef enum { DIR_DOWN = 0, DIR_UP = 1, DIR_LEFT = 2, DIR_RIGHT = 3 } Direction;

// 单条对话结构体
typedef struct {
    char speaker[64];
    char text[256];
} DialogueLine;

static struct {
    Texture2D bgMap;
    Texture2D playerTex;
    Texture2D treeTex;
    Texture2D holeTex;

    // 工具栏与物品贴图
    Texture2D inventorySlotTex;
    Texture2D itemBucketEmptyTex;
    Texture2D itemBucketWaterTex;
    Texture2D itemSaplingTex; // 树苗贴图

    int inventory[MG2_MAX_INVENTORY]; // 工具栏数组
    int selectedSlot;                 // 当前选中的格子

    // 玩家数据
    Vector2 playerPos;
    float speed;
    Direction currentDir;
    int currentFrame;
    float frameTimer;
    bool isMoving;

    // 交互区域
    Vector2 rabbitPos;
    bool rabbitActive;
    bool saplingDropped; // 兔子是否掉落了树苗

    Vector2 grassPatchPos;
    bool isSoil;
    bool isSaplingPlanted; // 是否已经种下树苗
    bool hasTree;          // 是否长成大树

    Rectangle riverRects[3];

    // 状态与UI
    MG2State state;
    
    // 对话分页系统数据
    DialogueLine dialogueQueue[MAX_DIALOGUE_LINES]; 
    int dialogueCount;        
    int currentDialogueIndex; 
    bool showDialogue;        

    float notificationTimer;
    char notificationText[128];

    // QTE 数据
    float shakeTimer;
    float qteTimer;
} mg2 = {0};

/* 辅助函数声明 */
static void ShowDialog(const char* speaker, const char* text);
static void ShowNotification(const char* text);
static float GetDistance(Vector2 p1, Vector2 p2);
static void ExitMinigame2(const char* nextScene);
static void AddToInventory(int itemId); 

void InitMinigame2(void) {
    // 加载资源
    mg2.bgMap = LoadTexture("UI/map.png");
    mg2.playerTex = LoadTexture("UI/player_sprite.png"); 
    mg2.treeTex = LoadTexture("UI/tree.png");
    mg2.holeTex = LoadTexture("UI/blackhole.png");

    // 加载工具栏与物品资源
    mg2.inventorySlotTex = LoadTexture("UI/slot.png");
    mg2.itemBucketEmptyTex = LoadTexture("UI/bucket_empty.png");
    mg2.itemBucketWaterTex = LoadTexture("UI/bucket_water.png");
    mg2.itemSaplingTex = LoadTexture("UI/sapling.png");

    // 初始化工具栏
    for (int i = 0; i < MG2_MAX_INVENTORY; i++) mg2.inventory[i] = -1;
    mg2.selectedSlot = -1;

    // 初始化玩家 (屏幕中央)
    mg2.playerPos = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    mg2.speed = 250.0f;
    mg2.currentDir = DIR_DOWN;
    mg2.currentFrame = 0;
    mg2.frameTimer = 0.0f;
    mg2.isMoving = false;

    // 初始化交互物位置
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    mg2.rabbitPos = (Vector2){ sw * 0.2f, sh * 0.3f };
    mg2.rabbitActive = true;
    mg2.saplingDropped = false;

    mg2.grassPatchPos = (Vector2){ sw * 0.4f, sh * 0.6f };
    mg2.isSoil = false;
    mg2.isSaplingPlanted = false;
    mg2.hasTree = false;

    // 1. 右上角纵向段
    mg2.riverRects[0] = (Rectangle){ sw * 0.75f, 0, sw * 0.25f, sh * 0.4f };
    // 2. 中间斜向过渡段
    mg2.riverRects[1] = (Rectangle){ sw * 0.65f, sh * 0.35f, sw * 0.2f, sh * 0.4f };
    // 3. 左下角横向/斜向段
    mg2.riverRects[2] = (Rectangle){ sw * 0.4f, sh * 0.7f, sw * 0.35f, sh * 0.3f };
    
    mg2.state = MG2_INTRO;
    mg2.showDialogue = false;
    mg2.dialogueCount = 0;        
    mg2.currentDialogueIndex = 0; 
    mg2.notificationTimer = 0.0f;
    mg2.shakeTimer = 0.0f;

    // 触发开场对话
    ShowDialog("Game", "Fine. You want to play a game? Then play.");
    ShowDialog("Game", "Enough. This little space is plenty for you to play with.");
    ShowDialog("Game", "If you're truly bored, go rustle through the grass or tease the rabbits.");
}

void UpdateMinigame2(void) {
    float dt = GetFrameTime();
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    Vector2 mouse = GetMousePosition();

    if (mg2.notificationTimer > 0) mg2.notificationTimer -= dt;

    // 对话框拦截与翻页逻辑
    if (mg2.showDialogue) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            mg2.currentDialogueIndex++;
            if (mg2.currentDialogueIndex >= mg2.dialogueCount) {
                mg2.showDialogue = false; 
                MG2State finishedState = mg2.state;
                mg2.dialogueCount = 0;
                mg2.currentDialogueIndex = 0;

                if (finishedState == MG2_INTRO) {
                    mg2.state = MG2_PLAYING;
                    AddToInventory(ITEM_BUCKET_EMPTY);
                    ShowNotification("Obtained: Iron Bucket x1");
                } else if (finishedState == MG2_BOSS_ANGRY) {
                    mg2.state = MG2_QTE;
                    mg2.qteTimer = 3.0f; 
                }
            }
        }
        return; 
    }

    // QTE 阶段
    if (mg2.state == MG2_QTE) {
        mg2.qteTimer -= dt;
        mg2.shakeTimer = 1.0f;
        Rectangle btnRect = { sw/2.0f - 100, sh/2.0f + 100, 200, 60 };
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, btnRect)) {
            ExitMinigame2("scene5");
            return;
        }
        if (mg2.qteTimer <= 0) {
            ExitMinigame2("ending1");
            return;
        }
        return;
    }

    // 树木生长震动
    if (mg2.shakeTimer > 0) {
        mg2.shakeTimer -= dt;
        if (mg2.shakeTimer <= 0 && mg2.state == MG2_GROWING) {
            mg2.state = MG2_BOSS_ANGRY;
            ShowDialog("Game", "Arrrrr! What are you doing in my game?");
            ShowDialog("Game", "Now take your ridiculous tree and get off my server!");
        }
    }

    // 玩家移动控制
    if (mg2.state == MG2_PLAYING || mg2.state == MG2_GROWING) {
        mg2.isMoving = false;
        Vector2 nextPos = mg2.playerPos;

        if (IsKeyDown(KEY_W)) { nextPos.y -= mg2.speed * dt; mg2.currentDir = DIR_UP; mg2.isMoving = true; }
        if (IsKeyDown(KEY_S)) { nextPos.y += mg2.speed * dt; mg2.currentDir = DIR_DOWN; mg2.isMoving = true; }
        if (IsKeyDown(KEY_A)) { nextPos.x -= mg2.speed * dt; mg2.currentDir = DIR_LEFT; mg2.isMoving = true; }
        if (IsKeyDown(KEY_D)) { nextPos.x += mg2.speed * dt; mg2.currentDir = DIR_RIGHT; mg2.isMoving = true; }

        if (nextPos.x > 30 && nextPos.x < sw - 30) mg2.playerPos.x = nextPos.x;
        if (nextPos.y > 50 && nextPos.y < sh - 20) mg2.playerPos.y = nextPos.y;

        if (mg2.isMoving) {
            mg2.frameTimer += dt;
            if (mg2.frameTimer > 0.15f) {
                mg2.currentFrame = (mg2.currentFrame + 1) % 4;
                mg2.frameTimer = 0.0f;
            }
        } else {
            mg2.currentFrame = 0;
        }

        // --- 工具栏点击逻辑 ---
        bool clickedUI = false;
        int slotSize = 80;
        int startX = 100;
        int slotY = sh - 120;
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            for (int slot = 0; slot < MG2_MAX_INVENTORY; slot++) {
                Rectangle slotRect = { startX + slot * (slotSize + 10), slotY, slotSize, slotSize };
                if (CheckCollisionPointRec(mouse, slotRect)) {
                    clickedUI = true;
                    if (mg2.inventory[slot] != -1) {
                        mg2.selectedSlot = (mg2.selectedSlot == slot) ? -1 : slot;
                    } else {
                        mg2.selectedSlot = -1;
                    }
                    break;
                }
            }
        }

        // --- 场景交互逻辑 ---
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !clickedUI) {
            float interactRange = 150.0f;

            // 1. 兔子交互 -> 掉落树苗
            if (GetDistance(mg2.playerPos, mg2.rabbitPos) < interactRange) {
                if (mg2.rabbitActive) {
                    mg2.rabbitActive = false;
                    mg2.saplingDropped = true;
                    ShowNotification("The rabbit hopped away and left a sapling.");
                } else if (mg2.saplingDropped) {
                    mg2.saplingDropped = false;
                    AddToInventory(ITEM_SAPLING); // 放入背包
                    ShowNotification("Obtained: Sapling x1");
                }
            }

            // 2. 土地 / 种树 / 浇水
            if (GetDistance(mg2.playerPos, mg2.grassPatchPos) < interactRange) {
                if (!mg2.isSoil) {
                    // 步骤A: 拔草变土地
                    mg2.isSoil = true;
                    ShowNotification("Grass removed. It turned into tilled soil.");
                } else if (mg2.isSoil && !mg2.isSaplingPlanted && !mg2.hasTree) {
                    // 步骤B: 种下树苗
                    if (mg2.selectedSlot != -1 && mg2.inventory[mg2.selectedSlot] == ITEM_SAPLING) {
                        mg2.isSaplingPlanted = true;
                        // 从工具栏移除树苗
                        mg2.inventory[mg2.selectedSlot] = -1; 
                        mg2.selectedSlot = -1;
                        ShowNotification("You planted the sapling. Now it needs water.");
                    } else {
                        ShowNotification("The soil is empty. You need something to plant.");
                    }
                } else if (mg2.isSaplingPlanted && !mg2.hasTree) {
                    // 步骤C: 浇水长大
                    if (mg2.selectedSlot != -1 && mg2.inventory[mg2.selectedSlot] == ITEM_BUCKET_WATER) {
                        mg2.hasTree = true;
                        // 水用完，变回空桶
                        mg2.inventory[mg2.selectedSlot] = ITEM_BUCKET_EMPTY; 
                        mg2.selectedSlot = -1; 
                        
                        mg2.state = MG2_GROWING;
                        mg2.shakeTimer = 2.0f;
                        ShowNotification("You poured water. A massive tree is growing!");
                    } else {
                        ShowNotification("The sapling looks dry. It needs water.");
                    }
                }
            }

            // 3. 河边打水
            bool isInRiver = false;
            for (int i = 0; i < 3; i++) {
                if (CheckCollisionPointRec(mg2.playerPos, mg2.riverRects[i])) {
                    isInRiver = true;
                    break;
                }
            }

            if (isInRiver) {
                if (mg2.selectedSlot != -1 && mg2.inventory[mg2.selectedSlot] == ITEM_BUCKET_EMPTY) {
                    mg2.inventory[mg2.selectedSlot] = ITEM_BUCKET_WATER; 
                    mg2.selectedSlot = -1; 
                    ShowNotification("Bucket filled with water.");
                }
            }
                    }
                }
            }

void DrawMinigame2(void) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    
    float offsetX = 0, offsetY = 0;
    if (mg2.shakeTimer > 0) {
        offsetX = GetRandomValue(-10, 10);
        offsetY = GetRandomValue(-10, 10);
    }

    // 背景
    DrawTexturePro(mg2.bgMap, 
        (Rectangle){0, 0, (float)mg2.bgMap.width, (float)mg2.bgMap.height}, 
        (Rectangle){offsetX, offsetY, (float)sw, (float)sh}, 
        (Vector2){0,0}, 0.0f, WHITE);

    // 兔子或掉落的树苗
    if (mg2.rabbitActive) {
        DrawCircleV((Vector2){mg2.rabbitPos.x + offsetX, mg2.rabbitPos.y + offsetY}, 20, PINK);
        DrawText("Rabbit", mg2.rabbitPos.x - 20 + offsetX, mg2.rabbitPos.y - 40 + offsetY, 20, BLACK);
    } else if (mg2.saplingDropped) {
        // 画一个绿色小倒三角代表掉落的树苗（如果没有贴图）
        if (mg2.itemSaplingTex.id > 0) {
            DrawTexture(mg2.itemSaplingTex, mg2.rabbitPos.x - mg2.itemSaplingTex.width/2 + offsetX, mg2.rabbitPos.y - mg2.itemSaplingTex.height/2 + offsetY, WHITE);
        } else {
            DrawTriangle((Vector2){mg2.rabbitPos.x + offsetX, mg2.rabbitPos.y - 15 + offsetY},
                         (Vector2){mg2.rabbitPos.x - 10 + offsetX, mg2.rabbitPos.y + 10 + offsetY},
                         (Vector2){mg2.rabbitPos.x + 10 + offsetX, mg2.rabbitPos.y + 10 + offsetY}, LIME);
        }
    }

    // 土地状态绘制
    if (!mg2.isSoil) {
        // 未开垦的草地
        DrawRectangle(mg2.grassPatchPos.x - 30 + offsetX, mg2.grassPatchPos.y - 30 + offsetY, 60, 60, DARKGREEN);
    } else {
        // 翻好的泥土
        DrawRectangle(mg2.grassPatchPos.x - 30 + offsetX, mg2.grassPatchPos.y - 30 + offsetY, 60, 60, BROWN);
        
        if (mg2.hasTree) {
            // 长成的大树
            if (mg2.treeTex.id > 0) {
                DrawTexture(mg2.treeTex, mg2.grassPatchPos.x - mg2.treeTex.width/2 + offsetX, mg2.grassPatchPos.y - mg2.treeTex.height + offsetY, WHITE);
            } else {
                DrawRectangle(mg2.grassPatchPos.x - 20 + offsetX, mg2.grassPatchPos.y - 200 + offsetY, 40, 200, MAROON);
                DrawCircle(mg2.grassPatchPos.x + offsetX, mg2.grassPatchPos.y - 200 + offsetY, 80, GREEN);
            }
        } else if (mg2.isSaplingPlanted) {
            // 种下的树苗（较小）
            if (mg2.itemSaplingTex.id > 0) {
                DrawTexture(mg2.itemSaplingTex, mg2.grassPatchPos.x - mg2.itemSaplingTex.width/2 + offsetX, mg2.grassPatchPos.y - mg2.itemSaplingTex.height/2 + offsetY, WHITE);
            } else {
                DrawRectangle(mg2.grassPatchPos.x - 5 + offsetX, mg2.grassPatchPos.y - 20 + offsetY, 10, 20, DARKGREEN);
                DrawCircle(mg2.grassPatchPos.x + offsetX, mg2.grassPatchPos.y - 25 + offsetY, 15, LIME);
            }
        }
    }

    // 玩家
    if (mg2.playerTex.id > 0) {
        float frameWidth = (float)mg2.playerTex.width / 4;
        float frameHeight = (float)mg2.playerTex.height / 4;
        Rectangle sourceRec = { mg2.currentFrame * frameWidth, mg2.currentDir * frameHeight, frameWidth, frameHeight };
        Rectangle destRec = { mg2.playerPos.x + offsetX, mg2.playerPos.y + offsetY, frameWidth * 0.2, frameHeight * 0.2 };
        destRec.x -= destRec.width / 2;
        destRec.y -= destRec.height / 2;
        DrawTexturePro(mg2.playerTex, sourceRec, destRec, (Vector2){0,0}, 0.0f, WHITE);
    } else {
        DrawRectangle(mg2.playerPos.x - 20 + offsetX, mg2.playerPos.y - 40 + offsetY, 40, 80, BLUE);
    }

    // QTE 界面
    if (mg2.state == MG2_QTE) {
        DrawRectangle(0, 0, sw, sh, (Color){0,0,0, 150});
        DrawCircle(sw/2, sh/2, 150 + GetRandomValue(-10, 10), BLACK);
        DrawText("PULLING YOU IN...", sw/2 - 150, sh/2 - 200, 40, RED);
        Rectangle btnRect = { sw/2.0f - 100, sh/2.0f + 100, 200, 60 };
        DrawRectangleRec(btnRect, RED);
        DrawRectangleLinesEx(btnRect, 3, WHITE);
        DrawText("Struggle!", btnRect.x + 35, btnRect.y + 15, 30, WHITE);
        DrawText(TextFormat("%.1f", mg2.qteTimer), sw/2 - 30, sh/2, 50, WHITE);
    }

    // 通知横幅
    if (mg2.notificationTimer > 0) {
        int tw = MeasureText(mg2.notificationText, 24);
        DrawRectangle(sw/2 - tw/2 - 20, 20, tw + 40, 50, (Color){0,0,0,200});
        DrawText(mg2.notificationText, sw/2 - tw/2, 33, 24, YELLOW);
    }

    // 底部工具栏
    int slotSize = 80;
    int startX = 100;
    int slotY = sh - 120;
    for (int slot = 0; slot < MG2_MAX_INVENTORY; slot++) {
        Rectangle slotRect = { startX + slot * (slotSize + 10), slotY, slotSize, slotSize };

        if (mg2.inventorySlotTex.id > 0) {
            DrawTexturePro(mg2.inventorySlotTex,
                (Rectangle){ 0, 0, (float)mg2.inventorySlotTex.width, (float)mg2.inventorySlotTex.height },
                (Rectangle){ slotRect.x, slotRect.y, slotRect.width, slotRect.height },
                (Vector2){ 0, 0 }, 0.0f, WHITE);
        } else {
            DrawRectangleRec(slotRect, LIGHTGRAY);
            DrawRectangleLinesEx(slotRect, 2, DARKGRAY);
        }

        if (mg2.inventory[slot] != -1) {
            int itemId = mg2.inventory[slot];
            Texture2D tex = {0};
            Color fallbackColor = BLANK;

            if (itemId == ITEM_BUCKET_EMPTY) { tex = mg2.itemBucketEmptyTex; fallbackColor = GRAY; }
            else if (itemId == ITEM_BUCKET_WATER) { tex = mg2.itemBucketWaterTex; fallbackColor = BLUE; }
            else if (itemId == ITEM_SAPLING) { tex = mg2.itemSaplingTex; fallbackColor = LIME; }

            if (tex.id > 0) {
                float scale = fminf((slotSize - 10) / (float)tex.width, (slotSize - 10) / (float)tex.height);
                int drawW = (int)(tex.width * scale);
                int drawH = (int)(tex.height * scale);
                int drawX = slotRect.x + (slotSize - drawW) / 2;
                int drawY = slotRect.y + (slotSize - drawH) / 2;
                DrawTextureEx(tex, (Vector2){ drawX, drawY }, 0.0f, scale, WHITE);
            } else {
                DrawRectangle(slotRect.x + 20, slotRect.y + 20, 40, 40, fallbackColor);
            }
        }

        if (slot == mg2.selectedSlot) {
            DrawRectangleLinesEx(slotRect, 4, YELLOW);
        }
    }

    // 对话框
    if (mg2.showDialogue) {
        int boxY = sh - 200;
        DialogueLine *currentLine = &mg2.dialogueQueue[mg2.currentDialogueIndex];

        DrawRectangle(20, boxY, sw - 40, 180, (Color){0,0,0,220});
        DrawText(currentLine->speaker, 50, boxY + 20, 30, RED);
        DrawText(currentLine->text, 50, boxY + 70, 24, WHITE);
        
        if (mg2.currentDialogueIndex < mg2.dialogueCount - 1) {
            DrawText("Click for next page...", sw - 280, boxY + 145, 20, GRAY);
        } else {
            DrawText("Click to continue", sw - 250, boxY + 145, 20, YELLOW);
        }
    }
}

void UnloadMinigame2(void) {
    if (mg2.bgMap.id > 0) UnloadTexture(mg2.bgMap);
    if (mg2.playerTex.id > 0) UnloadTexture(mg2.playerTex);
    if (mg2.treeTex.id > 0) UnloadTexture(mg2.treeTex);
    if (mg2.holeTex.id > 0) UnloadTexture(mg2.holeTex);
    
    if (mg2.inventorySlotTex.id > 0) UnloadTexture(mg2.inventorySlotTex);
    if (mg2.itemBucketEmptyTex.id > 0) UnloadTexture(mg2.itemBucketEmptyTex);
    if (mg2.itemBucketWaterTex.id > 0) UnloadTexture(mg2.itemBucketWaterTex);
    if (mg2.itemSaplingTex.id > 0) UnloadTexture(mg2.itemSaplingTex);

    memset(&mg2, 0, sizeof(mg2));
}

// --- 辅助函数实现 ---

static void AddToInventory(int itemId) {
    for (int i = 0; i < MG2_MAX_INVENTORY; i++) {
        if (mg2.inventory[i] == -1) {
            mg2.inventory[i] = itemId;
            break;
        }
    }
}

static void ShowDialog(const char* speaker, const char* text) {
    if (mg2.dialogueCount < MAX_DIALOGUE_LINES) {
        strncpy(mg2.dialogueQueue[mg2.dialogueCount].speaker, speaker, 63);
        strncpy(mg2.dialogueQueue[mg2.dialogueCount].text, text, 255);
        mg2.dialogueCount++;
        mg2.showDialogue = true;
    }
}

static void ShowNotification(const char* text) {
    strncpy(mg2.notificationText, text, 127);
    mg2.notificationTimer = 3.0f;
}

static float GetDistance(Vector2 p1, Vector2 p2) {
    return sqrtf((p1.x - p2.x)*(p1.x - p2.x) + (p1.y - p2.y)*(p1.y - p2.y));
}

static void ExitMinigame2(const char* nextScene) {
    char target[32];
    strncpy(target, nextScene, 31);
    UnloadMinigame2();
    game.current_scene = GetSceneByID(target);
    if (game.current_scene) {
        game.state = STATE_PLAYING;
        game.dialogue_index = 0;
        game.auto_timer = 0.0f;
    } else {
        game.state = STATE_TITLE;
    }
}
