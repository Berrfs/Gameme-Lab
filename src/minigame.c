/* minigame.c — 剧场后台小游戏模块
   修复与优化：
     - 引入 Layout 结构体，通过中心点比例(cx, cy)和缩放倍数(scale)直观控制物品大小与位置。
     - 调整了画与画框在数组中的顺序，确保画框垫底，画覆盖在其上方。
     - 动态计算 interactRect，确保显示区域与点击判定区域 100% 吻合，且画面不变形。
*/
#include "game.h"      
#include "scene.h"     
#include "minigame.h"
#include "raylib.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

// 参考分辨率（用于基础缩放基准）
#define REF_WIDTH  1280.0f
#define REF_HEIGHT 720.0f

extern GameContext game;

typedef enum {
    MG_MODE_NORMAL,
    MG_MODE_ZOOM_PICTURE,
    MG_MODE_ZOOM_TYPEWRITER
} MinigameMode;

typedef enum {
    PICTURE_STATE_IN_FRAME,      
    PICTURE_STATE_PICKED_UP,     
    PICTURE_STATE_NAILED,        
    PICTURE_STATE_COMPLETED      
} PictureState;

// 【新增】更加直观的物品布局结构体
typedef struct {
    float cx;       // 物品中心点 X 轴相对于屏幕宽度的比例 (0.0 ~ 1.0)
    float cy;       // 物品中心点 Y 轴相对于屏幕高度的比例 (0.0 ~ 1.0)
    float scale;    // 物品相对于其原图大小的缩放倍数
} ItemLayout;

static struct {
    Texture2D wallTextures[WALL_COUNT];
    Texture2D arrowLeft;
    Texture2D arrowRight;
    Texture2D inventorySlot;

    int currentWall;

    Item items[MAX_ITEMS];
    int itemCount;

    // 存储每个物品的布局配置
    ItemLayout itemLayouts[MAX_ITEMS];

    int inventory[MAX_INVENTORY];
    int selectedSlot;           

    struct {
        char speaker[64];
        char text[256];
        bool active;
        float timer;            
    } dialogue;

    bool completed;
    char nextSceneId[32];

    MinigameMode mode;                  
    int zoomItemId;                     
    char typewriterInput[256];          

    Texture2D wordImages[3];            
    int typewriterStep;                 

    PictureState pictureState;          
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
static void SpawnNail(void);            
static void SpawnKey(void);             
static void UpdateItemRects(void);      // 动态计算精准的点击与绘制边界

/* ------------------ 初始化小游戏 ------------------ */
void InitMinigame(void) {
    mg.wallTextures[0] = LoadTexture("UI/bg_wall_0.png");
    mg.wallTextures[1] = LoadTexture("UI/bg_wall_1.png");
    mg.wallTextures[2] = LoadTexture("UI/bg_wall_2.png");

    mg.arrowLeft  = LoadTexture("UI/arrow_left.png");
    mg.arrowRight = LoadTexture("UI/arrow_right.png");

    mg.inventorySlot = LoadTexture("UI/slot.png");

    mg.currentWall = 0;

    // ---------- 定义物品 ----------
    mg.itemCount = 7;   // 画(0)、打字机(1)、画框(2)、门(3)、钥匙(4)、钉子(5)、锤子(6)

    // 【注意】调整了索引0和2的顺序，确保画框(id=2)先被绘制（垫底），画(id=0)后绘制在上面
    mg.items[0] = (Item){
        .id = 2, .name = "Frame", .texture = LoadTexture("UI/frame.png"),
        .wallIndex = 0, .interactRect = {0}, .isPickedUp = false, .visible = true
    };
    mg.items[1] = (Item){
        .id = 1, .name = "Typewriter", .texture = LoadTexture("UI/typewriter.png"),
        .wallIndex = 1, .interactRect = {0}, .isPickedUp = false, .visible = true
    };
    mg.items[2] = (Item){
        .id = 0, .name = "Picture", .texture = LoadTexture("UI/picture.png"),
        .wallIndex = 0, .interactRect = {0}, .isPickedUp = false, .visible = true
    };
    mg.items[3] = (Item){
        .id = 3, .name = "Door", .texture = LoadTexture("UI/door.png"),
        .wallIndex = 1, .interactRect = {0}, .isPickedUp = false, .visible = true
    };
    mg.items[4] = (Item){
        .id = 4, .name = "Key", .texture = LoadTexture("UI/key.png"),
        .wallIndex = 0, .interactRect = {0}, .isPickedUp = false, .visible = false
    };
    mg.items[5] = (Item){
        .id = 5, .name = "Nail", .texture = LoadTexture("UI/nail.png"),
        .wallIndex = 2, .interactRect = {0}, .isPickedUp = false, .visible = false
    };
    mg.items[6] = (Item){
        .id = 6, .name = "Hammer", .texture = LoadTexture("UI/hammer.png"),
        .wallIndex = 2, .interactRect = {0}, .isPickedUp = false, .visible = true
    };

    // 【核心修改】在这里调节物品的位置和大小
    // cx, cy: 0.5f 代表屏幕正中央，0.0f 代表最左/上，1.0f 代表最右/下
    // scale: 放大倍数。
    
    // Frame (id=2, Wall 0) - Center of the patterned wall
    mg.itemLayouts[0] = (ItemLayout){ .cx = 0.50f, .cy = 0.35f, .scale = 0.65f }; 
    // Typewriter (id=1, Wall 1) - On the right side of the plain wall
    mg.itemLayouts[1] = (ItemLayout){ .cx = 0.72f, .cy = 0.68f, .scale = 0.30f };
    // Picture (id=0, Wall 0) - Must match frame exactly
    mg.itemLayouts[2] = (ItemLayout){ .cx = 0.50f, .cy = 0.35f, .scale = 0.65f }; 
    // Door (id=3, Wall 1) - Standing against the left side of the plain wall
    mg.itemLayouts[3] = (ItemLayout){ .cx = 0.28f, .cy = 0.48f, .scale = 1.30f };
    // Key (id=4, Wall 0) - Drops on the desk below the picture
    mg.itemLayouts[4] = (ItemLayout){ .cx = 0.50f, .cy = 0.75f, .scale = 0.3f }; 
    // Nail (id=5, Wall 2) - Drops near the hammer on the floor
    mg.itemLayouts[5] = (ItemLayout){ .cx = 0.55f, .cy = 0.85f, .scale = 0.2f }; 
    // Hammer (id=6, Wall 2) - On the messy floor
    mg.itemLayouts[6] = (ItemLayout){ .cx = 0.65f, .cy = 0.85f, .scale = 0.4f };

    // 初始化计算边界
    UpdateItemRects();

    for (int i = 0; i < MAX_INVENTORY; i++) mg.inventory[i] = -1;
    mg.selectedSlot = -1;

    mg.dialogue.active = false;
    mg.dialogue.timer = 0.0f;

    mg.completed = false;
    mg.nextSceneId[0] = '\0';

    mg.mode = MG_MODE_NORMAL;
    mg.zoomItemId = -1;
    mg.typewriterInput[0] = '\0';

    mg.wordImages[0] = LoadTexture("UI/time.png");
    mg.wordImages[1] = LoadTexture("UI/walk.png");
    mg.wordImages[2] = LoadTexture("UI/bride.png");
    mg.typewriterStep = 0;  

    mg.pictureState = PICTURE_STATE_IN_FRAME;

    TraceLog(LOG_INFO, "Minigame initialized.");
}

/* ------------------ 根据屏幕动态计算精确边界 ------------------ */
static void UpdateItemRects(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    
    // 计算屏幕缩放基准，确保在不同分辨率下物品相对大小一致
    float screenScale = fminf((float)sw / REF_WIDTH, (float)sh / REF_HEIGHT);

    for (int i = 0; i < mg.itemCount; i++) {
        Item* it = &mg.items[i];
        ItemLayout layout = mg.itemLayouts[i];

        // 只有当纹理成功加载时才计算
        if (it->texture.id > 0) {
            float actualWidth = it->texture.width * layout.scale * screenScale;
            float actualHeight = it->texture.height * layout.scale * screenScale;
            
            // 根据中心点百分比，算出左上角 X, Y
            it->interactRect.x = (sw * layout.cx) - (actualWidth / 2.0f);
            it->interactRect.y = (sh * layout.cy) - (actualHeight / 2.0f);
            it->interactRect.width = actualWidth;
            it->interactRect.height = actualHeight;

            // Shrink door (id=3) collision rect to match the opaque center of the image
            if (it->id == 3) {
                it->interactRect.x += actualWidth * 0.25f;       // Cut 25% from left margin
                it->interactRect.width -= actualWidth * 0.50f;   // Shrink width by 50%
                it->interactRect.y += actualHeight * 0.20f;      // Cut 20% from top margin
                it->interactRect.height -= actualHeight * 0.20f; // Shrink height by 20%
            }
        }
    }
}

/* ------------------ 每帧更新（输入 + 逻辑） ------------------ */
void UpdateMinigame(void) {
    static int lastWidth = 0, lastHeight = 0;
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    if (sw != lastWidth || sh != lastHeight) {
        UpdateItemRects();
        lastWidth = sw;
        lastHeight = sh;
    }

    if (mg.mode == MG_MODE_ZOOM_PICTURE) {
        UpdateZoomPicture();
        return;
    }
    if (mg.mode == MG_MODE_ZOOM_TYPEWRITER) {
        UpdateZoomTypewriter();
        return;
    }

    Vector2 mouse = GetMousePosition();

    if (mg.dialogue.active) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            mg.dialogue.active = false;
        }
        return;
    }

    // 切换墙壁
    Rectangle leftArrowRect  = { 20, sh/2 - 50, 80, 100 };
    Rectangle rightArrowRect = { sw - 100, sh/2 - 50, 80, 100 };

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (CheckCollisionPointRec(mouse, leftArrowRect)) {
            mg.currentWall = (mg.currentWall - 1 + WALL_COUNT) % WALL_COUNT;
            return;
        }
        else if (CheckCollisionPointRec(mouse, rightArrowRect)) {
            mg.currentWall = (mg.currentWall + 1) % WALL_COUNT;
            return;
        }
    }

    // 物品交互（倒序遍历，保证点到最上层的物品）
    for (int i = mg.itemCount - 1; i >= 0; i--) {
        Item* it = &mg.items[i];
        
        bool visibleOnWall = (it->wallIndex == mg.currentWall && it->visible && !it->isPickedUp);
        if (it->id == 0 && mg.pictureState == PICTURE_STATE_PICKED_UP) {
            visibleOnWall = false;
        }

        if (visibleOnWall && CheckCollisionPointRec(mouse, it->interactRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (mg.selectedSlot != -1 && mg.inventory[mg.selectedSlot] != -1) {
                int toolId = mg.inventory[mg.selectedSlot];
                UseItemWithItem(toolId, it->id);
            } else {
                if (it->id == 0 || it->id == 2) {  
                    EnterZoomPicture();
                } else if (it->id == 1) {          
                    EnterZoomTypewriter();
                } else if (it->id == 3) {          
                    ShowDialogue("Narrator", "The door is locked, find a way out.");
                } else {
                    PickUpItem(i);
                }
            }
            break;   
        }
    }

    // 工具栏点击
    int slotSize = 80;
    int startX = 100;
    int slotY = sh - 120;
    for (int slot = 0; slot < MAX_INVENTORY; slot++) {
        Rectangle slotRect = { startX + slot * (slotSize + 10), slotY, slotSize, slotSize };
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, slotRect)) {
            if (mg.inventory[slot] != -1) {
                mg.selectedSlot = (mg.selectedSlot == slot) ? -1 : slot;
            } else {
                mg.selectedSlot = -1;
            }
        }
    }

    if (mg.completed) {
        ExitToStory(mg.nextSceneId);
        return;
    }

    // ESC disabled in minigame - player must complete the door puzzle to proceed to scene3
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

    DrawTexturePro(mg.wallTextures[mg.currentWall],
        (Rectangle){ 0, 0, (float)mg.wallTextures[mg.currentWall].width, (float)mg.wallTextures[mg.currentWall].height },
        (Rectangle){ 0, 0, (float)sw, (float)sh },
        (Vector2){ 0, 0 }, 0.0f, WHITE);

    Rectangle leftArrowRect  = { 20, sh/2 - 50, 80, 100 };
    Rectangle rightArrowRect = { sw - 100, sh/2 - 50, 80, 100 };
    
    DrawTexturePro(mg.arrowLeft, 
        (Rectangle){ 0, 0, (float)mg.arrowLeft.width, (float)mg.arrowLeft.height }, 
        leftArrowRect, (Vector2){0,0}, 0.0f, WHITE);
        
    DrawTexturePro(mg.arrowRight, 
        (Rectangle){ 0, 0, (float)mg.arrowRight.width, (float)mg.arrowRight.height }, 
        rightArrowRect, (Vector2){0,0}, 0.0f, WHITE);

    // 绘制当前墙上的物品
    // 【修改点】：直接将原始图片按比例渲染进 interactRect。
    for (int i = 0; i < mg.itemCount; i++) {
        Item* it = &mg.items[i];
        bool visibleOnWall = (it->wallIndex == mg.currentWall && it->visible && !it->isPickedUp);
        
        if (it->id == 0 && mg.pictureState == PICTURE_STATE_PICKED_UP) {
            visibleOnWall = false;
        }

        if (visibleOnWall && it->texture.id > 0) {
            Rectangle source = { 0, 0, (float)it->texture.width, (float)it->texture.height };
            DrawTexturePro(it->texture, source, it->interactRect, (Vector2){0, 0}, 0.0f, WHITE);
        }
    }

    // 绘制底部工具栏
    int slotSize = 80;
    int startX = 100;
    int slotY = sh - 120;
    for (int slot = 0; slot < MAX_INVENTORY; slot++) {
        Rectangle slotRect = { startX + slot * (slotSize + 10), slotY, slotSize, slotSize };

        if (mg.inventorySlot.id > 0) {
            DrawTexturePro(mg.inventorySlot,
                (Rectangle){ 0, 0, (float)mg.inventorySlot.width, (float)mg.inventorySlot.height },
                (Rectangle){ slotRect.x, slotRect.y, slotRect.width, slotRect.height },
                (Vector2){ 0, 0 }, 0.0f, WHITE);
        } else {
            DrawRectangleRec(slotRect, LIGHTGRAY);
            DrawRectangleLinesEx(slotRect, 2, DARKGRAY);
        }

        if (mg.inventory[slot] != -1) {
            int itemId = mg.inventory[slot];
            for (int i = 0; i < mg.itemCount; i++) {
                if (mg.items[i].id == itemId) {
                    Texture2D tex = mg.items[i].texture;
                    float scale = fminf((slotSize - 10) / (float)tex.width, (slotSize - 10) / (float)tex.height);
                    int drawW = (int)(tex.width * scale);
                    int drawH = (int)(tex.height * scale);
                    int drawX = slotRect.x + (slotSize - drawW) / 2;
                    int drawY = slotRect.y + (slotSize - drawH) / 2;
                    DrawTextureEx(tex, (Vector2){ drawX, drawY }, 0.0f, scale, WHITE);
                    break;
                }
            }
        }

        if (slot == mg.selectedSlot) {
            DrawRectangleLinesEx(slotRect, 4, YELLOW);
        }
    }

    if (mg.dialogue.active) {
        int boxY = sh - 300;
        int boxH = 200;
        DrawRectangle(0, boxY, sw, boxH, (Color){ 0, 0, 0, 200 });
        DrawText(mg.dialogue.speaker, 50, boxY + 20, 40, MAROON);
        DrawText(mg.dialogue.text, 50, boxY + 80, 36, WHITE);
        DrawText("Click to continue", sw - 300, boxY + 150, 30, GRAY);
    }
}

/* ------------------ 其他原有逻辑保持不变 ------------------ */

void UnloadMinigame(void) {
    for (int i = 0; i < WALL_COUNT; i++) {
        if (mg.wallTextures[i].id > 0) UnloadTexture(mg.wallTextures[i]);
    }
    UnloadTexture(mg.arrowLeft);
    UnloadTexture(mg.arrowRight);
    if (mg.inventorySlot.id > 0) UnloadTexture(mg.inventorySlot);
    
    for (int i = 0; i < mg.itemCount; i++) {
        if (mg.items[i].texture.id > 0) UnloadTexture(mg.items[i].texture);
    }
    for (int i = 0; i < 3; i++) {
        if (mg.wordImages[i].id > 0) UnloadTexture(mg.wordImages[i]);
    }
    memset(&mg, 0, sizeof(mg));
    TraceLog(LOG_INFO, "Minigame unloaded.");
}

static void ShowDialogue(const char* speaker, const char* text) {
    strncpy(mg.dialogue.speaker, speaker, sizeof(mg.dialogue.speaker) - 1);
    mg.dialogue.speaker[sizeof(mg.dialogue.speaker) - 1] = '\0';
    strncpy(mg.dialogue.text, text, sizeof(mg.dialogue.text) - 1);
    mg.dialogue.text[sizeof(mg.dialogue.text) - 1] = '\0';
    mg.dialogue.active = true;
}

static void PickUpItem(int itemIndex) {
    Item* it = &mg.items[itemIndex];
    for (int i = 0; i < MAX_INVENTORY; i++) {
        if (mg.inventory[i] == -1) {
            mg.inventory[i] = it->id;
            it->isPickedUp = true;
            it->visible = false;
            char msg[128];
            snprintf(msg, sizeof(msg), "Picked up %s.", it->name);
            ShowDialogue("You", msg);
            return;
        }
    }
    ShowDialogue("System", "Inventory is full.");
}

static void UseItemWithItem(int toolItemId, int targetItemId) {
    if (toolItemId == 4 && targetItemId == 3) {
        TraceLog(LOG_INFO, "Minigame: Opening door with key!");
        ShowDialogue("Narrator", "You unlocked the door with the key!");
        for (int i = 0; i < MAX_INVENTORY; i++) {
            if (mg.inventory[i] == 4) { mg.inventory[i] = -1; break; }
        }
        mg.selectedSlot = -1;
        strcpy(mg.nextSceneId, "scene3");
        mg.completed = true;
        TraceLog(LOG_INFO, "Minigame: completed=true, nextSceneId=scene3");
        return;
    }

    if (toolItemId == 5 && (targetItemId == 0 || targetItemId == 2) && mg.pictureState == PICTURE_STATE_IN_FRAME) {
        for (int i = 0; i < MAX_INVENTORY; i++) {
            if (mg.inventory[i] == 5) { mg.inventory[i] = -1; break; }
        }
        mg.selectedSlot = -1;
        mg.pictureState = PICTURE_STATE_NAILED;
        ShowDialogue("Narrator", "You nail the picture to the frame.");
        return;
    }

    if (toolItemId == 6 && (targetItemId == 0 || targetItemId == 2) && mg.pictureState == PICTURE_STATE_NAILED) {
        for (int i = 0; i < MAX_INVENTORY; i++) {
            if (mg.inventory[i] == 6) { mg.inventory[i] = -1; break; }
        }
        mg.selectedSlot = -1;
        mg.pictureState = PICTURE_STATE_COMPLETED;
        SpawnKey();
        ShowDialogue("Narrator", "You hammer the nail, and a key falls out");
        return;
    }

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

static void ExitToTitle(void) {
    UnloadMinigame();          
    game.state = STATE_TITLE;   
}

static void ExitToStory(const char* sceneId) {
    // Copy sceneId to local buffer before UnloadMinigame clears it
    char targetScene[32];
    strncpy(targetScene, sceneId, sizeof(targetScene) - 1);
    targetScene[sizeof(targetScene) - 1] = '\0';

    UnloadMinigame();
    
    TraceLog(LOG_INFO, "Minigame: Attempting to load scene: %s", targetScene);
    
    game.current_scene = GetSceneByID(targetScene);  
    if (game.current_scene) {
        TraceLog(LOG_INFO, "Minigame: Scene %s found! Entering story mode.", targetScene);
        game.dialogue_index = 0;
        game.state = STATE_PLAYING;    
        game.auto_timer = 0.0f;

        // Clear stale background/portrait to prevent 1-frame flash of old scene
        if (game.currentBackground.id != 0) {
            UnloadTexture(game.currentBackground);
            game.currentBackground = (Texture2D){0};
        }
        game.currentBackgroundPath[0] = '\0';
        if (game.currentPortrait.id != 0) {
            UnloadTexture(game.currentPortrait);
            game.currentPortrait = (Texture2D){0};
        }
        game.currentSpeaker[0] = '\0';
    } else {
        TraceLog(LOG_ERROR, "Minigame: Scene %s NOT FOUND! Returning to title.", targetScene);
        game.state = STATE_TITLE;
    }
}

static void SpawnNail(void) {
    for (int i=0; i < mg.itemCount; i++) {
        if (mg.items[i].id == 5) {
            mg.items[i].visible = true;
            mg.items[i].isPickedUp = false;
            break;
        }
    }
    ShowDialogue("Narrator", "A nail appears on the floor.");
}

static void SpawnKey(void) {
    for (int i=0; i < mg.itemCount; i++) {
        if (mg.items[i].id == 4) {
            mg.items[i].visible = true;
            mg.items[i].isPickedUp = false;
            break;
        }
    }
}

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

    float scale = 1.0f; 
    
    // 获取画框纹理（ID 为 2 的物品通常是 items[0] 依据初始化数组）
    Texture2D frameTex = mg.items[0].texture;
    int frameW = frameTex.width > 0 ? (int)(frameTex.width * scale) : 320;
    int frameH = frameTex.height > 0 ? (int)(frameTex.height * scale) : 320;
    
    Rectangle pictureZoomRect = { sw/2 - frameW/2, sh/2 - frameH/2, (float)frameW, (float)frameH };

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (CheckCollisionPointRec(mouse, pictureZoomRect)) {
            if (mg.pictureState == PICTURE_STATE_IN_FRAME) {
                mg.pictureState = PICTURE_STATE_PICKED_UP;
                for (int i = 0; i < MAX_INVENTORY; i++) {
                    if (mg.inventory[i] == -1) {
                        mg.inventory[i] = 0; 
                        break;
                    }
                }
                ShowDialogue("You", "You take the picture out of the frame.");
            } else if (mg.pictureState == PICTURE_STATE_PICKED_UP) {
                for (int i = 0; i < MAX_INVENTORY; i++) {
                    if (mg.inventory[i] == 0) {
                        mg.inventory[i] = -1;
                        break;
                    }
                }
                mg.pictureState = PICTURE_STATE_IN_FRAME;
                ShowDialogue("You", "You put the picture back into the frame.");
            } else if (mg.pictureState == PICTURE_STATE_NAILED || mg.pictureState == PICTURE_STATE_COMPLETED) {
                ShowDialogue("Narrator", "The picture is now fixed to the frame.");
            }
        } else {
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

    // 提取画框和画的纹理（依赖于 init 中的数组顺位：items[0]是画框，items[2]是画）
    Texture2D frameTex = mg.items[0].texture; 
    Texture2D picTex   = mg.items[2].texture; 

    float scale = 1.0f; // 放大倍数（需与 UpdateZoomPicture 保持一致）
    
    // 1. 计算画框的居中坐标并绘制
    int frameW = frameTex.width > 0 ? (int)(frameTex.width * scale) : 320;
    int frameH = frameTex.height > 0 ? (int)(frameTex.height * scale) : 320;
    int frameX = sw/2 - frameW/2;
    int frameY = sh/2 - frameH/2;

    if (frameTex.id > 0) {
        DrawTextureEx(frameTex, (Vector2){frameX, frameY}, 0.0f, scale, WHITE);
    } else {
        DrawRectangle(frameX-20, frameY-20, frameW+40, frameH+40, BROWN);
        DrawRectangle(frameX, frameY, frameW, frameH, LIGHTGRAY);
    }

    // 2. 根据画的状态，计算画的【独立居中坐标】并绘制
    if (mg.pictureState == PICTURE_STATE_IN_FRAME || mg.pictureState == PICTURE_STATE_NAILED || mg.pictureState == PICTURE_STATE_COMPLETED) {
        
        if (picTex.id > 0) {
            // 【关键修复】：单独计算画的宽高，让画永远在屏幕正中心（也就是画框正中心）
            int picW = (int)(picTex.width * scale);
            int picH = (int)(picTex.height * scale);
            int picX = sw/2 - picW/2;
            int picY = sh/2 - picH/2;
            
            DrawTextureEx(picTex, (Vector2){picX, picY}, 0.0f, scale, WHITE);
        }
        
        // 绘制提示文字（基于画框的位置）
        if (mg.pictureState == PICTURE_STATE_NAILED) {
            DrawText("(Nailed)", frameX+10, frameY+frameH-30, 20, RED);
        } else if (mg.pictureState == PICTURE_STATE_COMPLETED) {
            DrawText("(Completed)", frameX+10, frameY+frameH-30, 20, GREEN);
        }
    } 

    // 3. 底部操作提示文字
    if (mg.pictureState == PICTURE_STATE_IN_FRAME)
        DrawText("Click on the picture to take it", sw/2-200, sh-50, 20, WHITE);
    else if (mg.pictureState == PICTURE_STATE_PICKED_UP)
        DrawText("Click on the frame to put the picture back", sw/2-250, sh-50, 20, WHITE);
    else
        DrawText("Click elsewhere to exit", sw/2-120, sh-50, 20, WHITE);
    
    DrawText("Press ESC to exit", sw-200, sh-30, 20, WHITE);
}

static void UpdateZoomTypewriter(void) {
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

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        char inputLower[256];
        strcpy(inputLower, mg.typewriterInput);
        for (int i = 0; inputLower[i]; i++) inputLower[i] = tolower(inputLower[i]);

        const char* requiredWords[] = { "time", "walk", "bridge" };

        if (mg.typewriterStep < 3) {
            if (strcmp(inputLower, requiredWords[mg.typewriterStep]) == 0) {
                mg.typewriterStep++;
                mg.typewriterInput[0] = '\0';
                if (mg.typewriterStep == 3) {
                    mg.mode = MG_MODE_NORMAL;
                    SpawnNail();
                }
            } else {
                mg.typewriterInput[0] = '\0';
            }
        } else {
            mg.typewriterInput[0] = '\0';
        }
    }

    int sw = GetScreenWidth(), sh = GetScreenHeight();
    Vector2 mouse = GetMousePosition();
    Rectangle exitArrowRect = { sw - 80, 40, 60, 60 }; // Match new top-right position
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
    DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 200 });

    Texture2D tex = mg.items[1].texture; // 打字机纹理
    float scale = 1.0f;
    int drawW = (int)(tex.width * scale);
    int drawH = (int)(tex.height * scale);
    int drawX = sw/2 - drawW/2;
    int drawY = sh/2 - drawH/2 - 100; // Raise typewriter position
    
    if(tex.id > 0) {
        DrawTextureEx(tex, (Vector2){ drawX, drawY }, 0.0f, scale, WHITE);
    }

    // TextBox positioned below the typewriter
    Rectangle textBox = { drawX + 70, drawY + 440, drawW - 140, 40 };
    DrawRectangleRec(textBox, DARKGRAY);
    DrawRectangleLinesEx(textBox, 2, LIGHTGRAY);
    DrawText(mg.typewriterInput, (int)textBox.x + 10, (int)textBox.y + 10, 24, LIME);

    // Draw the collected word images (time, walk, bride) neat and centered
    float imgScale = 0.35f;
    int imgW = (int)(500 * imgScale);
    int gap = 40;
    int total_width = (3 * imgW) + (2 * gap);
    int startX = sw/2 - total_width/2;
    int imgY = sh - imgW - 10;
    for (int i = 0; i < 3; i++) {
        if (mg.wordImages[i].id > 0) {
            // Berikan efek transparan (redup) jika kata tersebut belum berhasil ditebak,
            // dan warna terang sepenuhnya jika sudah ditebak.
            Color tintColor = (i < mg.typewriterStep) ? WHITE : (Color){ 255, 255, 255, 100 };
            DrawTextureEx(mg.wordImages[i], (Vector2){ startX + i * (imgW + gap), imgY }, 0.0f, imgScale, tintColor);
        }
    }

    DrawExitArrow(sw - 80, 40, 40, WHITE);
    DrawText("Exit", sw - 75, 90, 16, WHITE);
}

static void DrawExitArrow(int x, int y, int size, Color color) {
    Vector2 p1 = { x, y };
    Vector2 p2 = { x + size, y + size/2 };
    Vector2 p3 = { x, y + size };
    DrawTriangle(p1, p2, p3, color);
    DrawRectangle(x + size/2 - 5, y + size/2 - 3, size/2, 6, color);
}
