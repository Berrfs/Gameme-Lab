/* minigame.c — 剧场后台小游戏模块
   实现第一视角点击箭头切换墙壁、物品互动、工具栏、简单对话。
   新增功能：
     - 门移至墙1，点击提示上锁。
     - 画框与画的放大交互：点击画可拾取/放回，配合钉子和锤子完成谜题生成钥匙。
     - 打字机放大模式支持键盘输入，按顺序识别单词 "time", "walk", "bride"，正确后自动退出并生成钉子。
     - 墙2添加锤子物品，可拾取。
   编写者：周沐格
   日期：2026-03-22
*/
#include "game.h"      
#include "scene.h"     
#include "minigame.h"
#include "raylib.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

// 引用主游戏上下文（定义在 game.c 中）
extern GameContext game;

// 小游戏模式（正常/放大画/放大打字机）
typedef enum {
    MG_MODE_NORMAL,
    MG_MODE_ZOOM_PICTURE,
    MG_MODE_ZOOM_TYPEWRITER
} MinigameMode;

// 画的状态
typedef enum {
    PICTURE_STATE_IN_FRAME,      // 在画框内（初始）
    PICTURE_STATE_PICKED_UP,     // 被拾取（在物品栏）
    PICTURE_STATE_NAILED,        // 已被钉钉子（仍在框内）
    PICTURE_STATE_COMPLETED      // 已完成（生成钥匙后不可再交互）
} PictureState;

// 小游戏内部静态上下文，所有状态对外部不可见
static struct {
    // 三面墙的背景纹理
    Texture2D wallTextures[WALL_COUNT];

    // 左右箭头纹理
    Texture2D arrowLeft;
    Texture2D arrowRight;

    // 工具栏格子背景（可选，也可以用纯色矩形代替）
    Texture2D inventorySlot;

    // 当前所在的墙面索引 (0,1,2)
    int currentWall;

    // 物品数组及数量
    Item items[MAX_ITEMS];
    int itemCount;

    // 工具栏：每个格子存放物品ID，-1表示空
    int inventory[MAX_INVENTORY];
    int selectedSlot;           // 当前选中的格子索引，-1表示无选中

    // 对话相关
    struct {
        char speaker[64];
        char text[256];
        bool active;
        float timer;            // 可用来实现自动消失（本示例中由鼠标点击关闭）
    } dialogue;

    // 是否已经完成小游戏（用于触发返回故事）
    bool completed;

    // 如果需要返回故事，记录下一个场景ID
    char nextSceneId[32];

    // ----- 放大模式相关 -----
    MinigameMode mode;                  // 当前模式
    int zoomItemId;                     // 当前放大的物品ID（画0或打字机1）
    char typewriterInput[256];          // 打字机输入的文本

    // ----- 打字机单词识别新增 -----
    Texture2D wordImages[3];            // 对应 time, walk, bride 的图片纹理
    int typewriterStep;                 // 0:等待time, 1:等待walk, 2:等待bride, 3:已完成

    // ----- 画的状态机 -----
    PictureState pictureState;          // 当前画的状态
} mg = {0};

/* ------------------ 静态辅助函数声明 ------------------ */
static void ShowDialogue(const char* speaker, const char* text);
static void PickUpItem(int itemIndex);
static void UseItemWithItem(int toolItemId, int targetItemId);
static void ExitToTitle(void);
static void ExitToStory(const char* sceneId);
static void EnterZoomPicture(void);
static void EnterZoomTypewriter(void);
static void UpdateZoomPicture(void);
static void UpdateZoomTypewriter(void);
static void DrawZoomPicture(void);
static void DrawZoomTypewriter(void);
static void DrawExitArrow(int x, int y, int size, Color color);
static void SpawnNail(void);            // 生成钉子（打字机完成后）
static void SpawnKey(void);             // 生成钥匙（锤子完成后）

/* ------------------ 初始化小游戏 ------------------ */
void InitMinigame(void) {
    // 加载三面墙背景（请根据实际文件路径修改）
    mg.wallTextures[0] = LoadTexture("UI/computer.jpg");
    mg.wallTextures[1] = LoadTexture("UI/red curtain.jpg");
    mg.wallTextures[2] = LoadTexture("assets/backstage_wall3.png");

    // 加载箭头纹理
    mg.arrowLeft  = LoadTexture("UI/arrow_left.png");
    mg.arrowRight = LoadTexture("UI/arrow_right.png");

    // 加载工具栏格子纹理（如果没有，可省略，用 DrawRectangle 替代）
    mg.inventorySlot = LoadTexture("UI/slot.png");

    // 初始化当前墙面
    mg.currentWall = 0;

    // ---------- 定义物品 ----------
    mg.itemCount = 7;   // 画(0)、打字机(1)、剧本(2)、门(3)、钥匙(4)、钉子(5)、锤子(6)

    // 物品0：画（位于墙0）
    mg.items[0] = (Item){
        .id = 0,
        .name = "Picture",
        .texture = LoadTexture("UI/picture.png"),
        .wallIndex = 0,
        .interactRect = { 600, 500, 80, 80 },
        .isPickedUp = false,
        .visible = true
    };

    // 物品1：打字机（位于墙1）
    mg.items[1] = (Item){
        .id = 1,
        .name = "Typewriter",
        .texture = LoadTexture("UI/typewriter.png"),
        .wallIndex = 1,
        .interactRect = { 700, 400, 70, 70 },
        .isPickedUp = false,
        .visible = true
    };

    // 物品2：剧本（位于墙2）
    mg.items[2] = (Item){
        .id = 2,
        .name = "Script",
        .texture = LoadTexture("UI/script.png"),
        .wallIndex = 2,
        .interactRect = { 500, 300, 100, 60 },
        .isPickedUp = false,
        .visible = true
    };

    // 物品3：门（移至墙1，默认锁住）
    mg.items[3] = (Item){
        .id = 3,
        .name = "Door",
        .texture = LoadTexture("UI/door.png"),
        .wallIndex = 1,          // 改为墙1
        .interactRect = { 800, 200, 120, 200 },
        .isPickedUp = false,
        .visible = true
    };

    // 物品4：钥匙（初始不可见，锤子完成后生成）
    mg.items[4] = (Item){
        .id = 4,
        .name = "Key",
        .texture = LoadTexture("UI/key.png"),
        .wallIndex = 0,
        .interactRect = { 600, 500, 40, 40 },  // 与画同位置
        .isPickedUp = false,
        .visible = false
    };

    // 物品5：钉子（初始不可见，打字机完成后生成）
    mg.items[5] = (Item){
        .id = 5,
        .name = "Nail",
        .texture = LoadTexture("UI/nail.png"),
        .wallIndex = 0,
        .interactRect = { 600, 550, 30, 30 },  // 画下方一点
        .isPickedUp = false,
        .visible = false
    };

    // 物品6：锤子（墙2可见，可拾取）
    mg.items[6] = (Item){
        .id = 6,
        .name = "Hammer",
        .texture = LoadTexture("UI/hammer.png"),
        .wallIndex = 2,
        .interactRect = { 300, 400, 60, 60 },
        .isPickedUp = false,
        .visible = true
    };

    // 清空工具栏
    for (int i = 0; i < MAX_INVENTORY; i++) mg.inventory[i] = -1;
    mg.selectedSlot = -1;

    // 清空对话状态
    mg.dialogue.active = false;
    mg.dialogue.timer = 0.0f;

    // 标记小游戏未完成
    mg.completed = false;
    mg.nextSceneId[0] = '\0';

    // 放大模式初始化
    mg.mode = MG_MODE_NORMAL;
    mg.zoomItemId = -1;
    mg.typewriterInput[0] = '\0';

    // 加载打字机单词图片
    mg.wordImages[0] = LoadTexture("UI/time.png");
    mg.wordImages[1] = LoadTexture("UI/walk.png");
    mg.wordImages[2] = LoadTexture("UI/bride.png");
    mg.typewriterStep = 0;  // 初始等待 "time"

    // 画的状态初始为在框内
    mg.pictureState = PICTURE_STATE_IN_FRAME;

    TraceLog(LOG_INFO, "Minigame initialized.");
}

/* ------------------ 每帧更新（输入 + 逻辑） ------------------ */
void UpdateMinigame(void) {
    // 放大模式优先处理
    if (mg.mode == MG_MODE_ZOOM_PICTURE) {
        UpdateZoomPicture();
        return;
    }
    if (mg.mode == MG_MODE_ZOOM_TYPEWRITER) {
        UpdateZoomTypewriter();
        return;
    }

    Vector2 mouse = GetMousePosition();
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // 如果对话处于激活状态，点击任意位置关闭对话（本示例采用点击关闭）
    if (mg.dialogue.active) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            mg.dialogue.active = false;
        }
        // 对话激活时不处理其他输入（可选，也可允许同时操作）
        return;
    }

    // ----- 1. 切换墙壁（左右箭头）-----
    // 定义箭头点击区域（可根据屏幕大小调整）
    Rectangle leftArrowRect  = { 20, sh/2 - 50, 80, 100 };
    Rectangle rightArrowRect = { sw - 100, sh/2 - 50, 80, 100 };

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (CheckCollisionPointRec(mouse, leftArrowRect)) {
            mg.currentWall = (mg.currentWall - 1 + WALL_COUNT) % WALL_COUNT;
        }
        else if (CheckCollisionPointRec(mouse, rightArrowRect)) {
            mg.currentWall = (mg.currentWall + 1) % WALL_COUNT;
        }
    }

    // ----- 2. 物品交互（点击物品）-----
    for (int i = 0; i < mg.itemCount; i++) {
        Item* it = &mg.items[i];
        // 仅当物品在当前墙、未被拾取、且可见时，才能被点击
        bool visibleOnWall = (it->wallIndex == mg.currentWall && it->visible && !it->isPickedUp);
        // 对于画，如果状态为在框内才显示，其他状态不显示
        if (it->id == 0 && mg.pictureState != PICTURE_STATE_IN_FRAME) {
            visibleOnWall = false;
        }
        if (visibleOnWall && CheckCollisionPointRec(mouse, it->interactRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            // 检查当前是否有选中工具栏物品
            if (mg.selectedSlot != -1 && mg.inventory[mg.selectedSlot] != -1) {
                // 有选中物品，尝试组合使用
                int toolId = mg.inventory[mg.selectedSlot];
                UseItemWithItem(toolId, it->id);
            } else {
                // 无选中物品，根据物品类型处理
                if (it->id == 0) {          // 画：进入放大模式
                    EnterZoomPicture();
                } else if (it->id == 1) {    // 打字机：进入放大模式
                    EnterZoomTypewriter();
                } else if (it->id == 3) {    // 门：显示锁住提示
                    ShowDialogue("Narrator", "The door is locked, find a way out.");
                } else {
                    // 其他物品（剧本、钉子、锤子、钥匙）可拾取
                    PickUpItem(i);
                }
            }
            break;   // 一次只处理一个物品点击
        }
    }

    // ----- 3. 工具栏点击（选择/取消选中）-----
    int slotSize = 80;
    int startX = 100;
    int slotY = sh - 120;
    for (int slot = 0; slot < MAX_INVENTORY; slot++) {
        Rectangle slotRect = { startX + slot * (slotSize + 10), slotY, slotSize, slotSize };
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, slotRect)) {
            if (mg.inventory[slot] != -1) {
                // 如果点击的是同一个格子，取消选中；否则切换选中
                mg.selectedSlot = (mg.selectedSlot == slot) ? -1 : slot;
            } else {
                // 空格子点击取消选中
                mg.selectedSlot = -1;
            }
        }
    }

    // ----- 4. 检查小游戏是否完成（例如门已使用）-----
    if (mg.completed) {
        ExitToStory(mg.nextSceneId);
        return;
    }

    // ----- 5. 键盘退出：按 ESC 返回标题-----
    if (IsKeyPressed(KEY_ESCAPE)) {
        ExitToTitle();
        return;
    }
}

/* ------------------ 每帧绘制 ------------------ */
void DrawMinigame(void) {
    if (mg.mode == MG_MODE_ZOOM_PICTURE) {
        DrawZoomPicture();
        return;
    }
    if (mg.mode == MG_MODE_ZOOM_TYPEWRITER) {
        DrawZoomTypewriter();
        return;
    }

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // 1. 绘制当前墙的背景（全屏拉伸）
    DrawTexturePro(mg.wallTextures[mg.currentWall],
        (Rectangle){ 0, 0,
            (float)mg.wallTextures[mg.currentWall].width,
            (float)mg.wallTextures[mg.currentWall].height },
        (Rectangle){ 0, 0, (float)sw, (float)sh },
        (Vector2){ 0, 0 }, 0.0f, WHITE);

    // 2. 绘制左右箭头（简单纹理，位置可微调）
    DrawTexture(mg.arrowLeft,  20, sh/2 - 50, WHITE);
    DrawTexture(mg.arrowRight, sw - 100, sh/2 - 50, WHITE);

    // 3. 绘制当前墙上的物品（可见且未被拾取）
    for (int i = 0; i < mg.itemCount; i++) {
        Item* it = &mg.items[i];
        bool visibleOnWall = (it->wallIndex == mg.currentWall && it->visible && !it->isPickedUp);
        // 画需要根据状态决定是否显示
        if (it->id == 0 && mg.pictureState != PICTURE_STATE_IN_FRAME) {
            visibleOnWall = false;
        }
        if (visibleOnWall) {
            DrawTextureEx(it->texture,
                (Vector2){ it->interactRect.x, it->interactRect.y },
                0.0f, 1.0f, WHITE);
        }
    }

    // 4. 绘制底部工具栏
    int slotSize = 80;
    int startX = 100;
    int slotY = sh - 120;
    for (int slot = 0; slot < MAX_INVENTORY; slot++) {
        Rectangle slotRect = { startX + slot * (slotSize + 10), slotY, slotSize, slotSize };

        // 绘制格子背景（如果有纹理，否则用纯色矩形）
        if (mg.inventorySlot.id > 0) {
            DrawTexturePro(mg.inventorySlot,
                (Rectangle){ 0, 0,
                    (float)mg.inventorySlot.width,
                    (float)mg.inventorySlot.height },
                (Rectangle){ slotRect.x, slotRect.y, slotRect.width, slotRect.height },
                (Vector2){ 0, 0 }, 0.0f, WHITE);
        } else {
            DrawRectangleRec(slotRect, LIGHTGRAY);
            DrawRectangleLinesEx(slotRect, 2, DARKGRAY);
        }

        // 如果格子有物品，绘制物品图标（按比例缩放到格子内）
        if (mg.inventory[slot] != -1) {
            int itemId = mg.inventory[slot];
            // 假设物品ID与数组下标一致（实际应建立快速查找，这里简单遍历）
            for (int i = 0; i < mg.itemCount; i++) {
                if (mg.items[i].id == itemId) {
                    Texture2D tex = mg.items[i].texture;
                    // 缩放图标以适应格子，保留宽高比
                    float scale = fminf((slotSize - 10) / (float)tex.width,
                                        (slotSize - 10) / (float)tex.height);
                    int drawW = (int)(tex.width * scale);
                    int drawH = (int)(tex.height * scale);
                    int drawX = slotRect.x + (slotSize - drawW) / 2;
                    int drawY = slotRect.y + (slotSize - drawH) / 2;
                    DrawTextureEx(tex, (Vector2){ drawX, drawY }, 0.0f, scale, WHITE);
                    break;
                }
            }
        }

        // 高亮选中的格子
        if (slot == mg.selectedSlot) {
            DrawRectangleLinesEx(slotRect, 4, YELLOW);
        }
    }

    // 5. 绘制对话（如果激活）
    if (mg.dialogue.active) {
        int boxY = sh - 300;
        int boxH = 200;
        // 半透明黑底
        DrawRectangle(0, boxY, sw, boxH, (Color){ 0, 0, 0, 200 });
        DrawText(mg.dialogue.speaker, 50, boxY + 20, 40, MAROON);
        DrawText(mg.dialogue.text, 50, boxY + 80, 36, WHITE);
        DrawText("Click to continue", sw - 300, boxY + 150, 30, GRAY);
    }
}

/* ------------------ 释放小游戏资源 ------------------ */
void UnloadMinigame(void) {
    // 卸载所有纹理
    for (int i = 0; i < WALL_COUNT; i++) {
        if (mg.wallTextures[i].id > 0) UnloadTexture(mg.wallTextures[i]);
    }
    UnloadTexture(mg.arrowLeft);
    UnloadTexture(mg.arrowRight);
    if (mg.inventorySlot.id > 0) UnloadTexture(mg.inventorySlot);
    for (int i = 0; i < mg.itemCount; i++) {
        if (mg.items[i].texture.id > 0) UnloadTexture(mg.items[i].texture);
    }
    // 卸载打字机单词图片
    for (int i = 0; i < 3; i++) {
        if (mg.wordImages[i].id > 0) UnloadTexture(mg.wordImages[i]);
    }

    // 重置 mg 结构体（可选）
    memset(&mg, 0, sizeof(mg));

    TraceLog(LOG_INFO, "Minigame unloaded.");
}

/* ------------------ 静态辅助函数实现 ------------------ */

// 显示对话（覆盖当前对话）
static void ShowDialogue(const char* speaker, const char* text) {
    strncpy(mg.dialogue.speaker, speaker, sizeof(mg.dialogue.speaker) - 1);
    mg.dialogue.speaker[sizeof(mg.dialogue.speaker) - 1] = '\0';
    strncpy(mg.dialogue.text, text, sizeof(mg.dialogue.text) - 1);
    mg.dialogue.text[sizeof(mg.dialogue.text) - 1] = '\0';
    mg.dialogue.active = true;
}

// 拾取物品到工具栏
static void PickUpItem(int itemIndex) {
    Item* it = &mg.items[itemIndex];
    // 查找第一个空工具栏格子
    for (int i = 0; i < MAX_INVENTORY; i++) {
        if (mg.inventory[i] == -1) {
            mg.inventory[i] = it->id;
            it->isPickedUp = true;
            it->visible = false;   // 从场景消失
            // 可选：显示拾取反馈对话
            char msg[128];
            snprintf(msg, sizeof(msg), "Picked up %s.", it->name);
            ShowDialogue("You", msg);
            return;
        }
    }
    // 工具栏已满，可提示
    ShowDialogue("System", "Inventory is full.");
}

// 组合物品：当使用选中物品与场景物品交互时调用
static void UseItemWithItem(int toolItemId, int targetItemId) {
    // 钥匙开门
    if (toolItemId == 4 && targetItemId == 3) {
        ShowDialogue("Narrator", "You unlocked the door with the key!");
        // 移除工具栏中的钥匙
        for (int i = 0; i < MAX_INVENTORY; i++) {
            if (mg.inventory[i] == 4) {
                mg.inventory[i] = -1;
                break;
            }
        }
        mg.selectedSlot = -1;
        // 设置完成，返回故事场景
        strcpy(mg.nextSceneId, "scene3");
        mg.completed = true;
        return;
    }

    // 钉子 + 画（画必须在框内且未钉钉子）
    if (toolItemId == 5 && targetItemId == 0 && mg.pictureState == PICTURE_STATE_IN_FRAME) {
        // 消耗钉子
        for (int i = 0; i < MAX_INVENTORY; i++) {
            if (mg.inventory[i] == 5) {
                mg.inventory[i] = -1;
                break;
            }
        }
        mg.selectedSlot = -1;
        mg.pictureState = PICTURE_STATE_NAILED;
        ShowDialogue("Narrator", "You nail the picture to the frame.");
        return;
    }

    // 锤子 + 画（画必须已钉钉子且未完成）
    if (toolItemId == 6 && targetItemId == 0 && mg.pictureState == PICTURE_STATE_NAILED) {
        // 消耗锤子
        for (int i = 0; i < MAX_INVENTORY; i++) {
            if (mg.inventory[i] == 6) {
                mg.inventory[i] = -1;
                break;
            }
        }
        mg.selectedSlot = -1;
        mg.pictureState = PICTURE_STATE_COMPLETED;
        // 生成钥匙
        SpawnKey();
        ShowDialogue("Narrator", "You hammer the nail, and a key falls out!");
        return;
    }

    // 默认反馈：无效果
    const char* toolName = "Unknown";
    const char* targetName = "Unknown";
    for (int i = 0; i < mg.itemCount; i++) {
        if (mg.items[i].id == toolItemId) toolName = mg.items[i].name;
        if (mg.items[i].id == targetItemId) targetName = mg.items[i].name;
    }
    char buf[256];
    snprintf(buf, sizeof(buf), "Using %s on %s... Nothing happens.", toolName, targetName);
    ShowDialogue("Narrator", buf);
}

// 退出到标题画面
static void ExitToTitle(void) {
    UnloadMinigame();          // 释放小游戏资源
    game.state = STATE_TITLE;   // 切换主游戏状态
}

// 退出到故事模式，从指定场景继续
static void ExitToStory(const char* sceneId) {
    UnloadMinigame();          // 释放小游戏资源
    game.current_scene = GetSceneByID(sceneId);  // 获取下一个场景
    if (game.current_scene) {
        game.dialogue_index = 0;
        game.state = STATE_PLAYING;    // 切换到故事播放
        game.auto_timer = 0.0f;
    } else {
        // 如果场景不存在，回退到标题
        game.state = STATE_TITLE;
    }
}

// 生成钉子（打字机三个单词正确后调用）
static void SpawnNail(void) {
    mg.items[5].visible = true;
    mg.items[5].isPickedUp = false;
    ShowDialogue("Narrator", "A nail appears on the floor.");
}

// 生成钥匙（锤子完成组合后调用）
static void SpawnKey(void) {
    mg.items[4].visible = true;
    mg.items[4].isPickedUp = false;
}

/* ---------- 放大模式实现 ---------- */

static void EnterZoomPicture(void) {
    mg.mode = MG_MODE_ZOOM_PICTURE;
    mg.zoomItemId = 0;
}

static void EnterZoomTypewriter(void) {
    mg.mode = MG_MODE_ZOOM_TYPEWRITER;
    mg.zoomItemId = 1;
    mg.typewriterInput[0] = '\0';
}

static void UpdateZoomPicture(void) {
    Vector2 mouse = GetMousePosition();
    int sw = GetScreenWidth(), sh = GetScreenHeight();

    // 定义画框区域（放大视图中的画区域）
    Rectangle pictureZoomRect = { sw/2 - 200, sh/2 - 150, 400, 300 };

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        // 点击画区域
        if (CheckCollisionPointRec(mouse, pictureZoomRect)) {
            if (mg.pictureState == PICTURE_STATE_IN_FRAME) {
                // 拾取画
                mg.pictureState = PICTURE_STATE_PICKED_UP;
                // 将画放入物品栏
                for (int i = 0; i < MAX_INVENTORY; i++) {
                    if (mg.inventory[i] == -1) {
                        mg.inventory[i] = 0;
                        break;
                    }
                }
                ShowDialogue("You", "You take the picture out of the frame.");
            } else if (mg.pictureState == PICTURE_STATE_PICKED_UP) {
                // 画在物品栏，点击画框放回
                // 从物品栏移除画
                for (int i = 0; i < MAX_INVENTORY; i++) {
                    if (mg.inventory[i] == 0) {
                        mg.inventory[i] = -1;
                        break;
                    }
                }
                mg.pictureState = PICTURE_STATE_IN_FRAME;
                ShowDialogue("You", "You put the picture back into the frame.");
            } else if (mg.pictureState == PICTURE_STATE_NAILED || mg.pictureState == PICTURE_STATE_COMPLETED) {
                // 钉了钉子或已完成，不可再拾取/放回
                ShowDialogue("Narrator", "The picture is now fixed to the frame.");
            }
        } else {
            // 点击其他区域退出放大模式
            mg.mode = MG_MODE_NORMAL;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        mg.mode = MG_MODE_NORMAL;
    }
}

static void DrawZoomPicture(void) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, (Color){0,0,0,200});

    // 绘制画框（简单棕色框）
    int frameW = 400, frameH = 300;
    int frameX = sw/2 - frameW/2, frameY = sh/2 - frameH/2;
    DrawRectangle(frameX-20, frameY-20, frameW+40, frameH+40, BROWN);
    DrawRectangle(frameX, frameY, frameW, frameH, LIGHTGRAY);

    // 根据状态绘制画的内容
    if (mg.pictureState == PICTURE_STATE_IN_FRAME || mg.pictureState == PICTURE_STATE_NAILED) {
        DrawTextureEx(mg.items[0].texture, (Vector2){frameX, frameY}, 0, 1, WHITE);
        if (mg.pictureState == PICTURE_STATE_NAILED) {
            DrawText("(Nailed)", frameX+10, frameY+frameH-30, 20, RED);
        }
    } else if (mg.pictureState == PICTURE_STATE_PICKED_UP) {
        DrawText("The picture is missing.", frameX+50, frameY+frameH/2, 30, DARKGRAY);
    } else if (mg.pictureState == PICTURE_STATE_COMPLETED) {
        DrawTextureEx(mg.items[0].texture, (Vector2){frameX, frameY}, 0, 1, WHITE);
        DrawText("(Completed)", frameX+10, frameY+frameH-30, 20, GREEN);
    }

    // 提示文字
    if (mg.pictureState == PICTURE_STATE_IN_FRAME)
        DrawText("Click on the picture to take it", sw/2-200, sh-50, 20, WHITE);
    else if (mg.pictureState == PICTURE_STATE_PICKED_UP)
        DrawText("Click on the empty frame to put the picture back", sw/2-250, sh-50, 20, WHITE);
    else
        DrawText("Click elsewhere to exit", sw/2-120, sh-50, 20, WHITE);
    DrawText("Press ESC to exit", sw-200, sh-30, 20, WHITE);
}

static void UpdateZoomTypewriter(void) {
    // 处理键盘输入
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 126 && strlen(mg.typewriterInput) < 255) {
            int len = strlen(mg.typewriterInput);
            mg.typewriterInput[len] = (char)key;
            mg.typewriterInput[len+1] = '\0';
        }
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = strlen(mg.typewriterInput);
        if (len > 0) mg.typewriterInput[len-1] = '\0';
    }

    // 处理回车
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        char inputLower[256];
        strcpy(inputLower, mg.typewriterInput);
        for (int i = 0; inputLower[i]; i++) inputLower[i] = tolower(inputLower[i]);

        const char* requiredWords[] = { "time", "walk", "bride" };

        if (mg.typewriterStep < 3) {
            if (strcmp(inputLower, requiredWords[mg.typewriterStep]) == 0) {
                // 正确，推进 step
                mg.typewriterStep++;
                mg.typewriterInput[0] = '\0';
                if (mg.typewriterStep == 3) {
                    // 正确输入三个单词，退出放大模式，生成钉子
                    mg.mode = MG_MODE_NORMAL;
                    SpawnNail();
                }
            } else {
                // 错误，仅清空输入框
                mg.typewriterInput[0] = '\0';
            }
        } else {
            mg.typewriterInput[0] = '\0';
        }
    }

    // 处理右下角箭头退出
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    Vector2 mouse = GetMousePosition();
    Rectangle exitArrowRect = { sw - 80, sh - 80, 60, 60 };
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, exitArrowRect)) {
        mg.mode = MG_MODE_NORMAL;
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        mg.mode = MG_MODE_NORMAL;
    }
}

static void DrawZoomTypewriter(void) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();

    // 半透明背景
    DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 200 });

    // 绘制放大的打字机
    Texture2D tex = mg.items[1].texture;
    float scale = 2.5f;
    int drawW = (int)(tex.width * scale);
    int drawH = (int)(tex.height * scale);
    int drawX = sw/2 - drawW/2;
    int drawY = sh/2 - drawH/2 - 50;
    DrawTextureEx(tex, (Vector2){ drawX, drawY }, 0.0f, scale, WHITE);

    // 显示文本输入框
    Rectangle textBox = { drawX + 50, drawY + drawH - 80, drawW - 100, 60 };
    DrawRectangleRec(textBox, DARKGRAY);
    DrawRectangleLinesEx(textBox, 2, WHITE);
    DrawText(mg.typewriterInput, textBox.x + 10, textBox.y + 15, 30, LIME);

    // 绘制已解锁的单词图片（从左向右排列）
    int imgX = 100;
    int imgY = sh - 250;
    for (int i = 0; i < mg.typewriterStep; i++) {
        if (mg.wordImages[i].id > 0) {
            float imgScale = 0.5f;
            DrawTextureEx(mg.wordImages[i], (Vector2){ imgX + i * 200, imgY }, 0.0f, imgScale, WHITE);
        }
    }

    // 提示文字
    DrawText("Type the words: time, walk, bride (in order)", sw/2 - 300, sh-100, 20, WHITE);

    // 右下角绘制退出箭头
    DrawExitArrow(sw - 70, sh - 70, 40, WHITE);
    DrawText("Exit", sw - 70, sh - 30, 15, WHITE);
}

// 绘制一个简单的三角形箭头
static void DrawExitArrow(int x, int y, int size, Color color) {
    // 箭头指向右下方，这里画一个向右的箭头表示退出（可自行设计）
    Vector2 p1 = { x, y };
    Vector2 p2 = { x + size, y + size/2 };
    Vector2 p3 = { x, y + size };
    DrawTriangle(p1, p2, p3, color);
    // 加一个矩形杆
    DrawRectangle(x + size/2 - 5, y + size/2 - 3, size/2, 6, color);
}
