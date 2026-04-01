#include "minigame3.h"
#include "game.h"
#include "warehouse.h"
#include "raylib.h"
#include <math.h>
#include <string.h>

extern GameContext game;

// ==========================================
// 游戏难度参数调整区
// ==========================================
#define PLAYER_SPEED 350.0f         // 玩家移动速度
#define PLAYER_FIRE_RATE 0.15f      // 玩家开火间隔 (按住空格键时的连发速度)
#define ENEMY_SPAWN_TIME_MIN 0.8f   // 敌机生成最小间隔(秒)
#define ENEMY_SPAWN_TIME_MAX 1.5f   // 敌机生成最大间隔(秒)
#define OBSTACLE_SPAWN_TIME 3.0f    // 障碍物(钢琴/木板)生成间隔
#define MAX_PROGRESS 20             // 通关所需的碎片数量 (20个 = 每次拾取+5%)
// ==========================================

#define MAX_ENEMIES 20
#define MAX_BULLETS_ENEMY 100
#define MAX_BULLETS_PLAYER 50
#define MAX_DROPS 30
#define MAX_OBSTACLES 5

typedef struct { Vector2 pos; float radius; int hp; float speed; } PlayerPlane;
typedef struct { bool active; Vector2 pos; float radius; int type; int hp; float speed; float shootTimer; } EnemyPlane;
typedef struct { bool active; Vector2 pos; Vector2 velocity; float radius; } Bullet;
typedef struct { bool active; Vector2 pos; float radius; } DropItem;
typedef struct { bool active; Rectangle rect; float speed; int type; } Obstacle;

static struct {
    PlayerPlane player;
    EnemyPlane enemies[MAX_ENEMIES];
    Bullet enemyBullets[MAX_BULLETS_ENEMY];
    Bullet playerBullets[MAX_BULLETS_PLAYER];
    DropItem drops[MAX_DROPS];
    Obstacle obstacles[MAX_OBSTACLES];

    //图片资源
    Texture2D texPlayer;
    Texture2D texEnemy1;
    Texture2D texEnemy2;
    Texture2D texBulletPlayer;
    Texture2D texBulletEnemy;
    Texture2D texDrop;
    Texture2D texWood;
    Texture2D texPiano;
    Texture2D texHeart;

    bool showTutorial;      // 显示开局教程

    float playerShootTimer;
    float enemySpawnTimer;
    float obstacleSpawnTimer;
    
    int currentProgress;    // 当前收集的碎片数    
    bool exitAppears;       // 收集满后出口出现       
    Rectangle exitRect;     

    // --- Meta(元游戏)干扰系统变量 ---
    bool metaEvent1;        // 50%进度触发：反转按键
    float reverseTimer;     // 反转按键剩余时间
    bool metaEvent2;        // 80%进度触发：黑洞引力
    float metaTextTimer;    // 屏幕中央大字显示时间
    char metaText[64];      // 屏幕大字内容      

    bool isGameOver;
    bool isWin;
    float endTimer;
} mg3 = {0};

/* 辅助：发射敌方子弹 */
static void SpawnEnemyBullet(Vector2 pos, Vector2 velocity) {
    for (int i = 0; i < MAX_BULLETS_ENEMY; i++) {
        if (!mg3.enemyBullets[i].active) {
            mg3.enemyBullets[i].active = true;
            mg3.enemyBullets[i].pos = pos;
            mg3.enemyBullets[i].velocity = velocity;
            mg3.enemyBullets[i].radius = 5.0f;
            break;
        }
    }
}

/* 辅助：发射玩家子弹 */
static void SpawnPlayerBullet(Vector2 pos) {
    for (int i = 0; i < MAX_BULLETS_PLAYER; i++) {
        if (!mg3.playerBullets[i].active) {
            mg3.playerBullets[i].active = true;
            mg3.playerBullets[i].pos = pos;
            mg3.playerBullets[i].velocity = (Vector2){0, -600.0f}; // 向上飞
            mg3.playerBullets[i].radius = 4.0f;
            break;
        }
    }
}

/* 辅助：生成掉落碎片 */
static void SpawnDrop(Vector2 pos) {
    for (int i = 0; i < MAX_DROPS; i++) {
        if (!mg3.drops[i].active) {
            mg3.drops[i].active = true;
            mg3.drops[i].pos = pos;
            mg3.drops[i].radius = 8.0f;
            break;
        }
    }
}

static void ExitToWarehouse() {
    bool winStatus = mg3.isWin;
    UnloadMinigame3();
    game.state = STATE_WAREHOUSE;
    NotifyWarehouseMinigame3(winStatus);
}

void InitMinigame3(void) {
    memset(&mg3, 0, sizeof(mg3));
    //加载图片 (如果图片不存在，游戏会自动降级显示彩色形状)
    mg3.texPlayer       = LoadTexture("UI/player_plane.png");
    mg3.texEnemy1       = LoadTexture("UI/enemy_plane1.png");
    mg3.texEnemy2       = LoadTexture("UI/enemy_plane2.png");
    mg3.texBulletPlayer = LoadTexture("UI/bullet_player.png");
    mg3.texBulletEnemy  = LoadTexture("UI/bullet_enemy.png");
    mg3.texDrop         = LoadTexture("UI/data_drop.png");
    mg3.texWood         = LoadTexture("UI/wood.png");
    mg3.texPiano        = LoadTexture("UI/piano.png");
    mg3.texHeart        = LoadTexture("UI/heart.png");

    int sw = GetScreenWidth(), sh = GetScreenHeight();
    
    mg3.showTutorial = true; // 开局默认显示教程
    
    mg3.player.pos = (Vector2){ sw / 2.0f, sh - 100.0f };
    mg3.player.hp = 3;
    mg3.player.radius = 15.0f;
    mg3.exitRect = (Rectangle){ 0, 0, (float)sw, 60.0f };
    mg3.enemySpawnTimer = 1.0f;
    mg3.obstacleSpawnTimer = 3.0f;
}

void UpdateMinigame3(void) {
    float dt = GetFrameTime();
    int sw = GetScreenWidth(), sh = GetScreenHeight();

    // --- 开局教程拦截 ---
    if (mg3.showTutorial) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_J)) {
            mg3.showTutorial = false; // 按键关闭教程，游戏正式开始
        }
        return; // 拦截所有更新逻辑，暂停游戏
    }

    if (mg3.isGameOver) {
        mg3.endTimer -= dt;
        if (mg3.endTimer <= 0) ExitToWarehouse();
        return;
    }

    // --- Meta(元游戏)干扰逻辑触发 ---
    if (mg3.currentProgress >= MAX_PROGRESS * 0.5f && !mg3.metaEvent1) {
        mg3.metaEvent1 = true;
        mg3.reverseTimer = 5.0f; // 按键反转5秒
        mg3.metaTextTimer = 3.0f;
        strcpy(mg3.metaText, "GAME: YOU ARE NOT SUPPOSED TO WIN THIS!");
    }
    if (mg3.currentProgress >= MAX_PROGRESS * 0.8f && !mg3.metaEvent2) {
        mg3.metaEvent2 = true;
        mg3.metaTextTimer = 3.0f;
        strcpy(mg3.metaText, "GAME: STOP RIGHT NOW! Erasing player...");
    }

    if (mg3.metaTextTimer > 0) mg3.metaTextTimer -= dt;
    if (mg3.reverseTimer > 0) mg3.reverseTimer -= dt;

    // --- 1. 玩家移动与反转控制 ---
    int dir = (mg3.reverseTimer > 0) ? -1 : 1; // 如果触发Meta1，dir变成-1，控制反转
    
    if (IsKeyDown(KEY_W)) mg3.player.pos.y -= PLAYER_SPEED * dt * dir;
    if (IsKeyDown(KEY_S)) mg3.player.pos.y += PLAYER_SPEED * dt * dir;
    if (IsKeyDown(KEY_A)) mg3.player.pos.x -= PLAYER_SPEED * dt * dir;
    if (IsKeyDown(KEY_D)) mg3.player.pos.x += PLAYER_SPEED * dt * dir;

    // Meta2: 黑洞引力 (向屏幕上方中心拉扯)
    if (mg3.metaEvent2) {
        float pullSpeed = 80.0f * dt;
        if (mg3.player.pos.x < sw/2.0f) mg3.player.pos.x += pullSpeed;
        else mg3.player.pos.x -= pullSpeed;
        mg3.player.pos.y -= pullSpeed;
    }

    // 屏幕限制
    if (mg3.player.pos.x < mg3.player.radius) mg3.player.pos.x = mg3.player.radius;
    if (mg3.player.pos.x > sw - mg3.player.radius) mg3.player.pos.x = sw - mg3.player.radius;
    if (mg3.player.pos.y < mg3.player.radius) mg3.player.pos.y = mg3.player.radius;
    if (mg3.player.pos.y > sh - mg3.player.radius) mg3.player.pos.y = sh - mg3.player.radius;

    // --- 2. 玩家手动射击 ---
    mg3.playerShootTimer -= dt;
    // 按住 空格键 或 J键 持续开火
    if ((IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_J)) && mg3.playerShootTimer <= 0) {
        SpawnPlayerBullet(mg3.player.pos);
        mg3.playerShootTimer = PLAYER_FIRE_RATE;
    }

    // --- 3. 进度与出口 ---
    if (mg3.currentProgress >= MAX_PROGRESS && !mg3.exitAppears) {
        mg3.exitAppears = true;
    }
    if (mg3.exitAppears && CheckCollisionCircleRec(mg3.player.pos, mg3.player.radius, mg3.exitRect)) {
        mg3.isGameOver = true; mg3.isWin = true; mg3.endTimer = 2.0f; return;
    }

    // --- 4. 障碍物(木板/钢琴)生成与更新 ---
    mg3.obstacleSpawnTimer -= dt;
    if (mg3.obstacleSpawnTimer <= 0) {
        mg3.obstacleSpawnTimer = OBSTACLE_SPAWN_TIME;
        for (int i = 0; i < MAX_OBSTACLES; i++) {
            if (!mg3.obstacles[i].active) {
                mg3.obstacles[i].active = true;
                mg3.obstacles[i].type = GetRandomValue(0, 1);
                if (mg3.obstacles[i].type == 0) {
                    mg3.obstacles[i].rect = (Rectangle){ GetRandomValue(-50, sw-150), -100, 200, 40 };
                    mg3.obstacles[i].speed = 100.0f;    // 木板：宽，速度慢
                } else {
                    mg3.obstacles[i].rect = (Rectangle){ GetRandomValue(50, sw-100), -100, 80, 80 };
                    mg3.obstacles[i].speed = 250.0f;   // 钢琴：正方形，速度快 
                }
                break;
            }
        }
    }
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (mg3.obstacles[i].active) {
            mg3.obstacles[i].rect.y += mg3.obstacles[i].speed * dt;
            if (mg3.obstacles[i].rect.y > sh) mg3.obstacles[i].active = false; 
            
            if (CheckCollisionCircleRec(mg3.player.pos, mg3.player.radius, mg3.obstacles[i].rect)) {
                mg3.obstacles[i].active = false;
                mg3.player.hp--;
                if (mg3.player.hp <= 0) { mg3.isGameOver = true; mg3.isWin = false; mg3.endTimer = 2.0f; }
            }
        }
    }

    // --- 5. 敌机生成与更新 ---
    mg3.enemySpawnTimer -= dt;
    if (mg3.enemySpawnTimer <= 0 && !mg3.exitAppears) {
        mg3.enemySpawnTimer = GetRandomValue(ENEMY_SPAWN_TIME_MIN * 10, ENEMY_SPAWN_TIME_MAX * 10) / 10.0f;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!mg3.enemies[i].active) {
                mg3.enemies[i].active = true;
                mg3.enemies[i].pos = (Vector2){ (float)GetRandomValue(50, sw - 50), -30.0f };
                mg3.enemies[i].radius = 20.0f;
                mg3.enemies[i].type = GetRandomValue(1, 10) > 4 ? 1 : 2;
                mg3.enemies[i].hp = (mg3.enemies[i].type == 1) ? 2 : 4; // 敌机血量
                mg3.enemies[i].speed = GetRandomValue(100, 150);
                mg3.enemies[i].shootTimer = 1.0f;
                break;
            }
        }
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (mg3.enemies[i].active) {
            mg3.enemies[i].pos.y += mg3.enemies[i].speed * dt;
            if (mg3.enemies[i].pos.y > sh + 50) { mg3.enemies[i].active = false; continue; }
            // 射击
            mg3.enemies[i].shootTimer -= dt;
            if (mg3.enemies[i].shootTimer <= 0) {
                float bSpeed = 300.0f;
                if (mg3.enemies[i].type == 1) {
                    SpawnEnemyBullet(mg3.enemies[i].pos, (Vector2){0, bSpeed});
                    mg3.enemies[i].shootTimer = 1.2f;
                } else {
                    float angle60 = 60.0f * PI / 180.0f; 
                    SpawnEnemyBullet(mg3.enemies[i].pos, (Vector2){0, bSpeed});
                    SpawnEnemyBullet(mg3.enemies[i].pos, (Vector2){ -sinf(angle60) * bSpeed, cosf(angle60) * bSpeed });
                    SpawnEnemyBullet(mg3.enemies[i].pos, (Vector2){ sinf(angle60) * bSpeed, cosf(angle60) * bSpeed });
                    mg3.enemies[i].shootTimer = 2.0f;
                }
            }
        }
    }

    // --- 6. 玩家子弹打击敌机 ---
    for (int i = 0; i < MAX_BULLETS_PLAYER; i++) {
        if (mg3.playerBullets[i].active) {
            mg3.playerBullets[i].pos.y += mg3.playerBullets[i].velocity.y * dt;
            if (mg3.playerBullets[i].pos.y < -10) { mg3.playerBullets[i].active = false; continue; }

            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (mg3.enemies[j].active) {
                    float dx = mg3.playerBullets[i].pos.x - mg3.enemies[j].pos.x;
                    float dy = mg3.playerBullets[i].pos.y - mg3.enemies[j].pos.y;
                    if (dx*dx + dy*dy < (mg3.enemies[j].radius + mg3.playerBullets[i].radius)*(mg3.enemies[j].radius + mg3.playerBullets[i].radius)) {
                        mg3.playerBullets[i].active = false; // 子弹消失
                        mg3.enemies[j].hp--;
                        if (mg3.enemies[j].hp <= 0) {
                            mg3.enemies[j].active = false; // 敌机爆炸
                            SpawnDrop(mg3.enemies[j].pos); // 生成数据碎片
                        }
                        break;
                    }
                }
            }
        }
    }

    // --- 7. 敌机子弹打中玩家 ---
    for (int i = 0; i < MAX_BULLETS_ENEMY; i++) {
        if (mg3.enemyBullets[i].active) {
            mg3.enemyBullets[i].pos.x += mg3.enemyBullets[i].velocity.x * dt;
            mg3.enemyBullets[i].pos.y += mg3.enemyBullets[i].velocity.y * dt;
            if (mg3.enemyBullets[i].pos.y > sh + 50 || mg3.enemyBullets[i].pos.x < -50 || mg3.enemyBullets[i].pos.x > sw + 50) {
                mg3.enemyBullets[i].active = false; continue;
            }

            float dx = mg3.enemyBullets[i].pos.x - mg3.player.pos.x;
            float dy = mg3.enemyBullets[i].pos.y - mg3.player.pos.y;
            if (dx*dx + dy*dy < (mg3.player.radius + mg3.enemyBullets[i].radius)*(mg3.player.radius + mg3.enemyBullets[i].radius)) {
                mg3.enemyBullets[i].active = false;
                mg3.player.hp--;
                if (mg3.player.hp <= 0) { mg3.isGameOver = true; mg3.isWin = false; mg3.endTimer = 2.0f; }
            }
        }
    }

    // --- 8. 玩家拾取掉落碎片 ---
    for (int i = 0; i < MAX_DROPS; i++) {
        if (mg3.drops[i].active) {
            mg3.drops[i].pos.y += 80.0f * dt; // 碎片缓慢下落
            if (mg3.drops[i].pos.y > sh) { mg3.drops[i].active = false; continue; }

            float dx = mg3.drops[i].pos.x - mg3.player.pos.x;
            float dy = mg3.drops[i].pos.y - mg3.player.pos.y;
            if (dx*dx + dy*dy < (mg3.player.radius + mg3.drops[i].radius)*(mg3.player.radius + mg3.drops[i].radius)) {
                mg3.drops[i].active = false;
                if (mg3.currentProgress < MAX_PROGRESS) {
                    mg3.currentProgress++; // 增加进度
                }
            }
        }
    }
}

void DrawMinigame3(void) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    // 屏幕震动效果 (如果触发黑洞)
    float offsetX = 0, offsetY = 0;
    if (mg3.metaEvent2 && !mg3.exitAppears) {
        offsetX = GetRandomValue(-2, 2);
        offsetY = GetRandomValue(-2, 2);
    }
    
    DrawRectangle(0, 0, sw, sh, (Color){ 10, 15, 30, 255 }); // 背景

    if (mg3.exitAppears) {
        DrawRectangleRec(mg3.exitRect, (Color){ 50, 200, 50, 100 });
        DrawRectangleLinesEx(mg3.exitRect, 3, GREEN);
        DrawText("EXIT", sw/2 - 40, 15, 30, GREEN);
    }

    // 1. 画障碍物 (木板/钢琴)
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (mg3.obstacles[i].active) {
            Rectangle r = mg3.obstacles[i].rect;
            r.x += offsetX; r.y += offsetY;
            
            if (mg3.obstacles[i].type == 0) { // 木板
                if (mg3.texWood.id > 0) {
                    // 使用 DrawTexturePro 强制让图片拉伸填满矩形判定框
                    DrawTexturePro(mg3.texWood, (Rectangle){0,0,mg3.texWood.width,mg3.texWood.height}, r, (Vector2){0,0}, 0.0f, WHITE);
                } else {
                    DrawRectangleRec(r, DARKBROWN); 
                    DrawText("WOOD", r.x + 10, r.y + 10, 20, BLACK);
                }
            } else { // 钢琴
                if (mg3.texPiano.id > 0) {
                    DrawTexturePro(mg3.texPiano, (Rectangle){0,0,mg3.texPiano.width,mg3.texPiano.height}, r, (Vector2){0,0}, 0.0f, WHITE);
                } else {
                    DrawRectangleRec(r, BLACK); 
                    DrawRectangleLinesEx(r, 2, WHITE);
                    DrawText("PIANO", r.x + 10, r.y + 30, 20, WHITE);
                }
            }
        }
    }

    // 2. 画掉落碎片
    for (int i = 0; i < MAX_DROPS; i++) {
        if (mg3.drops[i].active) {
            Vector2 p = { mg3.drops[i].pos.x + offsetX, mg3.drops[i].pos.y + offsetY };
            if (mg3.texDrop.id > 0) {
                DrawTextureV(mg3.texDrop, (Vector2){ p.x - mg3.texDrop.width/2.0f, p.y - mg3.texDrop.height/2.0f }, WHITE);
            } else {
                DrawPoly(p, 4, mg3.drops[i].radius, 0, LIME);
            }
        }
    }

    // 3. 画玩家子弹
    for (int i = 0; i < MAX_BULLETS_PLAYER; i++) {
        if (mg3.playerBullets[i].active) {
            Vector2 p = { mg3.playerBullets[i].pos.x + offsetX, mg3.playerBullets[i].pos.y + offsetY };
            if (mg3.texBulletPlayer.id > 0) {
                DrawTextureV(mg3.texBulletPlayer, (Vector2){ p.x - mg3.texBulletPlayer.width/2.0f, p.y - mg3.texBulletPlayer.height/2.0f }, WHITE);
            } else {
                DrawCircleV(p, mg3.playerBullets[i].radius, BLUE);
            }
        }
    }

    // 4. 画敌方子弹
    for (int i = 0; i < MAX_BULLETS_ENEMY; i++) {
        if (mg3.enemyBullets[i].active) {
            Vector2 p = { mg3.enemyBullets[i].pos.x + offsetX, mg3.enemyBullets[i].pos.y + offsetY };
            if (mg3.texBulletEnemy.id > 0) {
                DrawTextureV(mg3.texBulletEnemy, (Vector2){ p.x - mg3.texBulletEnemy.width/2.0f, p.y - mg3.texBulletEnemy.height/2.0f }, WHITE);
            } else {
                DrawCircleV(p, mg3.enemyBullets[i].radius, YELLOW);
                DrawCircleV(p, mg3.enemyBullets[i].radius * 0.5f, WHITE);
            }
        }
    }

    // 5. 画敌机
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (mg3.enemies[i].active) {
            Vector2 pos = { mg3.enemies[i].pos.x + offsetX, mg3.enemies[i].pos.y + offsetY };
            Texture2D eTex = (mg3.enemies[i].type == 1) ? mg3.texEnemy1 : mg3.texEnemy2;
            
            if (eTex.id > 0) {
                DrawTextureV(eTex, (Vector2){ pos.x - eTex.width/2.0f, pos.y - eTex.height/2.0f }, WHITE);
            } else {
                Color eColor = (mg3.enemies[i].type == 1) ? RED : PURPLE;
                float r = mg3.enemies[i].radius;
                DrawTriangle(
                    (Vector2){pos.x, pos.y + r},          
                    (Vector2){pos.x + r, pos.y - r},      
                    (Vector2){pos.x - r, pos.y - r},      
                    eColor
                );
            }
        }
    }

    // 6. 画玩家飞机
    if (mg3.player.hp > 0) {
        Vector2 pos = { mg3.player.pos.x + offsetX, mg3.player.pos.y + offsetY };
        if (mg3.texPlayer.id > 0) {
            DrawTextureV(mg3.texPlayer, (Vector2){ pos.x - mg3.texPlayer.width/2.0f, pos.y - mg3.texPlayer.height/2.0f }, WHITE);
        } else {
            float r = mg3.player.radius;
            DrawTriangle((Vector2){pos.x, pos.y - r * 1.5f}, (Vector2){pos.x - r, pos.y + r}, (Vector2){pos.x + r, pos.y + r}, SKYBLUE);
        }
    }
    

    // --- Meta: 屏幕中央大字与系统警告文字 ---
    if (mg3.metaTextTimer > 0) {
        DrawRectangle(0, sh/2 - 60, sw, 120, (Color){ 0, 0, 0, 200 });
        int tw = MeasureText(mg3.metaText, 30);
        DrawText(mg3.metaText, sw/2 - tw/2 + offsetX, sh/2 - 30 + offsetY, 30, RED);
        
        // 追加系统层级的黄色/紫色警告暗示文字
        if (mg3.reverseTimer > 0) {
            DrawText("[SYSTEM WARNING: CONTROLS REVERSED!]", sw/2 - 210 + offsetX, sh/2 + 15 + offsetY, 20, YELLOW);
        } else if (mg3.metaEvent2) {
            DrawText("[SYSTEM WARNING: GRAVITY ANOMALY DETECTED!]", sw/2 - 250 + offsetX, sh/2 + 15 + offsetY, 20, PURPLE);
        }
    }
    //UI
    DrawText("HP: ", 20, sh - 40, 24, WHITE);
    for (int i = 0; i < mg3.player.hp; i++) {
        if (mg3.texHeart.id > 0) {
            DrawTextureV(mg3.texHeart, (Vector2){ 80 + i * 35, sh - 45 }, WHITE);
        } else {
            DrawCircle(80 + i * 30, sh - 28, 10, RED);
        }
    }

    int barWidth = 300;
    DrawText("DATA COLLECTED", sw - barWidth - 20, sh - 50, 20, GRAY);
    DrawRectangle(sw - barWidth - 20, sh - 25, barWidth, 15, DARKGRAY);
    float progressRatio = (float)mg3.currentProgress / MAX_PROGRESS;
    DrawRectangle(sw - barWidth - 20, sh - 25, (int)(barWidth * progressRatio), 15, LIME);

    // --- 开局教程界面 ---
    if (mg3.showTutorial) {
        DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 220 }); // 半透明黑底
        
        DrawText("COMBAT PROTOCOL INITIATED", sw/2 - 240, sh/2 - 120, 32, SKYBLUE);
        
        DrawText("- Press [W][A][S][D] to Move", sw/2 - 180, sh/2 - 40, 24, WHITE);
        DrawText("- Hold [SPACE] or [J] to Fire", sw/2 - 180, sh/2, 24, WHITE);
        
        DrawText("- Destroy enemies & collect GREEN DATA to progress", sw/2 - 300, sh/2 + 60, 24, LIME);
        DrawText("- Avoid bullets and warehouse objects (Wood/Piano)", sw/2 - 280, sh/2 + 100, 24, RED);
        
        // 闪烁的按键提示
        if ((int)(GetTime() * 2) % 2 == 0) {
            DrawText("Press [ENTER] or [SPACE] to Start", sw/2 - 220, sh/2 + 180, 26, YELLOW);
        }
    }

    if (mg3.isGameOver) {
        DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 150 });
        if (mg3.isWin) {
            DrawText("COMBAT CLEARED!", sw/2 - 180, sh/2 - 30, 40, GREEN);
            DrawText("New map unlocked", sw/2 - 120, sh/2 + 30, 24, WHITE);
        } else {
            DrawText("MISSION FAILED", sw/2 - 160, sh/2 - 30, 40, RED);
        }
    }
}

void UnloadMinigame3(void) { 
    //卸载纹理
    if (mg3.texPlayer.id > 0) UnloadTexture(mg3.texPlayer);
    if (mg3.texEnemy1.id > 0) UnloadTexture(mg3.texEnemy1);
    if (mg3.texEnemy2.id > 0) UnloadTexture(mg3.texEnemy2);
    if (mg3.texBulletPlayer.id > 0) UnloadTexture(mg3.texBulletPlayer);
    if (mg3.texBulletEnemy.id > 0) UnloadTexture(mg3.texBulletEnemy);
    if (mg3.texDrop.id > 0) UnloadTexture(mg3.texDrop);
    if (mg3.texWood.id > 0) UnloadTexture(mg3.texWood);
    if (mg3.texPiano.id > 0) UnloadTexture(mg3.texPiano);
    if (mg3.texHeart.id > 0) UnloadTexture(mg3.texHeart);

    memset(&mg3, 0, sizeof(mg3)); 
}