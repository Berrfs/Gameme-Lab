/* game.c — Core game logic: state machine, rendering, and input handling.
   Manages title screen, name input, story playback, choices, and settings.
   Code updated by 周沐格, at 05:12PM 2026/03/14 */

#include "game.h"
#include "raylib.h"
#include "minigame.h"
#include <stdio.h>
#include <string.h>   /* For strcpy, strcmp, strlen */

GameContext game;

/* --- Forward declarations for private (static) functions --- */
static void DrawTitle(void);
static void DrawPlaying(void);
static void DrawChoice(void);
static void DrawSettings(void);
static void DrawNameInput(void);
static void HandleTitleInput(void);
static void UpdatePlaying(void);
static void HandleChoiceInput(void);
static void HandleSettingsInput(void);
static void HandleNameInput(void);
static void SelectChoice(int index);
static bool IsButtonClicked(Texture2D tex, int posX, int posY, float scale);
static bool IsButtonHovered(Texture2D tex, int posX, int posY, float scale);

/* --- Utility: check if a texture-based button was clicked this frame --- */
static bool IsButtonClicked(Texture2D tex, int posX, int posY, float scale) {
    int btnWidth = (int)(tex.width * scale);
    int btnHeight = (int)(tex.height * scale);
    Rectangle btnRect = { (float)posX, (float)posY, (float)btnWidth, (float)btnHeight };
    return (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), btnRect));
}

/* --- Utility: check if the mouse is hovering over a texture-based button --- */
static bool IsButtonHovered(Texture2D tex, int posX, int posY, float scale) {
    int btnWidth = (int)(tex.width * scale);
    int btnHeight = (int)(tex.height * scale);
    Rectangle btnRect = { (float)posX, (float)posY, (float)btnWidth, (float)btnHeight };
    return CheckCollisionPointRec(GetMousePosition(), btnRect);
}

/* ========== Initialization ========== */
void InitGame(void) {
    /* Load scene data from the JSON script file */
    LoadScenesFromJSON("data/scenes.json");

    /* Set the starting game state */
    game.state = STATE_TITLE;
    game.current_scene = GetSceneByID("scene1");
    game.dialogue_index = 0;

    /* Default settings */
    game.master_volume = 0.8f;
    game.auto_mode = false;
    game.auto_interval = 2.0f;
    game.auto_timer = 0.0f;

    /* Set a default player name */
    strcpy(game.player_name, "Player");

    /* Load title screen textures */
    game.titleBackground = LoadTexture("UI/red curtain.jpg");
    game.titleLogo = LoadTexture("UI/no way.png");
    game.gamemeLabLogo = LoadTexture("UI/Gameme Lab.png");
    game.btnStart = LoadTexture("UI/touch to start.png");
    game.btnMenu = LoadTexture("UI/menu.png");
    game.btnExit = LoadTexture("UI/exit.png");

    game.forestBackground = LoadTexture("UI/forest.jpg");   
    game.computerImage    = LoadTexture("UI/computer.jpg"); 
    // 初始化背景/立绘缓存
    game.currentBackground = (Texture2D){0};
    game.currentPortrait = (Texture2D){0};
    game.currentBackgroundPath[0] = '\0';
    game.currentSpeaker[0] = '\0';
}

/* ========== Per-Frame Update Dispatch ========== */
void UpdateGame(void) {
    switch (game.state) {
        case STATE_TITLE:       HandleTitleInput(); break;
        case STATE_NAME_INPUT:  HandleNameInput(); break;
        case STATE_PLAYING:     UpdatePlaying(); break;
        case STATE_CHOICE:      HandleChoiceInput(); break;
        case STATE_SETTINGS:    HandleSettingsInput(); break;
        case STATE_MINIGAME:    UpdateMinigame(); break; //新增，小游戏状态
    }
}

/* ========== Per-Frame Render Dispatch ========== */
void DrawGame(void) {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    switch (game.state) {
        case STATE_TITLE:       DrawTitle(); break;
        case STATE_NAME_INPUT:  DrawNameInput(); break;
        case STATE_PLAYING:     DrawPlaying(); break;
        case STATE_CHOICE:      DrawChoice(); break;
        case STATE_SETTINGS:    DrawSettings(); break;
        case STATE_MINIGAME:    DrawMinigame(); break; //新增，小游戏状态
    }

    EndDrawing();
}

/* ========== Resource Cleanup ========== */
void UnloadGame(void) {
    UnloadScenes();
    UnloadTexture(game.titleBackground);
    UnloadTexture(game.titleLogo);
    UnloadTexture(game.gamemeLabLogo);
    UnloadTexture(game.btnStart);
    UnloadTexture(game.btnMenu);
    UnloadTexture(game.btnExit);

    UnloadTexture(game.forestBackground);
    UnloadTexture(game.computerImage);

    if (game.currentBackground.id != 0) UnloadTexture(game.currentBackground);
    if (game.currentPortrait.id != 0) UnloadTexture(game.currentPortrait);

    // 确保小游戏资源被释放
    UnloadMinigame();   // 声明在 minigame.h 中
}

/* ==================== Title Screen Drawing ==================== */
static void DrawTitle(void) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    /* Draw the full-screen background image */
    DrawTexturePro(game.titleBackground,
        (Rectangle){0, 0, (float)game.titleBackground.width, (float)game.titleBackground.height},
        (Rectangle){0, 0, (float)screenWidth, (float)screenHeight},
        (Vector2){0, 0}, 0.0f, WHITE);

    /* 1) Left side — large "NO WAY!" logo */
    float logoScale = 0.40f;
    int logoW = (int)(game.titleLogo.width * logoScale);
    int logoH = (int)(game.titleLogo.height * logoScale);
    int logoX = 40;
    int logoY = (screenHeight - logoH) / 2;
    DrawTextureEx(game.titleLogo, (Vector2){ (float)logoX, (float)logoY }, 0.0f, logoScale, WHITE);

    /* 2) Top-right — "Gameme Lab" icon */
    float glScale = 0.80f;
    int glW = (int)(game.gamemeLabLogo.width * glScale);
    int glX = screenWidth - glW - 60;
    int glY = 40;
    DrawTextureEx(game.gamemeLabLogo, (Vector2){ (float)glX, (float)glY }, 0.0f, glScale, WHITE);

    int rightAreaCenterX = screenWidth * 3 / 4;

    /* 3) "Touch to start" button — scales up slightly on hover */
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

    /* 4) "Menu" button — scales up slightly on hover */
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

    /* 5) "Exit" button — scales up slightly on hover */
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

/* ==================== Title Screen Input ==================== */
static void HandleTitleInput(void) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    int rightAreaCenterX = screenWidth * 3 / 4;

    /* Recalculate button positions (must match DrawTitle) */
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

    /* Click "Touch to start" → go to name input screen */
    if (IsButtonClicked(game.btnStart, startX, startY, startScale)) {
        game.state = STATE_NAME_INPUT;
        strcpy(game.player_name, "");   /* Clear so the user types fresh */
    }
    /* Click "Menu" → open settings */
    if (IsButtonClicked(game.btnMenu, menuX, menuY, menuScale)) {
        game.state = STATE_SETTINGS;
    }
    /* Click "Exit" → close the window */
    if (IsButtonClicked(game.btnExit, exitX, exitY, exitScale)) {
        CloseWindow();
    }
    /* Keyboard shortcuts */
    if (IsKeyPressed(KEY_ENTER)) {
        game.state = STATE_NAME_INPUT;
        strcpy(game.player_name, "");
    }
    if (IsKeyPressed(KEY_S)) game.state = STATE_SETTINGS;
}

/* ==================== Name Input Screen ==================== */

/* Draw the name input UI overlay */
static void DrawNameInput(void) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // 绘制电脑图片（全屏居中，保持原比例）
    float computerScale = 1.0f;
    int compW = (int)(game.computerImage.width * computerScale);
    int compH = (int)(game.computerImage.height * computerScale);
    int compX = (screenWidth - compW) / 2;
    int compY = (screenHeight - compH) / 2;
    DrawTextureEx(game.computerImage, (Vector2){ (float)compX, (float)compY }, 0.0f, computerScale, WHITE);

    // 输入框区域（屏幕正中央）
    int boxWidth = 600;
    int boxHeight = 80;
    int boxX = (screenWidth - boxWidth) / 2;
    int boxY = (screenHeight - boxHeight) / 2;

    // 输入框背景（半透明黑）
    DrawRectangle(boxX, boxY, boxWidth, boxHeight, (Color){ 0, 0, 0, 200 });

    // 提示文字（输入框上方）
    const char *prompt = "Please enter your name (max 20 chars):";
    int promptFontSize = 30;
    int promptWidth = MeasureText(prompt, promptFontSize);
    int promptX = (screenWidth - promptWidth) / 2;
    int promptY = boxY - 50;
    DrawText(prompt, promptX, promptY, promptFontSize, WHITE);

    // 已输入姓名
    int fontSize = 40;
    int textWidth = MeasureText(game.player_name, fontSize);
    int textX = boxX + 20;
    int textY = boxY + (boxHeight - fontSize) / 2;
    DrawText(game.player_name, textX, textY, fontSize, WHITE);

    // 闪烁光标
    if (((int)(GetTime() * 2) % 2) == 0) {
        int caretX = textX + textWidth;
        DrawRectangle(caretX, textY, 4, fontSize, YELLOW);
    }

    // 确认提示（输入框下方）
    const char *hint = "Press ENTER to confirm";
    int hintFontSize = 24;
    int hintWidth = MeasureText(hint, hintFontSize);
    int hintX = (screenWidth - hintWidth) / 2;
    int hintY = boxY + boxHeight + 20;
    DrawText(hint, hintX, hintY, hintFontSize, YELLOW);
}

   
/* Handle keyboard input for the name entry screen */
static void HandleNameInput(void) {

    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z') || key == ' ') {
            int len = strlen(game.player_name);
            if (len < 20) {
                game.player_name[len] = (char)key;
                game.player_name[len+1] = '\0';
            }
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = strlen(game.player_name);
        if (len > 0) {
            game.player_name[len-1] = '\0';
        }
    }

    if (IsKeyPressed(KEY_ENTER)) {
        if (strlen(game.player_name) == 0) {
            strcpy(game.player_name, "Player");
        }
        game.state = STATE_PLAYING;
        game.dialogue_index = 0;
        game.auto_timer = 0.0f;
    }

    // 按 ESC 返回标题
    if (IsKeyPressed(KEY_ESCAPE)) {
        game.state = STATE_TITLE;
    }
}
   
                
/* ==================== Story Playback ==================== */

/* Draw the current dialogue line (or end-of-scene message) */
static void DrawPlaying(void) {
    Scene *sc = game.current_scene;
    if (!sc) return;

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    /* 绘制背景 */
    if (game.currentBackground.id != 0) {
        DrawTexturePro(game.currentBackground,
            (Rectangle){0, 0, (float)game.currentBackground.width, (float)game.currentBackground.height},
            (Rectangle){0, 0, (float)screenWidth, (float)screenHeight},
            (Vector2){0, 0}, 0.0f, WHITE);
    } else {
        DrawRectangle(0, 0, screenWidth, screenHeight, LIGHTGRAY);
    }

    /* 绘制立绘（左侧，高度占屏幕 60%） */
    if (game.currentPortrait.id != 0) {
        float portraitHeight = screenHeight * 0.2f;
        float scale = portraitHeight / game.currentPortrait.height;
        float portraitWidth = game.currentPortrait.width * scale;
        float portraitX = 50;
        float portraitY = (screenHeight - portraitHeight) / 2 + 250;
        DrawTextureEx(game.currentPortrait, (Vector2){portraitX, portraitY}, 0.0f, scale, WHITE);
    }

    /* 绘制底部半透明对话框 */
    int dialogBoxHeight = 200;
    int dialogBoxY = screenHeight - dialogBoxHeight - 30;
    DrawRectangle(30, dialogBoxY, screenWidth - 60, dialogBoxHeight, (Color){0, 0, 0, 180});

    if (game.dialogue_index < sc->dialogue_count) {
        Dialogue *d = &sc->dialogues[game.dialogue_index];

        // 说话者（若为 "Player" 则替换为玩家输入的名字）
        const char *speaker = d->speaker;
        /*if (strcmp(speaker, "Player") == 0) {
            speaker = game.player_name;
        }
        DrawText(speaker, 50, dialogBoxY + 10, 40, YELLOW);*/

        if (strcmp(speaker, "Player") == 0) {
            speaker = game.player_name;
        }
        DrawText(speaker, 100, 1000, 40, MAROON);
        DrawText(d->text, 100, 1060, 40, DARKGRAY);
        // 对话文本（自动换行）
       /* const char *text = d->text;
        Rectangle textRect = { 50, dialogBoxY + 60, screenWidth - 100, dialogBoxHeight - 80 };
        DrawTextRec(GetFontDefault(), text, textRect, 36, 2.0f, true, WHITE);*/
    } else {
        DrawText("End of scene. Press ESC to title.", 400, 600, 40, BLACK);
    }

    // 显示当前模式（自动/手动）
    DrawText(TextFormat("Mode: %s", game.auto_mode ? "AUTO" : "MANUAL"), 20, 20, 30, BLACK);
}


/* Advance dialogue automatically or on click, and handle scene transitions */
static void UpdatePlaying(void) {
    Scene *sc = game.current_scene;
    if (!sc) return;

    /* ---------- 背景加载 ---------- */
    if (sc->background) {
        if (strcmp(game.currentBackgroundPath, sc->background) != 0) {
            // 卸载旧背景
            if (game.currentBackground.id != 0) {
                UnloadTexture(game.currentBackground);
                game.currentBackground = (Texture2D){0};
            }
            // 加载新背景（假设背景图片放在 UI/ 目录下）
            char path[256];
            snprintf(path, sizeof(path), "UI/%s", sc->background);
            game.currentBackground = LoadTexture(path);
            if (game.currentBackground.id == 0) {
                TraceLog(LOG_WARNING, "Failed to load background: %s", path);
            }
            strcpy(game.currentBackgroundPath, sc->background);
        }
    } else {
        // 场景没有指定背景，卸载现有背景
        if (game.currentBackground.id != 0) {
            UnloadTexture(game.currentBackground);
            game.currentBackground = (Texture2D){0};
        }
        game.currentBackgroundPath[0] = '\0';
    }

    /* ---------- 立绘加载 ---------- */
    if (game.dialogue_index < sc->dialogue_count) {
        Dialogue *d = &sc->dialogues[game.dialogue_index];
        const char *speaker = d->speaker;
        if (speaker && speaker[0] != '\0') {
            if (strcmp(game.currentSpeaker, speaker) != 0) {
                // 卸载旧立绘
                if (game.currentPortrait.id != 0) {
                    UnloadTexture(game.currentPortrait);
                    game.currentPortrait = (Texture2D){0};
                }
                // 加载新立绘（假设立绘放在 UI/characters/ 目录下，文件名为 说话者.png）
                char path[256];
                snprintf(path, sizeof(path), "UI/%s.jpg", speaker);
                game.currentPortrait = LoadTexture(path);
                if (game.currentPortrait.id == 0) {
                    TraceLog(LOG_WARNING, "Failed to load portrait: %s", path);
                }
                strcpy(game.currentSpeaker, speaker);
            }
        } else {
            // 当前对话没有说话者，卸载立绘
            if (game.currentPortrait.id != 0) {
                UnloadTexture(game.currentPortrait);
                game.currentPortrait = (Texture2D){0};
            }
            game.currentSpeaker[0] = '\0';
        }
    } else {
        // 没有对话内容，卸载立绘
        if (game.currentPortrait.id != 0) {
            UnloadTexture(game.currentPortrait);
            game.currentPortrait = (Texture2D){0};
        }
        game.currentSpeaker[0] = '\0';
    }

    /* ---------- 原有的自动/手动推进逻辑 ---------- */
    if (game.auto_mode) {
        /* Auto-advance: accumulate time and advance when interval is reached */
        game.auto_timer += GetFrameTime();
        if (game.auto_timer >= game.auto_interval) {
            game.auto_timer = 0.0f;
            game.dialogue_index++;
            if (game.dialogue_index >= sc->dialogue_count) {
                if (sc->choice_count > 0) {
                    game.state = STATE_CHOICE;
                } else {
                    // 新增：如果是特定场景，进入小游戏
                    if (strcmp(sc->id, "scene2") == 0) {
                        UnloadMinigame();      // 清理旧资源（安全，即使未初始化）
                        InitMinigame();          // 初始化小游戏资源
                        game.state = STATE_MINIGAME;
                    } else {
                        game.state = STATE_TITLE;
                    }
                }
            }
        }
    } else {
        /* Manual advance: click or press SPACE to go to next line */
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE)) {
            game.dialogue_index++;
            if (game.dialogue_index >= sc->dialogue_count) {
                if (sc->choice_count > 0) {
                    game.state = STATE_CHOICE;
                } else {
                    // 新增：如果是特定场景，进入小游戏
                    if (strcmp(sc->id, "scene2") == 0) {
                        UnloadMinigame();      // 清理旧资源（安全，即使未初始化）
                        InitMinigame();          // 初始化小游戏资源
                        game.state = STATE_MINIGAME;
                    } else {
                        game.state = STATE_TITLE;
                    }
                }
            }
        }
    }
    // 全局快捷键
    if (IsKeyPressed(KEY_ESCAPE)) game.state = STATE_TITLE;
    if (IsKeyPressed(KEY_S)) game.state = STATE_SETTINGS;
}


/* ==================== Choice Overlay ==================== */

/* Draw available branching choices on a dark overlay */
static void DrawChoice(void) {
    Scene *sc = game.current_scene;
    if (!sc) return;

    /* Semi-transparent dark overlay */
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 150});

    int startY = 600;
    for (int i = 0; i < sc->choice_count; i++) {
        Color color = (i == 0) ? YELLOW : WHITE;
        DrawText(TextFormat("%d. %s", i+1, sc->choices[i].text), 400, startY + i*80, 60, color);
    }
    DrawText("Press number key to choose", 400, startY + sc->choice_count*80 + 40, 40, GRAY);
}

/* Handle choice selection via keyboard (1-9) or mouse click */
static void HandleChoiceInput(void) {
    Scene *sc = game.current_scene;
    if (!sc) return;

    /* Keyboard: press number keys 1–9 to pick a choice */
    for (int i = 0; i < sc->choice_count; i++) {
        if (IsKeyPressed(KEY_ONE + i)) {
            SelectChoice(i);
            return;
        }
    }

    /* Mouse: click within the rough bounding box of a choice */
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

/* Transition to the scene pointed to by the selected choice */
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
        /* Target scene not found — fall back to title */
        game.state = STATE_TITLE;
    }
}

/* ==================== Settings Screen ==================== */

/* Draw the settings UI: volume slider, auto-mode toggle, interval slider */
static void DrawSettings(void) {
    DrawText("Settings", 700, 200, 60, BLACK);

    /* Master volume slider */
    DrawText(TextFormat("Master Volume: %.0f%%", game.master_volume * 100), 400, 400, 40, BLACK);
    DrawRectangle(400, 460, 800, 40, LIGHTGRAY);
    DrawRectangle(400, 460, (int)(800 * game.master_volume), 40, BLUE);

    /* Auto-mode on/off indicator */
    DrawText(TextFormat("Auto Mode: %s", game.auto_mode ? "ON" : "OFF"), 400, 560, 40, BLACK);
    DrawRectangle(400, 620, 800, 40, LIGHTGRAY);
    DrawRectangle(400, 620, (int)(800 * (game.auto_mode ? 1 : 0)), 40, PURPLE);
    DrawText("Press M to toggle Auto/Manual", 400, 680, 30, BLACK);

    /* Auto-advance interval slider */
    DrawText(TextFormat("Auto Interval: %.1f s", game.auto_interval), 400, 740, 40, BLACK);
    DrawRectangle(400, 800, 800, 40, LIGHTGRAY);
    DrawRectangle(400, 800, (int)(800 * (game.auto_interval / 5.0f)), 40, ORANGE);
    DrawText("Up/Down: adjust interval (0.5~5.0s)", 400, 860, 30, BLACK);

    DrawText("Press B to go back", 600, 1000, 40, DARKGRAY);
}

/* Handle settings input: volume, auto-mode toggle, interval adjustment */
static void HandleSettingsInput(void) {
    /* Volume control (Left / Right arrow keys) */
    if (IsKeyPressed(KEY_RIGHT)) {
        game.master_volume += 0.05f;
        if (game.master_volume > 1.0f) game.master_volume = 1.0f;
    }
    if (IsKeyPressed(KEY_LEFT)) {
        game.master_volume -= 0.05f;
        if (game.master_volume < 0.0f) game.master_volume = 0.0f;
    }

    /* Toggle auto-advance mode */
    if (IsKeyPressed(KEY_M)) {
        game.auto_mode = !game.auto_mode;
    }

    /* Auto-advance interval (Up / Down arrow keys, range 0.5–5.0 s) */
    if (IsKeyPressed(KEY_UP)) {
        game.auto_interval += 0.2f;
        if (game.auto_interval > 5.0f) game.auto_interval = 5.0f;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        game.auto_interval -= 0.2f;
        if (game.auto_interval < 0.5f) game.auto_interval = 0.5f;
    }

    /* Press B to return to the title screen */
    if (IsKeyPressed(KEY_B)) game.state = STATE_TITLE;
}
