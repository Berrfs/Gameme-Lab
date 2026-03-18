/* game.c — Core game logic: state machine, rendering, and input handling.
   (核心游戏逻辑：状态机、渲染和输入处理。)
   Manages title screen, name input, story playback, choices, and settings.
   (管理标题画面、姓名输入、剧情播放、选项和设置。)
   Code updated by Louis, at 09:24PM 2026/03/18 */

#include "game.h"
#include "raylib.h"
#include "minigame.h"
#include <stdio.h>
#include <string.h>   /* For strcpy, strcmp, strlen (用于 strcpy、strcmp、strlen) */

GameContext game;

/* --- Forward declarations for private (static) functions (私有静态函数的前置声明) --- */
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

/* --- Utility: check if a texture-based button was clicked this frame (检查纹理按钮在这一帧是否被点击) --- */
static bool IsButtonClicked(Texture2D tex, int posX, int posY, float scale) {
    int btnWidth = (int)(tex.width * scale);
    int btnHeight = (int)(tex.height * scale);
    Rectangle btnRect = { (float)posX, (float)posY, (float)btnWidth, (float)btnHeight };
    return (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), btnRect));
}

/* --- Utility: check if the mouse is hovering over a texture-based button (检查鼠标是否悬停在纹理按钮上) --- */
static bool IsButtonHovered(Texture2D tex, int posX, int posY, float scale) {
    int btnWidth = (int)(tex.width * scale);
    int btnHeight = (int)(tex.height * scale);
    Rectangle btnRect = { (float)posX, (float)posY, (float)btnWidth, (float)btnHeight };
    return CheckCollisionPointRec(GetMousePosition(), btnRect);
}

/* ========== Initialization (初始化) ========== */
void InitGame(void) {
    /* Load scene data from the JSON script file (从 JSON 脚本文件中加载场景数据) */
    LoadScenesFromJSON("data/scenes.json");

    /* Set the starting game state (设置初始游戏状态) */
    game.state = STATE_TITLE;
    game.current_scene = GetSceneByID("scene1");
    game.dialogue_index = 0;

    /* Default settings (默认设置) */
    game.master_volume = 0.8f;
    game.auto_mode = false;
    game.auto_interval = 2.0f;
    game.auto_timer = 0.0f;

    /* Set a default player name (设置默认玩家名称) */
    strcpy(game.player_name, "Player");

    /* Load title screen textures (加载标题画面纹理) */
    game.titleBackground = LoadTexture("UI/red curtain.jpg");
    game.titleLogo = LoadTexture("UI/no way.png");
    game.gamemeLabLogo = LoadTexture("UI/Gameme Lab.png");
    game.btnStart = LoadTexture("UI/touch to start.png");
    game.btnMenu = LoadTexture("UI/menu.png");
    game.btnExit = LoadTexture("UI/exit.png");

    game.forestBackground = LoadTexture("UI/forest.jpg");   
    game.computerImage    = LoadTexture("UI/computer.jpg"); 
    // Initialize background/portrait cache (初始化背景/立绘缓存)
    game.currentBackground = (Texture2D){0};
    game.currentPortrait = (Texture2D){0};
    game.currentBackgroundPath[0] = '\0';
    game.currentSpeaker[0] = '\0';
}

/* ========== Per-Frame Update Dispatch (每帧更新调度) ========== */
void UpdateGame(void) {
    switch (game.state) {
        case STATE_TITLE:       HandleTitleInput(); break;
        case STATE_NAME_INPUT:  HandleNameInput(); break;
        case STATE_PLAYING:     UpdatePlaying(); break;
        case STATE_CHOICE:      HandleChoiceInput(); break;
        case STATE_SETTINGS:    HandleSettingsInput(); break;
        case STATE_MINIGAME:    UpdateMinigame(); break; // Added: minigame state (新增：小游戏状态)
    }
}

/* ========== Per-Frame Render Dispatch (每帧渲染调度) ========== */
void DrawGame(void) {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    switch (game.state) {
        case STATE_TITLE:       DrawTitle(); break;
        case STATE_NAME_INPUT:  DrawNameInput(); break;
        case STATE_PLAYING:     DrawPlaying(); break;
        case STATE_CHOICE:      DrawChoice(); break;
        case STATE_SETTINGS:    DrawSettings(); break;
        case STATE_MINIGAME:    DrawMinigame(); break; // Added: minigame state (新增：小游戏状态)
    }

    EndDrawing();
}

/* ========== Resource Cleanup (资源清理) ========== */
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

    // Ensure minigame resources are released (确保小游戏资源被释放)
    UnloadMinigame();   // Declared in minigame.h (声明在 minigame.h 中)
}

/* ==================== Title Screen Drawing (标题画面绘制) ==================== */
static void DrawTitle(void) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    /* Draw the full-screen background image (绘制全屏背景图) */
    DrawTexturePro(game.titleBackground,
        (Rectangle){0, 0, (float)game.titleBackground.width, (float)game.titleBackground.height},
        (Rectangle){0, 0, (float)screenWidth, (float)screenHeight},
        (Vector2){0, 0}, 0.0f, WHITE);

    /* 1) Left side — large "NO WAY!" logo, scaled down for 720p (左侧——大"NO WAY!"标志，缩小以适配720p) */
    float logoScale = 0.30f;
    int logoW = (int)(game.titleLogo.width * logoScale);
    int logoH = (int)(game.titleLogo.height * logoScale);
    int logoX = 30;
    int logoY = (screenHeight - logoH) / 2;
    DrawTextureEx(game.titleLogo, (Vector2){ (float)logoX, (float)logoY }, 0.0f, logoScale, WHITE);

    /* 2) Top-right — "Gameme Lab" icon (右上角——"Gameme Lab"图标) */
    float glScale = 0.55f;
    int glW = (int)(game.gamemeLabLogo.width * glScale);
    int glX = screenWidth - glW - 30;
    int glY = 20;
    DrawTextureEx(game.gamemeLabLogo, (Vector2){ (float)glX, (float)glY }, 0.0f, glScale, WHITE);

    int rightAreaCenterX = screenWidth * 3 / 4;

    /* Track if any button is hovered for cursor change (跟踪是否有按钮被悬停以切换光标) */
    bool anyHovered = false;

    /* 3) "Touch to start" button — scales up slightly on hover ("开始"按钮——悬停时略微放大) */
    float startScale = 0.28f;
    int startW = (int)(game.btnStart.width * startScale);
    int startH = (int)(game.btnStart.height * startScale);
    int startX = rightAreaCenterX - startW / 2;
    int startY = (int)(screenHeight * 0.40f);
    bool startHov = IsButtonHovered(game.btnStart, startX, startY, startScale);
    if (startHov) anyHovered = true;
    if (startHov) {
        float hs = startScale * 1.08f;
        int hx = startX - (int)((game.btnStart.width * hs - startW) / 2);
        int hy = startY - (int)((game.btnStart.height * hs - startH) / 2);
        DrawTextureEx(game.btnStart, (Vector2){ (float)hx, (float)hy }, 0.0f, hs, WHITE);
    } else {
        DrawTextureEx(game.btnStart, (Vector2){ (float)startX, (float)startY }, 0.0f, startScale, WHITE);
    }

    /* 4) "Menu" button — scales up slightly on hover ("菜单"按钮——悬停时略微放大) */
    float menuScale = 0.08f;
    int menuW = (int)(game.btnMenu.width * menuScale);
    int menuH = (int)(game.btnMenu.height * menuScale);
    int menuX = rightAreaCenterX - menuW / 2;
    int menuY = startY + startH + 20;
    bool menuHov = IsButtonHovered(game.btnMenu, menuX, menuY, menuScale);
    if (menuHov) anyHovered = true;
    if (menuHov) {
        float hs = menuScale * 1.08f;
        int hx = menuX - (int)((game.btnMenu.width * hs - menuW) / 2);
        int hy = menuY - (int)((game.btnMenu.height * hs - menuH) / 2);
        DrawTextureEx(game.btnMenu, (Vector2){ (float)hx, (float)hy }, 0.0f, hs, WHITE);
    } else {
        DrawTextureEx(game.btnMenu, (Vector2){ (float)menuX, (float)menuY }, 0.0f, menuScale, WHITE);
    }

    /* 5) "Exit" button — scales up slightly on hover ("退出"按钮——悬停时略微放大) */
    float exitScale = 0.08f;
    int exitW = (int)(game.btnExit.width * exitScale);
    int exitH = (int)(game.btnExit.height * exitScale);
    int exitX = rightAreaCenterX - exitW / 2;
    int exitY = menuY + menuH + 20;
    bool exitHov = IsButtonHovered(game.btnExit, exitX, exitY, exitScale);
    if (exitHov) anyHovered = true;
    if (exitHov) {
        float hs = exitScale * 1.08f;
        int hx = exitX - (int)((game.btnExit.width * hs - exitW) / 2);
        int hy = exitY - (int)((game.btnExit.height * hs - exitH) / 2);
        DrawTextureEx(game.btnExit, (Vector2){ (float)hx, (float)hy }, 0.0f, hs, WHITE);
    } else {
        DrawTextureEx(game.btnExit, (Vector2){ (float)exitX, (float)exitY }, 0.0f, exitScale, WHITE);
    }

    /* Change mouse cursor to hand pointer when hovering any button (悬停按钮时将鼠标光标改为手型指针) */
    SetMouseCursor(anyHovered ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
}

/* ==================== Title Screen Input (标题画面输入) ==================== */
static void HandleTitleInput(void) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    int rightAreaCenterX = screenWidth * 3 / 4;

    /* Recalculate button positions, must match DrawTitle (重新计算按钮位置，必须与DrawTitle一致) */
    float startScale = 0.28f;
    int startW = (int)(game.btnStart.width * startScale);
    int startH = (int)(game.btnStart.height * startScale);
    int startX = rightAreaCenterX - startW / 2;
    int startY = (int)(screenHeight * 0.40f);

    float menuScale = 0.08f;
    int menuW = (int)(game.btnMenu.width * menuScale);
    int menuH = (int)(game.btnMenu.height * menuScale);
    int menuX = rightAreaCenterX - menuW / 2;
    int menuY = startY + startH + 20;

    float exitScale = 0.08f;
    int exitW = (int)(game.btnExit.width * exitScale);
    int exitH = (int)(game.btnExit.height * exitScale);
    int exitX = rightAreaCenterX - exitW / 2;
    int exitY = menuY + menuH + 20;

    /* Click "Touch to start" → go to name input screen (点击"开始"→ 进入姓名输入界面) */
    if (IsButtonClicked(game.btnStart, startX, startY, startScale)) {
        game.state = STATE_NAME_INPUT;
        strcpy(game.player_name, "");   /* Clear so the user types fresh (清空以便用户重新输入) */
    }
    /* Click "Menu" → open settings (点击"菜单"→ 打开设置) */
    if (IsButtonClicked(game.btnMenu, menuX, menuY, menuScale)) {
        game.state = STATE_SETTINGS;
    }
    /* Click "Exit" → close the window (点击"退出"→ 关闭窗口) */
    if (IsButtonClicked(game.btnExit, exitX, exitY, exitScale)) {
        CloseWindow();
    }
    /* Keyboard shortcuts (键盘快捷键) */
    if (IsKeyPressed(KEY_ENTER)) {
        game.state = STATE_NAME_INPUT;
        strcpy(game.player_name, "");
    }
    if (IsKeyPressed(KEY_S)) game.state = STATE_SETTINGS;
}

/* ==================== Name Input Screen (姓名输入界面) ==================== */

/* Draw the name input UI overlay (绘制姓名输入界面) */
static void DrawNameInput(void) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    /* Draw the computer image as full-screen background (将电脑图片作为全屏背景绘制) */
    DrawTexturePro(game.computerImage,
        (Rectangle){0, 0, (float)game.computerImage.width, (float)game.computerImage.height},
        (Rectangle){0, 0, (float)screenWidth, (float)screenHeight},
        (Vector2){0, 0}, 0.0f, WHITE);

    /* Input box area, centered on screen (输入框区域，居中于屏幕) */
    int boxWidth = 500;
    int boxHeight = 60;
    int boxX = (screenWidth - boxWidth) / 2;
    int boxY = (screenHeight - boxHeight) / 2;

    /* Input box background, semi-transparent black (输入框背景，半透明黑色) */
    DrawRectangle(boxX, boxY, boxWidth, boxHeight, (Color){ 0, 0, 0, 200 });

    /* Prompt text above the input box (输入框上方的提示文字) */
    const char *prompt = "Please enter your name (max 20 chars):";
    int promptFontSize = 24;
    int promptWidth = MeasureText(prompt, promptFontSize);
    int promptX = (screenWidth - promptWidth) / 2;
    int promptY = boxY - 40;
    DrawText(prompt, promptX, promptY, promptFontSize, WHITE);

    /* Player name text inside the input box (输入框内的玩家名称文本) */
    int fontSize = 32;
    int textWidth = MeasureText(game.player_name, fontSize);
    int textX = boxX + 15;
    int textY = boxY + (boxHeight - fontSize) / 2;
    DrawText(game.player_name, textX, textY, fontSize, WHITE);

    /* Blinking text cursor (闪烁的文本光标) */
    if (((int)(GetTime() * 2) % 2) == 0) {
        int caretX = textX + textWidth;
        DrawRectangle(caretX, textY, 3, fontSize, YELLOW);
    }

    /* Confirmation hint below the input box (输入框下方的确认提示) */
    const char *hint = "Press ENTER to confirm";
    int hintFontSize = 20;
    int hintWidth = MeasureText(hint, hintFontSize);
    int hintX = (screenWidth - hintWidth) / 2;
    int hintY = boxY + boxHeight + 15;
    DrawText(hint, hintX, hintY, hintFontSize, YELLOW);
}

   
/* Handle keyboard input for the name entry screen (处理姓名输入界面的键盘输入) */
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

    // Press ESC to return to title (按 ESC 返回标题)
    if (IsKeyPressed(KEY_ESCAPE)) {
        game.state = STATE_TITLE;
    }
}
   
                
/* ==================== Story Playback (剧情播放) ==================== */

/* Draw the current dialogue line or end-of-scene message (绘制当前对话行或场景结束提示) */
static void DrawPlaying(void) {
    Scene *sc = game.current_scene;
    if (!sc) return;

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    /* Draw scene background, full screen (绘制场景背景，全屏) */
    if (game.currentBackground.id != 0) {
        DrawTexturePro(game.currentBackground,
            (Rectangle){0, 0, (float)game.currentBackground.width, (float)game.currentBackground.height},
            (Rectangle){0, 0, (float)screenWidth, (float)screenHeight},
            (Vector2){0, 0}, 0.0f, WHITE);
    } else {
        DrawRectangle(0, 0, screenWidth, screenHeight, LIGHTGRAY);
    }

    /* Draw character portrait, left side, aligned above dialogue box (绘制角色立绘，左侧，对齐对话框上方) */
    int dialogBoxHeight = 150;
    int dialogBoxY = screenHeight - dialogBoxHeight - 20;
    if (game.currentPortrait.id != 0) {
        float portraitHeight = screenHeight * 0.35f;
        float scale = portraitHeight / game.currentPortrait.height;
        float portraitW = game.currentPortrait.width * scale;
        float portraitX = screenWidth - portraitW - 20;
        float portraitY = dialogBoxY - portraitHeight + 10;
        DrawTextureEx(game.currentPortrait, (Vector2){portraitX, portraitY}, 0.0f, scale, WHITE);
    }

    /* Draw bottom dialogue box, semi-transparent (绘制底部半透明对话框) */
    DrawRectangle(20, dialogBoxY, screenWidth - 40, dialogBoxHeight, (Color){0, 0, 0, 180});

    if (game.dialogue_index < sc->dialogue_count) {
        Dialogue *d = &sc->dialogues[game.dialogue_index];

        /* Speaker name, replace "Player" with actual player name (说话者名称，将"Player"替换为实际玩家名) */
        const char *speaker = d->speaker;
        if (strcmp(speaker, "Player") == 0) {
            speaker = game.player_name;
        }
        DrawText(speaker, 40, dialogBoxY + 12, 28, MAROON);
        DrawText(d->text, 40, dialogBoxY + 50, 26, WHITE);
    } else {
        int msgWidth = MeasureText("End of scene. Press ESC to title.", 30);
        DrawText("End of scene. Press ESC to title.", (screenWidth - msgWidth) / 2, screenHeight / 2, 30, BLACK);
    }

    /* Show current mode, auto or manual (显示当前模式，自动或手动) */
    DrawText(TextFormat("Mode: %s", game.auto_mode ? "AUTO" : "MANUAL"), 15, 10, 20, BLACK);
}


/* Advance dialogue automatically or on click, and handle scene transitions (自动或点击推进对话，并处理场景过渡) */
static void UpdatePlaying(void) {
    Scene *sc = game.current_scene;
    if (!sc) return;

    /* ---------- Background loading (背景加载) ---------- */
    if (sc->background) {
        if (strcmp(game.currentBackgroundPath, sc->background) != 0) {
            // Unload old background (卸载旧背景)
            if (game.currentBackground.id != 0) {
                UnloadTexture(game.currentBackground);
                game.currentBackground = (Texture2D){0};
            }
            // Load new background, assumes images are in UI/ directory (加载新背景，假设图片放在 UI/ 目录下)
            char path[256];
            snprintf(path, sizeof(path), "UI/%s", sc->background);
            game.currentBackground = LoadTexture(path);
            if (game.currentBackground.id == 0) {
                TraceLog(LOG_WARNING, "Failed to load background: %s", path);
            }
            strcpy(game.currentBackgroundPath, sc->background);
        }
    } else {
        // Scene has no background specified, unload existing (场景未指定背景，卸载现有背景)
        if (game.currentBackground.id != 0) {
            UnloadTexture(game.currentBackground);
            game.currentBackground = (Texture2D){0};
        }
        game.currentBackgroundPath[0] = '\0';
    }

    /* ---------- Portrait loading (立绘加载) ---------- */
    if (game.dialogue_index < sc->dialogue_count) {
        Dialogue *d = &sc->dialogues[game.dialogue_index];
        const char *speaker = d->speaker;
        if (speaker && speaker[0] != '\0') {
            if (strcmp(game.currentSpeaker, speaker) != 0) {
                // Unload old portrait (卸载旧立绘)
                if (game.currentPortrait.id != 0) {
                    UnloadTexture(game.currentPortrait);
                    game.currentPortrait = (Texture2D){0};
                }
                // Load new portrait, filename based on speaker name (加载新立绘，文件名基于说话者名称)
                char path[256];
                snprintf(path, sizeof(path), "UI/%s.png", speaker);
                game.currentPortrait = LoadTexture(path);
                if (game.currentPortrait.id == 0) {
                    TraceLog(LOG_WARNING, "Failed to load portrait: %s", path);
                }
                strcpy(game.currentSpeaker, speaker);
            }
        } else {
            // Current dialogue has no speaker, unload portrait (当前对话无说话者，卸载立绘)
            if (game.currentPortrait.id != 0) {
                UnloadTexture(game.currentPortrait);
                game.currentPortrait = (Texture2D){0};
            }
            game.currentSpeaker[0] = '\0';
        }
    } else {
        // No dialogue content, unload portrait (无对话内容，卸载立绘)
        if (game.currentPortrait.id != 0) {
            UnloadTexture(game.currentPortrait);
            game.currentPortrait = (Texture2D){0};
        }
        game.currentSpeaker[0] = '\0';
    }

    /* ---------- Auto/manual advance logic (自动/手动推进逻辑) ---------- */
    if (game.auto_mode) {
        /* Auto-advance: accumulate time and advance when interval is reached (自动推进：累计时间，达到间隔时推进) */
        game.auto_timer += GetFrameTime();
        if (game.auto_timer >= game.auto_interval) {
            game.auto_timer = 0.0f;
            game.dialogue_index++;
            if (game.dialogue_index >= sc->dialogue_count) {
                if (sc->choice_count > 0) {
                    game.state = STATE_CHOICE;
                } else {
                    // Added: if specific scene, enter minigame (新增：如果是特定场景，进入小游戏)
                    if (strcmp(sc->id, "scene2") == 0) {
                        UnloadMinigame();      // Clean up old resources, safe even if uninitialized (清理旧资源，即使未初始化也安全)
                        InitMinigame();          // Initialize minigame resources (初始化小游戏资源)
                        game.state = STATE_MINIGAME;
                    } else {
                        game.state = STATE_TITLE;
                    }
                }
            }
        }
    } else {
        /* Manual advance: click or press SPACE to go to next line (手动推进：点击或按空格键进入下一行) */
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE)) {
            game.dialogue_index++;
            if (game.dialogue_index >= sc->dialogue_count) {
                if (sc->choice_count > 0) {
                    game.state = STATE_CHOICE;
                } else {
                    // Added: if specific scene, enter minigame (新增：如果是特定场景，进入小游戏)
                    if (strcmp(sc->id, "scene2") == 0) {
                        UnloadMinigame();      // Clean up old resources, safe even if uninitialized (清理旧资源，即使未初始化也安全)
                        InitMinigame();          // Initialize minigame resources (初始化小游戏资源)
                        game.state = STATE_MINIGAME;
                    } else {
                        game.state = STATE_TITLE;
                    }
                }
            }
        }
    }
    // Global shortcuts (全局快捷键)
    if (IsKeyPressed(KEY_ESCAPE)) game.state = STATE_TITLE;
    if (IsKeyPressed(KEY_S)) game.state = STATE_SETTINGS;
}


/* ==================== Choice Overlay (选项覆盖层) ==================== */

/* Draw available branching choices on a dark overlay (在暗色覆盖层上绘制可用的分支选项) */
static void DrawChoice(void) {
    Scene *sc = game.current_scene;
    if (!sc) return;

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    /* Draw the scene background behind the overlay (在覆盖层后面绘制场景背景) */
    if (game.currentBackground.id != 0) {
        DrawTexturePro(game.currentBackground,
            (Rectangle){0, 0, (float)game.currentBackground.width, (float)game.currentBackground.height},
            (Rectangle){0, 0, (float)screenWidth, (float)screenHeight},
            (Vector2){0, 0}, 0.0f, WHITE);
    }

    /* Draw character portrait if available (如果有立绘则绘制) */
    if (game.currentPortrait.id != 0) {
        float portraitHeight = screenHeight * 0.30f;
        float scale = portraitHeight / game.currentPortrait.height;
        float portraitW = game.currentPortrait.width * scale;
        float portraitX = screenWidth - portraitW - 25;
        float portraitY = screenHeight - portraitHeight - 160;
        DrawTextureEx(game.currentPortrait, (Vector2){portraitX, portraitY}, 0.0f, scale, WHITE);
    }

    /* Semi-transparent dark overlay (半透明暗色覆盖层) */
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 170});

    int startX = screenWidth / 4;
    int startY = (int)(screenHeight * 0.30f);
    int rowGap = 55;
    int panelPadding = 20;
    int panelHeight = sc->choice_count * rowGap + 60;

    /* Draw a semi-transparent panel behind choices for readability (在选项后绘制半透明面板以增加可读性) */
    DrawRectangle(startX - panelPadding, startY - panelPadding,
                  screenWidth / 2 + panelPadding * 2, panelHeight,
                  (Color){20, 20, 30, 180});
    DrawRectangleLinesEx(
        (Rectangle){(float)(startX - panelPadding), (float)(startY - panelPadding),
                     (float)(screenWidth / 2 + panelPadding * 2), (float)panelHeight},
        2, (Color){255, 255, 255, 60});

    Vector2 mouse = GetMousePosition();
    bool anyHovered = false;

    for (int i = 0; i < sc->choice_count; i++) {
        int choiceY = startY + i * rowGap;
        Rectangle choiceRect = { (float)startX, (float)(choiceY - 5), (float)(screenWidth / 2), 40 };
        bool hovered = CheckCollisionPointRec(mouse, choiceRect);
        if (hovered) anyHovered = true;

        /* Highlight bar behind hovered choice (悬停选项的高亮条) */
        if (hovered) {
            DrawRectangle(startX - 10, choiceY - 5, screenWidth / 2 + 20, 40, (Color){255, 200, 50, 40});
        }
        Color color = hovered ? YELLOW : WHITE;
        DrawText(TextFormat("%d. %s", i+1, sc->choices[i].text), startX, choiceY, 32, color);
    }
    DrawText("Press number key to choose", startX, startY + sc->choice_count * rowGap + 5, 22, GRAY);

    /* Change cursor to hand pointer when hovering a choice (悬停选项时将光标改为手型指针) */
    SetMouseCursor(anyHovered ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
}

/* Handle choice selection via keyboard 1-9 or mouse click (通过键盘1-9或鼠标点击处理选项选择) */
static void HandleChoiceInput(void) {
    Scene *sc = game.current_scene;
    if (!sc) return;

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    int startX = screenWidth / 4;
    int startY = (int)(screenHeight * 0.30f);
    int rowGap = 55;

    /* Keyboard: press number keys 1–9 to pick a choice (键盘：按数字键1-9选择选项) */
    for (int i = 0; i < sc->choice_count; i++) {
        if (IsKeyPressed(KEY_ONE + i)) {
            SelectChoice(i);
            return;
        }
    }

    /* Mouse: click within the bounding box of a choice (鼠标：在选项边界框内点击) */
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse = GetMousePosition();
        for (int i = 0; i < sc->choice_count; i++) {
            Rectangle rect = { (float)startX, (float)(startY + i * rowGap - 5), (float)(screenWidth / 2), 40 };
            if (CheckCollisionPointRec(mouse, rect)) {
                SelectChoice(i);
                return;
            }
        }
    }
}

/* Transition to the scene pointed to by the selected choice (过渡到所选选项指向的场景) */
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
        /* Target scene not found — fall back to title (目标场景未找到——回退到标题画面) */
        game.state = STATE_TITLE;
    }
}

/* ==================== Settings Screen (设置界面) ==================== */

/* Draw the settings UI: volume slider, auto-mode toggle, interval slider (绘制设置界面：音量滑块、自动模式切换、间隔滑块) */
static void DrawSettings(void) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    int sliderW = 500;
    int leftX = (screenWidth - sliderW) / 2;
    int baseY = 80;

    /* Title (标题) */
    int titleW = MeasureText("Settings", 48);
    DrawText("Settings", (screenWidth - titleW) / 2, baseY, 48, BLACK);

    /* Master volume slider (主音量滑块) */
    DrawText(TextFormat("Master Volume: %.0f%%", game.master_volume * 100), leftX, baseY + 100, 28, BLACK);
    DrawRectangle(leftX, baseY + 140, sliderW, 30, LIGHTGRAY);
    DrawRectangle(leftX, baseY + 140, (int)(sliderW * game.master_volume), 30, BLUE);

    /* Auto-mode on/off indicator (自动模式开关指示器) */
    DrawText(TextFormat("Auto Mode: %s", game.auto_mode ? "ON" : "OFF"), leftX, baseY + 210, 28, BLACK);
    DrawRectangle(leftX, baseY + 250, sliderW, 30, LIGHTGRAY);
    DrawRectangle(leftX, baseY + 250, (int)(sliderW * (game.auto_mode ? 1 : 0)), 30, PURPLE);
    DrawText("Press M to toggle Auto/Manual", leftX, baseY + 290, 22, BLACK);

    /* Auto-advance interval slider (自动推进间隔滑块) */
    DrawText(TextFormat("Auto Interval: %.1f s", game.auto_interval), leftX, baseY + 350, 28, BLACK);
    DrawRectangle(leftX, baseY + 390, sliderW, 30, LIGHTGRAY);
    DrawRectangle(leftX, baseY + 390, (int)(sliderW * (game.auto_interval / 5.0f)), 30, ORANGE);
    DrawText("Up/Down: adjust interval (0.5~5.0s)", leftX, baseY + 430, 22, BLACK);

    /* Back button hint (返回按钮提示) */
    int backW = MeasureText("Press B to go back", 28);
    DrawText("Press B to go back", (screenWidth - backW) / 2, screenHeight - 60, 28, DARKGRAY);
}

/* Handle settings input: volume, auto-mode toggle, interval adjustment (处理设置输入：音量、自动模式切换、间隔调整) */
static void HandleSettingsInput(void) {
    /* Volume control, Left/Right arrow keys (音量控制，左/右方向键) */
    if (IsKeyPressed(KEY_RIGHT)) {
        game.master_volume += 0.05f;
        if (game.master_volume > 1.0f) game.master_volume = 1.0f;
    }
    if (IsKeyPressed(KEY_LEFT)) {
        game.master_volume -= 0.05f;
        if (game.master_volume < 0.0f) game.master_volume = 0.0f;
    }

    /* Toggle auto-advance mode (切换自动推进模式) */
    if (IsKeyPressed(KEY_M)) {
        game.auto_mode = !game.auto_mode;
    }

    /* Auto-advance interval, Up/Down arrow keys, range 0.5–5.0s (自动推进间隔，上/下方向键，范围 0.5-5.0秒) */
    if (IsKeyPressed(KEY_UP)) {
        game.auto_interval += 0.2f;
        if (game.auto_interval > 5.0f) game.auto_interval = 5.0f;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        game.auto_interval -= 0.2f;
        if (game.auto_interval < 0.5f) game.auto_interval = 0.5f;
    }

    /* Press B to return to the title screen (按 B 返回标题画面) */
    if (IsKeyPressed(KEY_B)) game.state = STATE_TITLE;
}
