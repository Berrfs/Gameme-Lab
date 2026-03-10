#include "game.h"
#include "raylib.h"
#include <stdio.h>

// 定义全局游戏上下文
GameContext game;

// 静态辅助函数声明（仅限本文件使用）
static void DrawTitle(void);
static void DrawPlaying(void);
static void DrawSettings(void);
static void HandleTitleInput(void);
static void HandlePlayingInput(void);
static void HandleSettingsInput(void);

void InitGame(void) {
    // 加载场景数据
    LoadScenesFromJSON("data/scenes.json");

    // 初始化游戏状态
    game.state = STATE_TITLE;
    game.current_scene = GetSceneByID("scene1");  // 假设第一幕ID为 scene1
    game.dialogue_index = 0;

    // 默认设置
    game.master_volume = 0.8f;
    game.text_speed = 30;  // 每帧等待？简化：点击才推进，所以暂时没用
}

void UpdateGame(void) {
    switch (game.state) {
        case STATE_TITLE:
            HandleTitleInput();
            break;
        case STATE_PLAYING:
            HandlePlayingInput();
            break;
        case STATE_SETTINGS:
            HandleSettingsInput();
            break;
    }
}

void DrawGame(void) {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    switch (game.state) {
        case STATE_TITLE:
            DrawTitle();
            break;
        case STATE_PLAYING:
            DrawPlaying();
            break;
        case STATE_SETTINGS:
            DrawSettings();
            break;
    }

    EndDrawing();
}

void UnloadGame(void) {
    UnloadScenes();
    // 如果有加载纹理，在这里卸载
}

// --- 标题画面 ---
static void DrawTitle(void) {
    DrawText("No Way!", 200, 200, 40, BLACK);
    DrawText("Press Enter to Start", 220, 300, 20, DARKGRAY);
    DrawText("Press S for Settings", 220, 340, 20, DARKGRAY);
}

static void HandleTitleInput(void) {
    if (IsKeyPressed(KEY_ENTER)) {
        game.state = STATE_PLAYING;
        game.dialogue_index = 0;  // 从头开始
    }
    if (IsKeyPressed(KEY_S)) {
        game.state = STATE_SETTINGS;
    }
}

// --- 剧情播放 ---
static void DrawPlaying(void) {
    Scene *sc = game.current_scene;
    if (!sc) return;

    // 绘制背景（暂时用颜色代替，因为没有实际图片）
    DrawRectangle(0, 0, 800, 600, LIGHTGRAY);
    DrawText(TextFormat("Background: %s", sc->background ? sc->background : "none"), 20, 20, 10, BLACK);

    // 绘制当前对话
    if (game.dialogue_index < sc->dialogue_count) {
        Dialogue *d = &sc->dialogues[game.dialogue_index];
        // 绘制说话者
        DrawText(d->speaker, 50, 500, 20, MAROON);
        // 绘制对话内容（简单换行未处理，先直接显示）
        DrawText(d->text, 50, 530, 20, DARKGRAY);
    } else {
        // 对话结束，回到标题（或后续处理）
        DrawText("End of scene. Press ESC to title.", 200, 300, 20, BLACK);
    }

    // 提示操作
    DrawText("Left Click: Next dialogue | ESC: Title | S: Settings", 10, 570, 10, BLACK);
}

static void HandlePlayingInput(void) {
    // 点击鼠标左键或按空格推进对话
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE)) {
        game.dialogue_index++;
        // 如果对话结束，可以循环或返回标题（这里简单回到标题）
        if (game.dialogue_index >= game.current_scene->dialogue_count) {
            game.state = STATE_TITLE;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        game.state = STATE_TITLE;
    }

    if (IsKeyPressed(KEY_S)) {
        game.state = STATE_SETTINGS;
    }
}

// --- 设置界面 ---
static void DrawSettings(void) {
    DrawText("Settings", 350, 100, 30, BLACK);

    // 显示当前音量
    DrawText(TextFormat("Master Volume: %.0f%%", game.master_volume * 100), 200, 200, 20, BLACK);
    // 简单的滑动条模拟（按左右键调整）
    DrawRectangle(200, 230, 400, 20, LIGHTGRAY);
    DrawRectangle(200, 230, (int)(400 * game.master_volume), 20, BLUE);

    // 显示文字速度（用 1-10 表示，简单起见用整数）
    DrawText(TextFormat("Text Speed: %d", game.text_speed), 200, 280, 20, BLACK);
    DrawRectangle(200, 310, 400, 20, LIGHTGRAY);
    DrawRectangle(200, 310, (int)(400 * game.text_speed / 60.0f), 20, GREEN); // 假设最大60

    DrawText("Left/Right: adjust volume | Up/Down: adjust speed", 150, 400, 15, BLACK);
    DrawText("Press B to go back", 300, 450, 20, DARKGRAY);
}

static void HandleSettingsInput(void) {
    // 调整音量
    if (IsKeyPressed(KEY_RIGHT)) {
        game.master_volume += 0.05f;
        if (game.master_volume > 1.0f) game.master_volume = 1.0f;
    }
    if (IsKeyPressed(KEY_LEFT)) {
        game.master_volume -= 0.05f;
        if (game.master_volume < 0.0f) game.master_volume = 0.0f;
    }

    // 调整文字速度（这里简单用 1-60 表示，实际可以关联到延迟）
    if (IsKeyPressed(KEY_UP)) {
        game.text_speed += 5;
        if (game.text_speed > 60) game.text_speed = 60;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        game.text_speed -= 5;
        if (game.text_speed < 5) game.text_speed = 5;
    }

    // 返回上一级（标题）
    if (IsKeyPressed(KEY_B)) {
        game.state = STATE_TITLE;
    }
}