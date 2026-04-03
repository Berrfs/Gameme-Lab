#include "bossbattle.h"
#include "game.h"
#include "scene.h"
#include "raylib.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

extern GameContext game;

// ==========================================
//  Boss 战难度与机制调整区
// ==========================================
#define PLAYER_HP_MAX       5       // 玩家最大血量
#define PLAYER_SPEED        300.0f  // 玩家移动速度

#define BOSS_HP_MAX         3       // Boss最大血量 (重铸武器后需要攻击的次数)
#define BOSS_FIRE_RATE      1.2f    // Boss 发射弹幕的间隔时间 (秒)
#define BOSS_BULLET_SPEED   400.0f  // 弹幕飞行速度
#define BOSS_BULLET_COUNT   3       // 每次发射散弹的数量
#define BOSS_SPREAD_ANGLE   45.0f   // 散弹的扩散角度 (度)

#define PIECE_REQUIRED      5       // 修复武器需要捡起的碎片总数
#define ATTACK_RANGE        1200.0f  // 玩家必须在此距离内点击 Boss 才有效
// ==========================================

#define MAX_BULLETS 100
#define MAX_UI_BLOCKS 3
#define MAX_PIECES 10

// 游戏阶段
typedef enum {
    PHASE_INTRO,        // 开场对话
    PHASE_SURVIVAL,     // 躲避弹幕，UI被摧毁，收集碎片
    PHASE_REPAIRING,    // 收集满碎片，Game 提示重铸中
    PHASE_COUNTER,      // 武器修好，玩家靠近Boss反击
    PHASE_DEFEATED      // Boss死亡演出
} BossPhase;

// 实体结构体
typedef struct { Vector2 pos; float radius; int hp; } Entity;
typedef struct { bool active; Vector2 pos; Vector2 vel; float radius; } Bullet;
typedef struct { bool active; Rectangle rect; int hp; } UIBlock; // 假的Attack按钮
typedef struct { 
    bool active; 
    Vector2 pos; 
    Vector2 vel; // 碎片飞行速度
    float radius; 
    float friction; // 摩擦力，让碎片飞出去后慢慢停下
} Piece; // 掉落的碎片

static struct {
    Entity player;
    Entity boss;
    Bullet bullets[MAX_BULLETS];
    UIBlock uiBlocks[MAX_UI_BLOCKS];
    Piece pieces[MAX_PIECES];

    // 精灵图与动画
    Texture2D texPlayer;
    Texture2D texBoss;
    int pFrame;
    int pDir;       // 0下 1上 2左 3右
    float pAnimTimer;
    bool pIsMoving;

    BossPhase phase;
    float bossShootTimer;
    
    int collectedPieces;
    bool hasWeapon;     // 是否已重铸攻击按钮
    float attackCooldown; // 玩家攻击CD

    // 对话框提示
    char dialogSpeaker[32];
    char dialogText[128];
    float dialogTimer;
    
    bool isGameOver;
    float endTimer;
} bb = {0};

/* 辅助：显示短期对话提示 */
static void ShowDialog(const char* speaker, const char* text, float duration) {
    strncpy(bb.dialogSpeaker, speaker, 31);
    strncpy(bb.dialogText, text, 127);
    bb.dialogTimer = duration;
}

/* 辅助：发射Boss弹幕 (自机狙+散弹) */
static void FireBossBullets() {
    float dx = bb.player.pos.x - bb.boss.pos.x;
    float dy = bb.player.pos.y - bb.boss.pos.y;
    float baseAngle = atan2f(dy, dx); // 瞄准玩家的角度

    int halfCount = BOSS_BULLET_COUNT / 2;
    float angleStep = (BOSS_SPREAD_ANGLE * PI / 180.0f) / (BOSS_BULLET_COUNT > 1 ? BOSS_BULLET_COUNT - 1 : 1);

    for (int i = 0; i < BOSS_BULLET_COUNT; i++) {
        float angle = baseAngle + (i - halfCount) * angleStep;
        Vector2 vel = { cosf(angle) * BOSS_BULLET_SPEED, sinf(angle) * BOSS_BULLET_SPEED };

        for (int j = 0; j < MAX_BULLETS; j++) {
            if (!bb.bullets[j].active) {
                bb.bullets[j].active = true;
                bb.bullets[j].pos = bb.boss.pos;
                bb.bullets[j].vel = vel;
                bb.bullets[j].radius = 8.0f;
                break;
            }
        }
    }
}

/* 辅助：掉落碎片 */
static void SpawnPiece(Vector2 pos) {
    for (int i = 0; i < MAX_PIECES; i++) {
        if (!bb.pieces[i].active) {
            bb.pieces[i].active = true;
            bb.pieces[i].pos = pos;
            bb.pieces[i].radius = 12.0f;
            
            // 随机生成一个向四周弹射的速度
            float angle = (float)GetRandomValue(0, 360) * PI / 180.0f;
            float speed = (float)GetRandomValue(400, 700); // 初始弹射速度很快
            bb.pieces[i].vel = (Vector2){ cosf(angle) * speed, sinf(angle) * speed };
            bb.pieces[i].friction = 0.92f; // 每帧减速，最终停在角落
            break;
        }
    }
}

void InitBossBattle(void) {
    memset(&bb, 0, sizeof(bb));
    int sw = GetScreenWidth(), sh = GetScreenHeight();

    bb.texPlayer = LoadTexture("UI/player_sprite.png");
    bb.texBoss = LoadTexture("UI/mr_glitch.png"); // 必须有这张图，否则画方块

    bb.player.pos = (Vector2){ sw / 2.0f, sh - 150.0f };
    bb.player.hp = PLAYER_HP_MAX;
    bb.player.radius = 20.0f;
    bb.pDir = 1; // 初始面朝上(背对屏幕看着Boss)

    bb.boss.pos = (Vector2){ sw / 2.0f, 150.0f };
    bb.boss.hp = BOSS_HP_MAX;
    bb.boss.radius = 40.0f;

    // 放置三个假的攻击按钮 (盾牌)
    bb.uiBlocks[0] = (UIBlock){ true, { sw/2.0f - 250, sh/2.0f, 120, 50 }, 3 };
    bb.uiBlocks[1] = (UIBlock){ true, { sw/2.0f - 60, sh/2.0f + 50, 120, 50 }, 3 };
    bb.uiBlocks[2] = (UIBlock){ true, { sw/2.0f + 130, sh/2.0f, 120, 50 }, 3 };

    bb.phase = PHASE_INTRO;
    ShowDialog("Mr. Glitch", "You think you can defeat me by clicking buttons?\nLook! Your UI is completely useless here!", 4.0f);
}

void UpdateBossBattle(void) {
    float dt = GetFrameTime();
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    Vector2 mouse = GetMousePosition(); 

    if (bb.dialogTimer > 0) bb.dialogTimer -= dt;
    if (bb.attackCooldown > 0) bb.attackCooldown -= dt;

    if (bb.isGameOver) {
        bb.endTimer -= dt;
        if (bb.endTimer <= 0) {
            // 1. 先保存胜负结果（因为 Unload 会清空 bb 结构体）
            bool playerWon = (bb.boss.hp <= 0);

            // 2. 彻底卸载 Boss 战资源
            UnloadBossBattle();

            // 3. 根据结果跳转场景
            if (playerWon) {
                // 胜利：进入 scene10 (Game 感谢你)
                game.current_scene = GetSceneByID("scene10");
            } else {
                // 失败：进入 ending2 (被 Mr. Glitch 删除)
                game.current_scene = GetSceneByID("ending2");
            }

            // 4. 重置游戏状态为剧情播放模式
            game.state = STATE_PLAYING;
            game.dialogue_index = 0;
            game.auto_timer = 0.0f;
        }
        return;
    }

    // --- 开场后切换到生存阶段 ---
    if (bb.phase == PHASE_INTRO && bb.dialogTimer <= 0) {
        bb.phase = PHASE_SURVIVAL;
    }

    // --- 1. 玩家 WASD 移动与精灵图动画 ---
    bb.pIsMoving = false;
    Vector2 pNext = bb.player.pos;

    if (IsKeyDown(KEY_W)) { pNext.y -= PLAYER_SPEED * dt; bb.pDir = 1; bb.pIsMoving = true; }
    if (IsKeyDown(KEY_S)) { pNext.y += PLAYER_SPEED * dt; bb.pDir = 0; bb.pIsMoving = true; }
    if (IsKeyDown(KEY_A)) { pNext.x -= PLAYER_SPEED * dt; bb.pDir = 2; bb.pIsMoving = true; }
    if (IsKeyDown(KEY_D)) { pNext.x += PLAYER_SPEED * dt; bb.pDir = 3; bb.pIsMoving = true; }

    // 限制在屏幕内
    if (pNext.x > bb.player.radius && pNext.x < sw - bb.player.radius) bb.player.pos.x = pNext.x;
    if (pNext.y > bb.player.radius && pNext.y < sh - bb.player.radius) bb.player.pos.y = pNext.y;

    if (bb.pIsMoving) {
        bb.pAnimTimer += dt;
        if (bb.pAnimTimer > 0.15f) { bb.pFrame = (bb.pFrame + 1) % 4; bb.pAnimTimer = 0; }
    } else {
        bb.pFrame = 1; // 站立帧
    }

    // --- 2. Boss 弹幕逻辑 (仅在存活阶段) ---
    if (bb.phase == PHASE_SURVIVAL || bb.phase == PHASE_COUNTER) {
        bb.bossShootTimer -= dt;
        if (bb.bossShootTimer <= 0) {
            FireBossBullets();
            bb.bossShootTimer = BOSS_FIRE_RATE;
        }
    }

    // --- 3. 弹幕移动与碰撞检测 ---
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bb.bullets[i].active) {
            bb.bullets[i].pos.x += bb.bullets[i].vel.x * dt;
            bb.bullets[i].pos.y += bb.bullets[i].vel.y * dt;

            // 飞出屏幕销毁
            if (bb.bullets[i].pos.y > sh || bb.bullets[i].pos.y < 0 || bb.bullets[i].pos.x < 0 || bb.bullets[i].pos.x > sw) {
                bb.bullets[i].active = false;
                continue;
            }

            // 碰撞检测：打中假的 UI 按钮
            bool hitBlock = false;
            for (int j = 0; j < MAX_UI_BLOCKS; j++) {
                if (bb.uiBlocks[j].active && CheckCollisionCircleRec(bb.bullets[i].pos, bb.bullets[i].radius, bb.uiBlocks[j].rect)) {
                    bb.bullets[i].active = false;
                    hitBlock = true;
                    
                    // 只有在没修好武器前，按钮才会被打碎
                    if (bb.phase == PHASE_SURVIVAL) {
                        bb.uiBlocks[j].hp--;
                        if (bb.uiBlocks[j].hp <= 0) {
                            bb.uiBlocks[j].active = false;
                            // 一个按钮炸出 3 个碎片
                            for (int p = 0; p < 3; p++) {
                                SpawnPiece((Vector2){ bb.uiBlocks[j].rect.x + 60, bb.uiBlocks[j].rect.y + 25 });
                            }
                            // 第一次打碎时，Game 给予提示
                            if (bb.collectedPieces == 0 && bb.dialogTimer <= 0) {
                                ShowDialog("Game", "Wait! If you gather those shattered code fragments,\nI can reconstruct a working [ATTACK] module for you!", 4.0f);
                            }
                        }
                    }
                    break;
                }
            }
            if (hitBlock) continue; 

            // 碰撞检测：打中玩家
            float dx = bb.bullets[i].pos.x - bb.player.pos.x;
            float dy = bb.bullets[i].pos.y - bb.player.pos.y;
            if (dx*dx + dy*dy < (bb.player.radius + bb.bullets[i].radius)*(bb.player.radius + bb.bullets[i].radius)) {
                bb.bullets[i].active = false;
                bb.player.hp--;
                if (bb.player.hp <= 0) {
                    bb.isGameOver = true; bb.endTimer = 2.0f;
                }
            }
        }
    }

    // --- 碎片的物理滑动 (散射效果) ---
    for (int i = 0; i < MAX_PIECES; i++) {
        if (bb.pieces[i].active) {
            // 移动碎片
            bb.pieces[i].pos.x += bb.pieces[i].vel.x * dt;
            bb.pieces[i].pos.y += bb.pieces[i].vel.y * dt;
            // 模拟摩擦力减速
            bb.pieces[i].vel.x *= bb.pieces[i].friction;
            bb.pieces[i].vel.y *= bb.pieces[i].friction;

            // 屏幕边界碰撞（弹回一点，防止飞出屏幕看不见）
            if (bb.pieces[i].pos.x < 10 || bb.pieces[i].pos.x > GetScreenWidth() - 10) bb.pieces[i].vel.x *= -1;
            if (bb.pieces[i].pos.y < 10 || bb.pieces[i].pos.y > GetScreenHeight() - 10) bb.pieces[i].vel.y *= -1;

            // 玩家拾取检测 (逻辑不变)
            // ... 拾取代码 ...
        }
    }
    // --- 4. 拾取碎片逻辑 ---
    for (int i = 0; i < MAX_PIECES; i++) {
        if (bb.pieces[i].active) {
            float dx = bb.pieces[i].pos.x - bb.player.pos.x;
            float dy = bb.pieces[i].pos.y - bb.player.pos.y;
            if (dx*dx + dy*dy < (bb.player.radius + bb.pieces[i].radius)*(bb.player.radius + bb.pieces[i].radius)) {
                bb.pieces[i].active = false;
                bb.collectedPieces++;

                if (bb.collectedPieces >= PIECE_REQUIRED && bb.phase == PHASE_SURVIVAL) {
                    bb.phase = PHASE_REPAIRING;
                    ShowDialog("Game", "Recompiling fragments... Success!\nNow! Get close and smash him with the new UI!", 4.0f);
                    bb.hasWeapon = true;
                }
            }
        }
    }

    if (bb.phase == PHASE_REPAIRING && bb.dialogTimer <= 0) {
        bb.phase = PHASE_COUNTER;
    }

    // --- 5. 玩家靠近反击 Boss ---
    if (bb.phase == PHASE_COUNTER && bb.hasWeapon && bb.attackCooldown <= 0) {
        // 只有玩家点击鼠标左键时才触发
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            float dx = bb.player.pos.x - bb.boss.pos.x;
            float dy = bb.player.pos.y - bb.boss.pos.y;
            float dist = sqrtf(dx*dx + dy*dy);

            // 必须满足两个条件：1. 距离 Boss 够近  2. 鼠标点击在 Boss 身上
            Rectangle bossRect = { bb.boss.pos.x , bb.boss.pos.y , 800, 800 };
            if (dist < ATTACK_RANGE && CheckCollisionPointRec(mouse, bossRect)) {
                bb.boss.hp--;
                bb.attackCooldown = 0.1f; // 缩短一点攻击CD
                // 屏幕轻微震动反馈
                bb.player.pos.y += 10; 
            }    
            
            if (bb.boss.hp <= 0) {
                bb.phase = PHASE_DEFEATED;
                bb.isGameOver = true;
                bb.endTimer = 3.0f;
                ShowDialog("Mr. Glitch", "IMPOSSIBLE... How could mere broken code... NOOOOOOO!", 3.0f);
            }
        }
    }
}

void DrawBossBattle(void) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();

    // 背景绘制 
    ClearBackground((Color){ 20, 5, 5, 255 });
    
    // 绘制被污染的网格线作为背景
    for (int i = 0; i < sw; i+= 50) DrawLine(i, 0, i, sh, (Color){ 50, 0, 0, 100 });
    for (int i = 0; i < sh; i+= 50) DrawLine(0, i, sw, i, (Color){ 50, 0, 0, 100 });

    // --- 1. 绘制 Boss (如果被打了闪烁红光) ---
    if (bb.boss.hp > 0) {
        Color bColor = (bb.attackCooldown > 0) ? RED : WHITE; // 受击发红
        if (bb.texBoss.id > 0) {
            DrawTexturePro(bb.texBoss, 
                (Rectangle){0, 0, bb.texBoss.width, bb.texBoss.height},
                (Rectangle){bb.boss.pos.x - 80, bb.boss.pos.y - 80, 160, 160},
                (Vector2){0,0}, 0.0f, bColor);
        } else {
            DrawCircleV(bb.boss.pos, bb.boss.radius, PURPLE);
            DrawText("Mr. Glitch", bb.boss.pos.x - 40, bb.boss.pos.y - 60, 20, RED);
        }
    }

    // --- 2. 绘制假的/碎裂的 UI 按钮 ---
    for (int i = 0; i < MAX_UI_BLOCKS; i++) {
        if (bb.uiBlocks[i].active) {
            Rectangle r = bb.uiBlocks[i].rect;
            Color uiColor = (bb.uiBlocks[i].hp == 3) ? GRAY : ((bb.uiBlocks[i].hp == 2) ? DARKGRAY : MAROON);
            DrawRectangleRec(r, uiColor);
            DrawRectangleLinesEx(r, 2, WHITE);
            // 制造乱码感
            if (bb.uiBlocks[i].hp < 3) DrawText("$@!*#", r.x + 20, r.y + 15, 20, RED);
            else DrawText("[ ATTACK ]", r.x + 10, r.y + 15, 20, WHITE);
        }
    }

    // --- 3. 绘制掉落的碎片 ---
    for (int i = 0; i < MAX_PIECES; i++) {
        if (bb.pieces[i].active) {
            // 画一个绿色的代码块
            DrawRectangle(bb.pieces[i].pos.x - 10, bb.pieces[i].pos.y - 10, 20, 20, LIME);
            DrawText("01", bb.pieces[i].pos.x - 8, bb.pieces[i].pos.y - 5, 10, BLACK);
        }
    }

    // --- 4. 绘制弹幕 (乱码球) ---
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bb.bullets[i].active) {
            DrawCircleV(bb.bullets[i].pos, bb.bullets[i].radius, RED);
            DrawCircleV(bb.bullets[i].pos, bb.bullets[i].radius/2, YELLOW);
        }
    }

    // --- 5. 绘制玩家与修好的武器 ---
    if (bb.player.hp > 0) {
        // 重铸的巨大 ATTACK 环绕玩家
        if (bb.hasWeapon) {
            DrawRectangleLines(bb.player.pos.x - 50, bb.player.pos.y - 50, 100, 100, LIME);
            DrawText("> ATTACK <", bb.player.pos.x - 40, bb.player.pos.y - 60, 16, LIME);
            if (bb.phase == PHASE_COUNTER && bb.attackCooldown <= 0) {
                DrawCircleLines(bb.player.pos.x, bb.player.pos.y, 80, GREEN); // 显示攻击范围
            }
        }

        if (bb.texPlayer.id > 0) {
            float fw = (float)bb.texPlayer.width / 4;
            float fh = (float)bb.texPlayer.height / 4;
            Rectangle src = { bb.pFrame * fw, bb.pDir * fh, fw, fh };
            Rectangle dest = { bb.player.pos.x - 30, bb.player.pos.y - 30, 60, 60 };
            DrawTexturePro(bb.texPlayer, src, dest, (Vector2){0,0}, 0.0f, WHITE);
        } else {
            DrawCircleV(bb.player.pos, bb.player.radius, SKYBLUE);
        }
    }

    // --- UI 血条绘制 ---
    // 玩家血条
    DrawText("PLAYER HP:", 20, 20, 20, WHITE);
    for (int i = 0; i < PLAYER_HP_MAX; i++) {
        DrawRectangle(140 + i*30, 20, 25, 20, (i < bb.player.hp) ? LIME : DARKGRAY);
    }
    // Boss 血条
    DrawText("BOSS HP:", sw - 280, 20, 20, RED);
    for (int i = 0; i < BOSS_HP_MAX; i++) {
        DrawRectangle(sw - 180 + i*40, 20, 35, 20, (i < bb.boss.hp) ? RED : DARKGRAY);
    }

    // --- 对话框绘制 ---
    if (bb.dialogTimer > 0) {
        int boxY = sh - 150;
        DrawRectangle(0, boxY, sw, 150, (Color){ 0, 0, 0, 200 });
        DrawText(bb.dialogSpeaker, 50, boxY + 20, 24, (strcmp(bb.dialogSpeaker, "Game") == 0) ? SKYBLUE : PURPLE);
        DrawText(bb.dialogText, 50, boxY + 60, 24, WHITE);
    }

    // --- 结束字幕 ---
    if (bb.isGameOver) {
        DrawRectangle(0, 0, sw, sh, (Color){0,0,0,150});
        if (bb.boss.hp <= 0) {
            DrawText("SYSTEM RECOVERED", sw/2 - 180, sh/2 - 30, 40, LIME);
        } else {
            DrawText("FATAL ERROR. PLAYER DEFEATED.", sw/2 - 280, sh/2 - 30, 40, RED);
        }
    }
}

void UnloadBossBattle(void) {
    if (bb.texPlayer.id > 0) UnloadTexture(bb.texPlayer);
    if (bb.texBoss.id > 0) UnloadTexture(bb.texBoss);
    memset(&bb, 0, sizeof(bb));
}