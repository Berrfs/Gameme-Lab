#include "minigame4.h" 
#include "game.h"
#include "warehouse.h"
#include "raylib.h"
#include <string.h>

extern GameContext game;

// 定义平台的颜色类型
#define TYPE_RED 0
#define TYPE_BLACK 1

typedef struct {
    Rectangle rect;
    int type; // TYPE_RED 或 TYPE_BLACK
} Platform;

static struct {
    Vector2 playerPos;
    Vector2 playerVel;
    float playerSize;
    bool isGrounded;

    // 精灵图动画变量
    Texture2D playerTex;
    int currentFrame;
    float frameTimer;
    int facingDir;      // 面朝方向：2为左，3为右 (对应精灵图的第3、4行)
    bool isMoving;

    Platform platforms[20];
    int platformCount;

    Rectangle keyRect;
    bool keyCollected;
    Texture2D texKey; 

    bool isRedWorld; // 核心机制：当前世界状态

    // 延迟切换控制
    bool isSwitchPending;   // 是否正在等待切换世界
    float switchDelayTimer; // 倒计时器

    bool isGameOver;
    float endTimer;
} mg4 = {0};

/* 辅助：退出游戏 */
static void ExitToWarehouse() {
    UnloadMinigame4(); 
    game.state = STATE_WAREHOUSE;
    NotifyWarehouseMinigame4(true); // 通知仓库通关
}

void InitMinigame4(void) {
    memset(&mg4, 0, sizeof(mg4));
    
    // 加载精灵图与钥匙图片
    mg4.playerTex = LoadTexture("UI/player_sprite.png");
    mg4.texKey = LoadTexture("UI/key.png"); 
    
    mg4.facingDir = 3; 
    mg4.currentFrame = 1; 

    int sw = GetScreenWidth(), sh = GetScreenHeight();
    
    // 柱子的 X 坐标分布，中间留出更大的空隙
    int c[8] = {120, 250, 380, 510,  710, 840, 970, 1100};
    int w = 60;

    // --- 1. 关卡设计：8根等长柱子，顶部和底部对齐 ---
    mg4.platformCount = 0;
    
    int yTop = 250;     // 所有柱子的统一顶端 Y 坐标
    int yBottom = 650;  // 所有柱子的统一底端 Y 坐标 (悬空，不接触屏幕底部)
    
    // 4 种切断点高度 
    int splits[4] = { 310, 400, 470, 530 };

    // 利用对称循环生成 8 根柱子
    for (int i = 0; i < 4; i++) {
        int split = splits[i]; // 获取当前的切断点
        
        // 判断当前柱子的红黑分布规律
        // 根据截图：左1(i=0)上半截黑，下半截红；左2(i=1)上半截红，下半截黑...
        int typeTop = (i % 2 == 0) ? TYPE_BLACK : TYPE_RED;
        int typeBot = (i % 2 == 0) ? TYPE_RED : TYPE_BLACK;

        // 生成左半边的柱子 (Left 1 到 Left 4)
        mg4.platforms[mg4.platformCount++] = (Platform){ {c[i], yTop, w, split - yTop}, typeTop };
        mg4.platforms[mg4.platformCount++] = (Platform){ {c[i], split, w, yBottom - split}, typeBot };

        // 对称生成右半边的柱子 (Right 1 到 Right 4)
        mg4.platforms[mg4.platformCount++] = (Platform){ {c[7 - i], yTop, w, split - yTop}, typeTop };
        mg4.platforms[mg4.platformCount++] = (Platform){ {c[7 - i], split, w, yBottom - split}, typeBot };
    }

    // --- 2. 玩家与目标初始化 ---
    mg4.playerSize = 30.0f;
    mg4.isRedWorld = false; 
    mg4.isSwitchPending = false;    //初始不等待
    mg4.switchDelayTimer = 0.0f;    //计时器归零

    // 玩家出生在 左4柱子 (索引i=3)
    mg4.playerPos = (Vector2){ c[3] + 15, yTop - mg4.playerSize }; 

    // 钥匙，高度在最高切断点之上
    mg4.keyRect = (Rectangle){ c[4] + w/2.0f - 20, 85, 40, 40 };
}
    
void UpdateMinigame4(void) {
    float dt = GetFrameTime();
    
    if (mg4.isGameOver) {
        mg4.endTimer -= dt;
        if (mg4.endTimer <= 0) ExitToWarehouse();
        return;
    }

    // --- 1. 底色切换机制 ---
    // 按下空格键：立刻起跳，并启动“延迟变色”倒计时
    if (IsKeyPressed(KEY_SPACE) && mg4.isGrounded) {
        mg4.playerVel.y = -550.0f; // 立刻起跳
        mg4.isGrounded = false;
        
        mg4.isSwitchPending = true; // 开始等待变色
        mg4.switchDelayTimer = 0.15f; // 【参数设定】延迟 0.15 秒后变色 (可根据手感微调)
    }

    // 处理延迟变色倒计时
    if (mg4.isSwitchPending) {
        mg4.switchDelayTimer -= dt;
        if (mg4.switchDelayTimer <= 0) {
            mg4.isRedWorld = !mg4.isRedWorld; // 时间到，切换世界颜色！
            mg4.isSwitchPending = false;
        }
    }

    // --- 2. 玩家左右移动与动画控制 ---
    mg4.isMoving = false;
    if (IsKeyDown(KEY_A)) {
        mg4.playerVel.x = -300.0f;
        mg4.facingDir = 2; // 面朝左
        mg4.isMoving = true;
    } else if (IsKeyDown(KEY_D)) {
        mg4.playerVel.x = 300.0f;
        mg4.facingDir = 3; // 面朝右
        mg4.isMoving = true;
    } else {
        mg4.playerVel.x = 0; // 不按键就停下
    }

    // 动画更新逻辑
    if (!mg4.isGrounded) {
        // 如果在空中（跳跃/下落），固定显示第1列的跨步姿势
        mg4.currentFrame = 0; 
    } else if (mg4.isMoving) {
        // 如果在地面且移动，播放走路循环动画
        mg4.frameTimer += dt;
        if (mg4.frameTimer > 0.15f) {
            mg4.currentFrame = (mg4.currentFrame + 1) % 4; // 4帧循环
            mg4.frameTimer = 0.0f;
        }
    } else {
        // 如果在地面且没移动，固定显示第2列的站立姿势
        mg4.currentFrame = 1; 
    }

    // --- 3. 物理系统与碰撞检测 (平台跳跃核心) ---
    mg4.playerVel.y += 1500.0f * dt; // 施加重力

    // 【X轴先移动并检测】
    mg4.playerPos.x += mg4.playerVel.x * dt;
    Rectangle playerBoxX = { mg4.playerPos.x, mg4.playerPos.y, mg4.playerSize, mg4.playerSize };
    
    for (int i = 0; i < mg4.platformCount; i++) {
        // 【已修复逻辑】：红背景下，黑色是实体；黑背景下，红色是实体。
        if (mg4.isRedWorld && mg4.platforms[i].type != TYPE_BLACK) continue;
        if (!mg4.isRedWorld && mg4.platforms[i].type != TYPE_RED) continue;

        if (CheckCollisionRecs(playerBoxX, mg4.platforms[i].rect)) {
            if (mg4.playerVel.x > 0) mg4.playerPos.x = mg4.platforms[i].rect.x - mg4.playerSize;
            else if (mg4.playerVel.x < 0) mg4.playerPos.x = mg4.platforms[i].rect.x + mg4.platforms[i].rect.width;
        }
    }

    // 【Y轴再移动并检测】
    mg4.playerPos.y += mg4.playerVel.y * dt;
    Rectangle playerBoxY = { mg4.playerPos.x, mg4.playerPos.y, mg4.playerSize, mg4.playerSize };
    mg4.isGrounded = false; 

    for (int i = 0; i < mg4.platformCount; i++) {
        // 【已修复逻辑】：红背景下，黑色是实体；黑背景下，红色是实体。
        if (mg4.isRedWorld && mg4.platforms[i].type != TYPE_BLACK) continue;
        if (!mg4.isRedWorld && mg4.platforms[i].type != TYPE_RED) continue;

        if (CheckCollisionRecs(playerBoxY, mg4.platforms[i].rect)) {
            if (mg4.playerVel.y > 0) { 
                mg4.playerPos.y = mg4.platforms[i].rect.y - mg4.playerSize; 
                mg4.playerVel.y = 0;
                mg4.isGrounded = true;
            } else if (mg4.playerVel.y < 0) { 
                mg4.playerPos.y = mg4.platforms[i].rect.y + mg4.platforms[i].rect.height; 
                mg4.playerVel.y = 0;
            }
        }
    }

    // 掉出屏幕外死亡重启
    if (mg4.playerPos.y > GetScreenHeight() + 100) {
        InitMinigame4(); // 重新加载关卡
    }

    // --- 4. 拾取钥匙检测 ---
    Rectangle pBox = { mg4.playerPos.x, mg4.playerPos.y, mg4.playerSize, mg4.playerSize };
    if (!mg4.keyCollected && CheckCollisionRecs(pBox, mg4.keyRect)) {
        mg4.keyCollected = true;
        mg4.isGameOver = true;
        mg4.endTimer = 2.0f;
    }
}

void DrawMinigame4(void) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();

    // 提取的官方配色
    Color bgRed = (Color){ 228, 86, 60, 255 };     
    Color bgBlack = (Color){ 65, 65, 65, 255 };    
    Color lineRed = (Color){ 180, 50, 40, 255 };   
    Color lineBlack = (Color){ 40, 40, 40, 255 };  

    // 1. 绘制背景
    ClearBackground(mg4.isRedWorld ? bgRed : bgBlack);

    // 2. 绘制 8 根柱子 (实体画纯色，虚影画线框)
    for (int i = 0; i < mg4.platformCount; i++) {
        Platform p = mg4.platforms[i];
        
        if (mg4.isRedWorld) {
            // 红底：黑色是实体，红色用线框
            if (p.type == TYPE_BLACK) DrawRectangleRec(p.rect, bgBlack); 
            else DrawRectangleLinesEx(p.rect, 2, lineRed);
        } else {
            // 黑底：红色是实体，黑色用线框
            if (p.type == TYPE_RED) DrawRectangleRec(p.rect, bgRed);
            else DrawRectangleLinesEx(p.rect, 2, lineBlack); 
        }
    }

    // 3. 绘制钥匙
    if (!mg4.keyCollected) {
        if (mg4.texKey.id > 0) {
            // 加载成功则渲染钥匙图片
            DrawTexturePro(mg4.texKey, 
                (Rectangle){0, 0, mg4.texKey.width, mg4.texKey.height}, 
                mg4.keyRect, (Vector2){0,0}, 0.0f, WHITE);
        } else {
            // 没有图片则画个太阳占位
            DrawPoly((Vector2){mg4.keyRect.x + 20, mg4.keyRect.y + 20}, 8, 20, 0, WHITE);
            DrawPoly((Vector2){mg4.keyRect.x + 20, mg4.keyRect.y + 20}, 8, 15, 0, mg4.isRedWorld ? bgRed : bgBlack);
            DrawPoly((Vector2){mg4.keyRect.x + 20, mg4.keyRect.y + 20}, 8, 10, 0, WHITE);
        }
    }

    // 4. 绘制玩家
    if (mg4.playerTex.id > 0) {
        float frameWidth = (float)mg4.playerTex.width / 4;
        float frameHeight = (float)mg4.playerTex.height / 4;
        Rectangle sourceRec = { mg4.currentFrame * frameWidth, mg4.facingDir * frameHeight, frameWidth, frameHeight };
        
        float drawSize = 60.0f; 
        float drawX = mg4.playerPos.x + (mg4.playerSize / 2.0f) - (drawSize / 2.0f);
        float drawY = mg4.playerPos.y + mg4.playerSize - drawSize;
        Rectangle destRec = { drawX, drawY, drawSize, drawSize };

        DrawTexturePro(mg4.playerTex, sourceRec, destRec, (Vector2){0,0}, 0.0f, WHITE);
    } else {
        DrawRectangle(mg4.playerPos.x, mg4.playerPos.y, mg4.playerSize, mg4.playerSize, WHITE);
    }

    // UI 提示
    DrawText("Press [SPACE] to Jump and Switch World!", 20, 20, 24, WHITE);

    if (mg4.isGameOver) {
        DrawRectangle(0, 0, sw, sh, (Color){0,0,0,150});
        DrawText("KEY FOUND!", sw/2 - 120, sh/2, 40, YELLOW);
    }
}

void UnloadMinigame4(void) {
    if (mg4.playerTex.id > 0) UnloadTexture(mg4.playerTex);
    if (mg4.texKey.id > 0) UnloadTexture(mg4.texKey); 
    memset(&mg4, 0, sizeof(mg4));
}