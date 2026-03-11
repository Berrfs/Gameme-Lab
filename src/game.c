#include "game.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>   // 新增：用于字符串操作

GameContext game;

// 静态函数声明
static void DrawTitle(void);
static void DrawPlaying(void);
static void DrawChoice(void);
static void DrawSettings(void);
static void DrawNameInput(void);        // 新增
static void HandleTitleInput(void);
static void UpdatePlaying(void);
static void HandleChoiceInput(void);
static void HandleSettingsInput(void);
static void HandleNameInput(void);       // 新增
static void SelectChoice(int index);
static bool IsButtonClicked(Texture2D tex, int posX, int posY, float scale);
static bool IsButtonHovered(Texture2D tex, int posX, int posY, float scale);

// --- 辅助函数：按钮点击检测 ---
static bool IsButtonClicked(Texture2D tex, int posX, int posY, float scale) {
    int btnWidth = (int)(tex.width * scale);
    int btnHeight = (int)(tex.height * scale);
    Rectangle btnRect = { (float)posX, (float)posY, (float)btnWidth, (float)btnHeight };
    return (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), btnRect));
}

static bool IsButtonHovered(Texture2D tex, int posX, int posY, float scale) {
    int btnWidth = (int)(tex.width * scale);
    int btnHeight = (int)(tex.height * scale);
    Rectangle btnRect = { (float)posX, (float)posY, (float)btnWidth, (float)btnHeight };
    return CheckCollisionPointRec(GetMousePosition(), btnRect);
}

// --- 初始化 ---
void InitGame(void) {
    // 加载场景数据
    LoadScenesFromJSON("data/scenes.json");

    // 初始化状态
    game.state = STATE_TITLE;
    game.current_scene = GetSceneByID("scene1");
    game.dialogue_index = 0;

    // 默认设置
    game.master_volume = 0.8f;
    game.auto_mode = false;
    game.auto_interval = 2.0f;
    game.auto_timer = 0.0f;

    // 设置默认玩家名字
    strcpy(game.player_name, "Player");

    // 加载标题画面纹理
    game.titleBackground = LoadTexture("UI/red curtain.jpg");
    game.titleLogo = LoadTexture("UI/no way.png");
    game.gamemeLabLogo = LoadTexture("UI/Gameme Lab.png");
    game.btnStart = LoadTexture("UI/touch to start.png");
    game.btnMenu = LoadTexture("UI/menu.png");
    game.btnExit = LoadTexture("UI/exit.png");
}

// --- 主更新循环 ---
void UpdateGame(void) {
    switch (game.state) {
        case STATE_TITLE:       HandleTitleInput(); break;
        case STATE_NAME_INPUT:  HandleNameInput(); break;   // 新增
        case STATE_PLAYING:     UpdatePlaying(); break;
        case STATE_CHOICE:      HandleChoiceInput(); break;
        case STATE_SETTINGS:    HandleSettingsInput(); break;
    }
}

// --- 主绘制循环 ---
void DrawGame(void) {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    switch (game.state) {
        case STATE_TITLE:       DrawTitle(); break;
        case STATE_NAME_INPUT:  DrawNameInput(); break;    // 新增
        case STATE_PLAYING:     DrawPlaying(); break;
        case STATE_CHOICE:      DrawChoice(); break;
        case STATE_SETTINGS:    DrawSettings(); break;
    }

    EndDrawing();
}

// --- 资源释放 ---
void UnloadGame(void) {
    UnloadScenes();
    UnloadTexture(game.titleBackground);
    UnloadTexture(game.titleLogo);
    UnloadTexture(game.gamemeLabLogo);
    UnloadTexture(game.btnStart);
    UnloadTexture(game.btnMenu);
    UnloadTexture(game.btnExit);
}

// ==================== 标题画面 ====================
static void DrawTitle(void) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // 背景
    DrawTexturePro(game.titleBackground,
        (Rectangle){0, 0, (float)game.titleBackground.width, (float)game.titleBackground.height},
        (Rectangle){0, 0, (float)screenWidth, (float)screenHeight},
        (Vector2){0, 0}, 0.0f, WHITE);

    // 1. 左侧大标牌 "NO WAY!"
    float logoScale = 0.40f;
    int logoW = (int)(game.titleLogo.width * logoScale);
    int logoH = (int)(game.titleLogo.height * logoScale);
    int logoX = 40;
    int logoY = (screenHeight - logoH) / 2;
    DrawTextureEx(game.titleLogo, (Vector2){ (float)logoX, (float)logoY }, 0.0f, logoScale, WHITE);

    // 2. 右上角 "Gameme Lab" 图标
    float glScale = 0.80f;
    int glW = (int)(game.gamemeLabLogo.width * glScale);
    int glX = screenWidth - glW - 60;
    int glY = 40;
    DrawTextureEx(game.gamemeLabLogo, (Vector2){ (float)glX, (float)glY }, 0.0f, glScale, WHITE);

    int rightAreaCenterX = screenWidth * 3 / 4;

    // 3. 按钮 "Touch to start"
    float startScale = 0.39f;
    int startW = (int)(game.btnStart.width * startScale);
    int startH = (int)(game.btnStart.height * startScale);
    int startX = rightAreaCenterX - startW / 2;
    int startY = 400;
    if (IsButtonHovered(game.btnStart, startX, startY, startScale)) {
        float hs = startScale * 1.05f;
        int hx = startX - (int)((game.btnStart.width * hs - startW) / 2);
        int hy = startY - (int)((game.btnStart.height * hs - startH) / 2);
        DrawTextureEx(game.btnStart, (Vector2){ (float)hx, (float)hy }, 0.0f, hs, WHITE);
    } else {
        DrawTextureEx(game.btnStart, (Vector2){ (float)startX, (float)startY }, 0.0f, startScale, WHITE);
    }

    // 4. 按钮 "Menu"
    float menuScale = 0.11f;
    int menuW = (int)(game.btnMenu.width * menuScale);
    int menuH = (int)(game.btnMenu.height * menuScale);
    int menuX = rightAreaCenterX - menuW / 2;
    int menuY = startY + startH + 40;
    if (IsButtonHovered(game.btnMenu, menuX, menuY, menuScale)) {
        float hs = menuScale * 1.05f;
        int hx = menuX - (int)((game.btnMenu.width * hs - menuW) / 2);
        int hy = menuY - (int)((game.btnMenu.height * hs - menuH) / 2);
        DrawTextureEx(game.btnMenu, (Vector2){ (float)hx, (float)hy }, 0.0f, hs, WHITE);
    } else {
        DrawTextureEx(game.btnMenu, (Vector2){ (float)menuX, (float)menuY }, 0.0f, menuScale, WHITE);
    }

    // 5. 按钮 "Exit"
    float exitScale = 0.11f;
    int exitW = (int)(game.btnExit.width * exitScale);
    int exitH = (int)(game.btnExit.height * exitScale);
    int exitX = rightAreaCenterX - exitW / 2;
    int exitY = menuY + menuH + 40;
    if (IsButtonHovered(game.btnExit, exitX, exitY, exitScale)) {
        float hs = exitScale * 1.05f;
        int hx = exitX - (int)((game.btnExit.width * hs - exitW) / 2);
        int hy = exitY - (int)((game.btnExit.height * hs - exitH) / 2);
        DrawTextureEx(game.btnExit, (Vector2){ (float)hx, (float)hy }, 0.0f, hs, WHITE);
    } else {
        DrawTextureEx(game.btnExit, (Vector2){ (float)exitX, (float)exitY }, 0.0f, exitScale, WHITE);
    }
}

static void HandleTitleInput(void) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    int rightAreaCenterX = screenWidth * 3 / 4;

    float startScale = 0.39f;
    int startW = (int)(game.btnStart.width * startScale);
    int startH = (int)(game.btnStart.height * startScale);
    int startX = rightAreaCenterX - startW / 2;
    int startY = 400;

    float menuScale = 0.11f;
    int menuW = (int)(game.btnMenu.width * menuScale);
    int menuH = (int)(game.btnMenu.height * menuScale);
    int menuX = rightAreaCenterX - menuW / 2;
    int menuY = startY + startH + 40;

    float exitScale = 0.11f;
    int exitW = (int)(game.btnExit.width * exitScale);
    int exitH = (int)(game.btnExit.height * exitScale);
    int exitX = rightAreaCenterX - exitW / 2;
    int exitY = menuY + menuH + 40;

    // 点击 "Touch to start" 进入姓名输入界面
    if (IsButtonClicked(game.btnStart, startX, startY, startScale)) {
        game.state = STATE_NAME_INPUT;
        strcpy(game.player_name, "");   // 清空，让用户输入
    }
    if (IsButtonClicked(game.btnMenu, menuX, menuY, menuScale)) {
        game.state = STATE_SETTINGS;
    }
    if (IsButtonClicked(game.btnExit, exitX, exitY, exitScale)) {
        CloseWindow();
    }
    // 键盘快捷键
    if (IsKeyPressed(KEY_ENTER)) {
        game.state = STATE_NAME_INPUT;
        strcpy(game.player_name, "");
    }
    if (IsKeyPressed(KEY_S)) game.state = STATE_SETTINGS;
}

// ==================== 姓名输入界面 ====================
static void DrawNameInput(void) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // 半透明背景或纯色背景
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 50, 50, 50, 255 });

    // 提示文字
    const char *prompt = "Please enter your name (English only, max 20 chars):";
    DrawText(prompt, screenWidth/2 - MeasureText(prompt, 40)/2, screenHeight/2 - 100, 40, WHITE);

    // 绘制输入框
    int boxWidth = 600;
    int boxHeight = 60;
    int boxX = screenWidth/2 - boxWidth/2;
    int boxY = screenHeight/2 - boxHeight/2;
    DrawRectangle(boxX, boxY, boxWidth, boxHeight, LIGHTGRAY);

    // 绘制当前输入的名字
    int fontSize = 40;
    int textWidth = MeasureText(game.player_name, fontSize);
    DrawText(game.player_name, boxX + 10, boxY + 10, fontSize, BLACK);

    // 绘制光标（闪烁效果）
    if (((int)(GetTime() * 2) % 2) == 0) {
        int caretX = boxX + 10 + textWidth;
        DrawRectangle(caretX, boxY + 10, 3, fontSize, BLACK);
    }

    // 提示按回车确认
    DrawText("Press ENTER to confirm", screenWidth/2 - 150, boxY + 80, 20, GRAY);
}

static void HandleNameInput(void) {
    // 获取按下的字符
    int key = GetCharPressed();
    while (key > 0) {
        // 只允许英文字母（大小写）和空格
        if ((key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z') || key == ' ') {
            int len = strlen(game.player_name);
            if (len < 20) {
                game.player_name[len] = (char)key;
                game.player_name[len+1] = '\0';
            }
        }
        key = GetCharPressed();
    }

    // 处理退格键
    if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = strlen(game.player_name);
        if (len > 0) {
            game.player_name[len-1] = '\0';
        }
    }

    // 按下回车确认
    if (IsKeyPressed(KEY_ENTER)) {
        // 如果名字为空，设为默认 "Player"
        if (strlen(game.player_name) == 0) {
            strcpy(game.player_name, "Player");
        }
        // 进入游戏
        game.state = STATE_PLAYING;
        game.dialogue_index = 0;
        game.auto_timer = 0.0f;
    }
}

// ==================== 剧情播放 ====================
static void DrawPlaying(void) {
    Scene *sc = game.current_scene;
    if (!sc) return;

    // 临时用颜色代替背景
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), LIGHTGRAY);
    DrawText(TextFormat("Background: %s", sc->background ? sc->background : "none"), 40, 40, 20, BLACK);

    if (game.dialogue_index < sc->dialogue_count) {
        Dialogue *d = &sc->dialogues[game.dialogue_index];
        // 判断说话者是否为 "Player"，如果是则显示玩家名字
        const char *speaker = d->speaker;
        if (strcmp(speaker, "Player") == 0) {
            speaker = game.player_name;
        }
        DrawText(speaker, 100, 1000, 40, MAROON);
        DrawText(d->text, 100, 1060, 40, DARKGRAY);
    } else {
        DrawText("End of scene. Press ESC to title.", 400, 600, 40, BLACK);
    }

    // 显示当前模式
    DrawText(TextFormat("Mode: %s", game.auto_mode ? "AUTO" : "MANUAL"), 20, 20, 30, BLACK);
}

static void UpdatePlaying(void) {
    Scene *sc = game.current_scene;
    if (!sc) return;

    if (game.auto_mode) {
        // 自动翻页
        game.auto_timer += GetFrameTime();
        if (game.auto_timer >= game.auto_interval) {
            game.auto_timer = 0.0f;
            game.dialogue_index++;
            if (game.dialogue_index >= sc->dialogue_count) {
                if (sc->choice_count > 0) {
                    game.state = STATE_CHOICE;
                } else {
                    game.state = STATE_TITLE;
                }
            }
        }
    } else {
        // 手动点击
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE)) {
            game.dialogue_index++;
            if (game.dialogue_index >= sc->dialogue_count) {
                if (sc->choice_count > 0) {
                    game.state = STATE_CHOICE;
                } else {
                    game.state = STATE_TITLE;
                }
            }
        }
    }

    // 通用按键
    if (IsKeyPressed(KEY_ESCAPE)) game.state = STATE_TITLE;
    if (IsKeyPressed(KEY_S)) game.state = STATE_SETTINGS;
}

// ==================== 选项界面 ====================
static void DrawChoice(void) {
    Scene *sc = game.current_scene;
    if (!sc) return;

    // 半透明遮罩
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 150});

    int startY = 600;
    for (int i = 0; i < sc->choice_count; i++) {
        Color color = (i == 0) ? YELLOW : WHITE;
        DrawText(TextFormat("%d. %s", i+1, sc->choices[i].text), 400, startY + i*80, 60, color);
    }
    DrawText("Press number key to choose", 400, startY + sc->choice_count*80 + 40, 40, GRAY);
}

static void HandleChoiceInput(void) {
    Scene *sc = game.current_scene;
    if (!sc) return;

    // 键盘选择（数字键1~9）
    for (int i = 0; i < sc->choice_count; i++) {
        if (IsKeyPressed(KEY_ONE + i)) {
            SelectChoice(i);
            return;
        }
    }

    // 鼠标选择（粗略区域）
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse = GetMousePosition();
        int startY = 600;
        for (int i = 0; i < sc->choice_count; i++) {
            Rectangle rect = {400, startY + i*80 - 10, 800, 60};
            if (CheckCollisionPointRec(mouse, rect)) {
                SelectChoice(i);
                return;
            }
        }
    }
}

static void SelectChoice(int index) {
    Scene *sc = game.current_scene;
    if (!sc || index >= sc->choice_count) return;
    Scene *next = GetSceneByID(sc->choices[index].next_scene_id);
    if (next) {
        game.current_scene = next;
        game.dialogue_index = 0;
        game.state = STATE_PLAYING;
        game.auto_timer = 0.0f;
    } else {
        game.state = STATE_TITLE;
    }
}

// ==================== 设置界面 ====================
static void DrawSettings(void) {
    DrawText("Settings", 700, 200, 60, BLACK);

    // 音量
    DrawText(TextFormat("Master Volume: %.0f%%", game.master_volume * 100), 400, 400, 40, BLACK);
    DrawRectangle(400, 460, 800, 40, LIGHTGRAY);
    DrawRectangle(400, 460, (int)(800 * game.master_volume), 40, BLUE);

    // 自动翻页开关
    DrawText(TextFormat("Auto Mode: %s", game.auto_mode ? "ON" : "OFF"), 400, 560, 40, BLACK);
    DrawRectangle(400, 620, 800, 40, LIGHTGRAY);
    DrawRectangle(400, 620, (int)(800 * (game.auto_mode ? 1 : 0)), 40, PURPLE);
    DrawText("Press M to toggle Auto/Manual", 400, 680, 30, BLACK);

    // 自动间隔
    DrawText(TextFormat("Auto Interval: %.1f s", game.auto_interval), 400, 740, 40, BLACK);
    DrawRectangle(400, 800, 800, 40, LIGHTGRAY);
    DrawRectangle(400, 800, (int)(800 * (game.auto_interval / 5.0f)), 40, ORANGE);
    DrawText("Up/Down: adjust interval (0.5~5.0s)", 400, 860, 30, BLACK);

    DrawText("Press B to go back", 600, 1000, 40, DARKGRAY);
}

static void HandleSettingsInput(void) {
    // 音量（左右键）
    if (IsKeyPressed(KEY_RIGHT)) {
        game.master_volume += 0.05f;
        if (game.master_volume > 1.0f) game.master_volume = 1.0f;
    }
    if (IsKeyPressed(KEY_LEFT)) {
        game.master_volume -= 0.05f;
        if (game.master_volume < 0.0f) game.master_volume = 0.0f;
    }

    // 切换自动模式
    if (IsKeyPressed(KEY_M)) {
        game.auto_mode = !game.auto_mode;
    }

    // 调整自动间隔（上下键）
    if (IsKeyPressed(KEY_UP)) {
        game.auto_interval += 0.2f;
        if (game.auto_interval > 5.0f) game.auto_interval = 5.0f;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        game.auto_interval -= 0.2f;
        if (game.auto_interval < 0.5f) game.auto_interval = 0.5f;
    }

    // 返回
    if (IsKeyPressed(KEY_B)) game.state = STATE_TITLE;
}