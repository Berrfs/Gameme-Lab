/* minigame.c — 剧场后台小游戏模块
   实现第一视角点击箭头切换墙壁、物品互动、工具栏、简单对话。
   通过全局 game 变量与主游戏交互，支持退出返回标题或继续故事。
   编写者：周沐格
   日期：2026-03-18
*/
#include "game.h"      
#include "scene.h"     
#include "minigame.h"
#include "raylib.h"
#include <string.h>
#include <stdio.h>

// 引用主游戏上下文（定义在 game.c 中）
extern GameContext game;

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
} mg = {0};   // 初始化为零

/* ------------------ 静态辅助函数声明 ------------------ */
static void ShowDialogue(const char* speaker, const char* text);
static void PickUpItem(int itemIndex);
static void UseItemWithItem(int toolItemId, int targetItemId);  // 组合物品示例
static void ExitToTitle(void);
static void ExitToStory(const char* sceneId);

/* ------------------ 初始化小游戏 ------------------ */
void InitMinigame(void) {
    // 加载三面墙背景（请根据实际文件路径修改）
    mg.wallTextures[0] = LoadTexture("assets/backstage_wall1.png");
    mg.wallTextures[1] = LoadTexture("assets/backstage_wall2.png");
    mg.wallTextures[2] = LoadTexture("assets/backstage_wall3.png");

    // 加载箭头纹理
    mg.arrowLeft  = LoadTexture("UI/arrow_left.png");
    mg.arrowRight = LoadTexture("UI/arrow_right.png");

    // 加载工具栏格子纹理（如果没有，可省略，用 DrawRectangle 替代）
    mg.inventorySlot = LoadTexture("UI/slot.png");

    // 初始化当前墙面
    mg.currentWall = 0;

    // ---------- 定义物品 ----------
    mg.itemCount = 4;   // 示例设置4个物品

    // 物品0：画（位于墙0）
    mg.items[0] = (Item){
        .id = 0,
        .name = "Picture",
        .texture = LoadTexture("UI/picture.png"),
        .wallIndex = 0,
        .interactRect = { 600, 500, 80, 80 },   // 相对屏幕坐标的点击区域
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
        .texture = LoadTexture("items/script.png"),
        .wallIndex = 2,
        .interactRect = { 500, 300, 100, 60 },
        .isPickedUp = false,
        .visible = true
    };

    // 物品3：一扇门（位于墙2）
    mg.items[3] = (Item){
        .id = 3,
        .name = "Door",
        .texture = LoadTexture("items/door.png"),
        .wallIndex = 2,
        .interactRect = { 800, 200, 120, 200 },
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

    TraceLog(LOG_INFO, "Minigame initialized.");
}

/* ------------------ 每帧更新（输入 + 逻辑） ------------------ */
void UpdateMinigame(void) {
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
        if (it->wallIndex == mg.currentWall && !it->isPickedUp && it->visible) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                CheckCollisionPointRec(mouse, it->interactRect)) {
                // 检查当前是否有选中工具栏物品
                if (mg.selectedSlot != -1 && mg.inventory[mg.selectedSlot] != -1) {
                    // 有选中物品，尝试组合使用
                    int toolId = mg.inventory[mg.selectedSlot];
                    UseItemWithItem(toolId, it->id);
                } else {
                    // 无选中物品，直接拾取（如果物品是可拾取的）
                    // 门是不可拾取的，我们通过 id 判断
                    if (it->id == 3) {   // 门：触发退出
                        ShowDialogue("Narrator", "You push the door open and step outside...");
                        // 设置要返回的故事场景ID（例如 "scene3"）
                        strcpy(mg.nextSceneId, "scene3");
                        mg.completed = true;   // 标记完成，下一帧退出
                    } else {
                        // 普通物品：拾取
                        PickUpItem(i);
                    }
                }
                break;   // 一次只处理一个物品点击
            }
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
        // 退出小游戏，返回故事
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
        if (it->wallIndex == mg.currentWall && !it->isPickedUp && it->visible) {
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

// 组合物品示例：当使用选中物品与场景物品交互时调用
static void UseItemWithItem(int toolItemId, int targetItemId) {
    // 这里可以根据游戏设计实现各种组合逻辑
    // 例如：使用钥匙开锁、使用胶水修复物品等
    // 本示例仅演示反馈对话
    const char* toolName = "Unknown";
    const char* targetName = "Unknown";
    for (int i = 0; i < mg.itemCount; i++) {
        if (mg.items[i].id == toolItemId) toolName = mg.items[i].name;
        if (mg.items[i].id == targetItemId) targetName = mg.items[i].name;
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "Using %s on %s... Nothing happens.", toolName, targetName);
    ShowDialogue("Narrator", buf);

    // 如果组合成功，可以设置 mg.completed = true 并指定下一场景
    // 例如：使用钥匙开门
    if (toolItemId == 0 && targetItemId == 3) {   // 假设梳子（id=0）对门（id=3）无效
        // 无效果
    }
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
