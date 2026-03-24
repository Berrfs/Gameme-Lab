/* game.c — Core game logic: state machine, rendering, and input handling.
   (核心游戏逻辑：状态机、渲染和输入处理。)
   Manages title screen, name input, story playback, choices, and settings.
   (管理标题画面、姓名输入、剧情播放、选项和设置。)
   Code updated by 周沐格, at 10:24PM 2026/03/24 */

#include "game.h"
#include "raylib.h"
#include "minigame.h"
#include "minigame2.h"
#include "cJSON/cJSON.h"
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
static void LoadSettings(void);
static void SaveSettings(void);
static void HandleFullscreenToggle(void);

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

/* --- Utility: calculate UI scale factor based on current screen resolution (根据当前屏幕分辨率计算UI缩放因子) --- */
static float GetUIScale(void) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    float scaleX = screenWidth / (float)BASE_SCREEN_WIDTH;
    float scaleY = screenHeight / (float)BASE_SCREEN_HEIGHT;
    /* Use the minimum scale to maintain aspect ratio (使用最小缩放以保持宽高比) */
    return scaleX < scaleY ? scaleX : scaleY;
}

/* ========== Initialization (初始化) ========== */
void InitGame(void) {
    /* Load scene data from the JSON script file (从 JSON 脚本文件中加载场景数据) */
    LoadScenesFromJSON("data/scenes.json");

    /* Load settings from JSON (从 JSON 加载设置) */
    LoadSettings();

    /* Set the starting game state (设置初始游戏状态) */
    game.state = STATE_TITLE;
    game.current_scene = GetSceneByID("scene1");
    game.dialogue_index = 0;

    /* Default settings if not loaded from file (如果未从文件加载，使用默认设置) */
    if (game.master_volume == 0.0f && game.auto_interval == 0.0f) {
        game.master_volume = 0.8f;
        game.auto_mode = false;
        game.auto_interval = 2.0f;
        game.auto_timer = 0.0f;
        game.fullscreen = false;
        game.window_width = 1280;
        game.window_height = 720;
    }

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
        case STATE_MINIGAME2:   UpdateMinigame2(); break;
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
        case STATE_MINIGAME2:   DrawMinigame2(); break;
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
    UnloadMinigame2();
}

/* ==================== Title Screen Drawing (标题画面绘制) ==================== */
static void DrawTitle(void) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    float uiScale = GetUIScale();

    /* Draw the full-screen background image (绘制全屏背景图) */
    DrawTexturePro(game.titleBackground,
        (Rectangle){0, 0, (float)game.titleBackground.width, (float)game.titleBackground.height},
        (Rectangle){0, 0, (float)screenWidth, (float)screenHeight},
        (Vector2){0, 0}, 0.0f, WHITE);

    /* 1) Left side — large "NO WAY!" logo, scaled down for 720p (左侧——大"NO WAY!"标志，缩小以适配720p) */
    float logoScale = 0.30f * uiScale;
    int logoW = (int)(game.titleLogo.width * logoScale);
    int logoH = (int)(game.titleLogo.height * logoScale);
    int logoX = (int)(30 * uiScale);
    int logoY = (screenHeight - logoH) / 2;
    DrawTextureEx(game.titleLogo, (Vector2){ (float)logoX, (float)logoY }, 0.0f, logoScale, WHITE);

    /* 2) Top-right — "Gameme Lab" icon (右上角——"Gameme Lab"图标) */
    float glScale = 0.55f * uiScale;
    int glW = (int)(game.gamemeLabLogo.width * glScale);
    int glX = screenWidth - glW - (int)(30 * uiScale);
    int glY = (int)(20 * uiScale);
    DrawTextureEx(game.gamemeLabLogo, (Vector2){ (float)glX, (float)glY }, 0.0f, glScale, WHITE);

    int rightAreaCenterX = screenWidth * 3 / 4;

    /* Track if any button is hovered for cursor change (跟踪是否有按钮被悬停以切换光标) */
    bool anyHovered = false;

    /* 3) "Touch to start" button — scales up slightly on hover ("开始"按钮——悬停时略微放大) */
    float startScale = 0.28f * uiScale;
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
    float menuScale = 0.08f * uiScale;
    int menuW = (int)(game.btnMenu.width * menuScale);
    int menuH = (int)(game.btnMenu.height * menuScale);
    int menuX = rightAreaCenterX - menuW / 2;
    int menuY = startY + startH + (int)(20 * uiScale);
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
    float exitScale = 0.08f * uiScale;
    int exitW = (int)(game.btnExit.width * exitScale);
    int exitH = (int)(game.btnExit.height * exitScale);
    int exitX = rightAreaCenterX - exitW / 2;
    int exitY = menuY + menuH + (int)(20 * uiScale);
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
    float uiScale = GetUIScale();
    int rightAreaCenterX = screenWidth * 3 / 4;

    /* Recalculate button positions, must match DrawTitle (重新计算按钮位置，必须与DrawTitle一致) */
    float startScale = 0.28f * uiScale;
    int startW = (int)(game.btnStart.width * startScale);
    int startH = (int)(game.btnStart.height * startScale);
    int startX = rightAreaCenterX - startW / 2;
    int startY = (int)(screenHeight * 0.40f);

    float menuScale = 0.08f * uiScale;
    int menuW = (int)(game.btnMenu.width * menuScale);
    int menuH = (int)(game.btnMenu.height * menuScale);
    int menuX = rightAreaCenterX - menuW / 2;
    int menuY = startY + startH + (int)(20 * uiScale);

    float exitScale = 0.08f * uiScale;
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
    float uiScale = GetUIScale();

    /* Draw the computer image as full-screen background (将电脑图片作为全屏背景绘制) */
    DrawTexturePro(game.computerImage,
        (Rectangle){0, 0, (float)game.computerImage.width, (float)game.computerImage.height},
        (Rectangle){0, 0, (float)screenWidth, (float)screenHeight},
        (Vector2){0, 0}, 0.0f, WHITE);

    /* Input box area, centered on screen (输入框区域，居中于屏幕) */
    int boxWidth = (int)(500 * uiScale);
    int boxHeight = (int)(60 * uiScale);
    int boxX = (screenWidth - boxWidth) / 2;
    int boxY = (screenHeight - boxHeight) / 2;

    /* Input box background, semi-transparent black (输入框背景，半透明黑色) */
    DrawRectangle(boxX, boxY, boxWidth, boxHeight, (Color){ 0, 0, 0, 200 });

    /* Prompt text above the input box (输入框上方的提示文字) */
    const char *prompt = "Please enter your name (max 20 chars):";
    int promptFontSize = (int)(24 * uiScale);
    int promptWidth = MeasureText(prompt, promptFontSize);
    int promptX = (screenWidth - promptWidth) / 2;
    int promptY = boxY - (int)(40 * uiScale);
    DrawText(prompt, promptX, promptY, promptFontSize, WHITE);

    /* Player name text inside the input box (输入框内的玩家名称文本) */
    int fontSize = (int)(32 * uiScale);
    int textWidth = MeasureText(game.player_name, fontSize);
    int textX = boxX + (int)(15 * uiScale);
    int textY = boxY + (boxHeight - fontSize) / 2;
    DrawText(game.player_name, textX, textY, fontSize, WHITE);

    /* Blinking text cursor (闪烁的文本光标) */
    if (((int)(GetTime() * 2) % 2) == 0) {
        int caretX = textX + textWidth;
        DrawRectangle(caretX, textY, (int)(3 * uiScale), fontSize, YELLOW);
    }

    /* Confirmation hint below the input box (输入框下方的确认提示) */
    const char *hint = "Press ENTER to confirm";
    int hintFontSize = (int)(20 * uiScale);
    int hintWidth = MeasureText(hint, hintFontSize);
    int hintX = (screenWidth - hintWidth) / 2;
    int hintY = boxY + boxHeight + (int)(15 * uiScale);
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
    float uiScale = GetUIScale();

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
    int dialogBoxHeight = (int)(150 * uiScale);
    int dialogBoxY = screenHeight - dialogBoxHeight - (int)(20 * uiScale);
    if (game.currentPortrait.id != 0) {
        float portraitHeight = screenHeight * 0.35f;
        float scale = portraitHeight / game.currentPortrait.height;
        float portraitW = game.currentPortrait.width * scale;
        float portraitX = screenWidth - portraitW - (int)(20 * uiScale);
        float portraitY = dialogBoxY - portraitHeight + (int)(10 * uiScale);
        DrawTextureEx(game.currentPortrait, (Vector2){portraitX, portraitY}, 0.0f, scale, WHITE);
    }

    /* Draw bottom dialogue box, semi-transparent (绘制底部半透明对话框) */
    DrawRectangle((int)(20 * uiScale), dialogBoxY, screenWidth - (int)(40 * uiScale), dialogBoxHeight, (Color){0, 0, 0, 180});

    if (game.dialogue_index < sc->dialogue_count) {
        Dialogue *d = &sc->dialogues[game.dialogue_index];

        /* Speaker name, replace "Player" with actual player name (说话者名称，将"Player"替换为实际玩家名) */
        const char *speaker = d->speaker;
        if (strcmp(speaker, "Player") == 0) {
            speaker = game.player_name;
        }
        DrawText(speaker, (int)(40 * uiScale), dialogBoxY + (int)(12 * uiScale), (int)(28 * uiScale), MAROON);
        DrawText(d->text, (int)(40 * uiScale), dialogBoxY + (int)(50 * uiScale), (int)(26 * uiScale), WHITE);
    } else {
        int msgWidth = MeasureText("End of scene. Press ESC to title.", (int)(30 * uiScale));
        DrawText("End of scene. Press ESC to title.", (screenWidth - msgWidth) / 2, screenHeight / 2, (int)(30 * uiScale), BLACK);
    }

    /* Show current mode, auto or manual (显示当前模式，自动或手动) */
    DrawText(TextFormat("Mode: %s", game.auto_mode ? "AUTO" : "MANUAL"), (int)(15 * uiScale), (int)(10 * uiScale), (int)(20 * uiScale), BLACK);
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
                    // 检查是否配置了下一场景的ID（注意这里判空用 != NULL）
                    if (sc->next_scene_id != NULL) {
                        
                        // 特殊情况：进入小游戏
                        if (strcmp(sc->next_scene_id, "MINIGAME") == 0) {
                            UnloadMinigame();
                            InitMinigame();
                            game.state = STATE_MINIGAME;
                        } 
                        else if (strcmp(sc->next_scene_id, "MINIGAME2") == 0) {
                            UnloadMinigame2();
                            InitMinigame2();
                            game.state = STATE_MINIGAME2;
                        }
                        // 正常情况：跳往下一个场景
                        else {
                            Scene *next = GetSceneByID(sc->next_scene_id);
                            if (next) {
                                game.current_scene = next;
                                game.dialogue_index = 0;
                                game.auto_timer = 0.0f;
                            } else {
                                game.state = STATE_TITLE;
                            }
                        }
                    } 
                    // 如果什么都没配置，退回标题 (比如最终结局)
                    else {
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
                    // 检查是否配置了下一场景的ID（注意这里判空用 != NULL）
                    if (sc->next_scene_id != NULL) {
                        
                        // 特殊情况：进入小游戏
                        if (strcmp(sc->next_scene_id, "MINIGAME") == 0) {
                            UnloadMinigame();
                            InitMinigame();
                            game.state = STATE_MINIGAME;
                        } 
                        else if (strcmp(sc->next_scene_id, "MINIGAME2") == 0) {
                            UnloadMinigame2();
                            InitMinigame2();
                            game.state = STATE_MINIGAME2;
                        }
                        // 正常情况：跳往下一个场景
                        else {
                            Scene *next = GetSceneByID(sc->next_scene_id);
                            if (next) {
                                game.current_scene = next;
                                game.dialogue_index = 0;
                                game.auto_timer = 0.0f;
                            } else {
                                game.state = STATE_TITLE;
                            }
                        }
                    } 
                    // 如果什么都没配置，退回标题 (比如最终结局)
                    else {
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
    float uiScale = GetUIScale();

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
        float portraitX = screenWidth - portraitW - (int)(25 * uiScale);
        float portraitY = screenHeight - portraitHeight - (int)(160 * uiScale);
        DrawTextureEx(game.currentPortrait, (Vector2){portraitX, portraitY}, 0.0f, scale, WHITE);
    }

    /* Semi-transparent dark overlay (半透明暗色覆盖层) */
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 170});

    int startX = screenWidth / 4;
    int startY = (int)(screenHeight * 0.30f);
    int rowGap = (int)(55 * uiScale);
    int panelPadding = (int)(20 * uiScale);
    int panelHeight = sc->choice_count * rowGap + (int)(60 * uiScale);

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
        Rectangle choiceRect = { (float)startX, (float)(choiceY - 5), (float)(screenWidth / 2), (float)(40 * uiScale) };
        bool hovered = CheckCollisionPointRec(mouse, choiceRect);
        if (hovered) anyHovered = true;

        /* Highlight bar behind hovered choice (悬停选项的高亮条) */
        if (hovered) {
            DrawRectangle(startX - (int)(10 * uiScale), choiceY - 5, screenWidth / 2 + (int)(20 * uiScale), (int)(40 * uiScale), (Color){255, 200, 50, 40});
        }
        Color color = hovered ? YELLOW : WHITE;
        DrawText(TextFormat("%d. %s", i+1, sc->choices[i].text), startX, choiceY, (int)(32 * uiScale), color);
    }
    DrawText("Press number key to choose", startX, startY + sc->choice_count * rowGap + (int)(5 * uiScale), (int)(22 * uiScale), GRAY);

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
    float uiScale = GetUIScale();
    int sliderW = (int)(500 * uiScale);
    int leftX = (screenWidth - sliderW) / 2;

    /* Calculate total content height and vertically center it */
    int contentHeight = (int)(462 * uiScale);
    int baseY = (screenHeight - contentHeight) / 2;

    /* Title — centered horizontally */
    int titleW = MeasureText("Settings", (int)(42 * uiScale));
    DrawText("Settings", (screenWidth - titleW) / 2, baseY, (int)(42 * uiScale), BLACK);

    /* Master volume slider */
    DrawText(TextFormat("Master Volume: %.0f%%", game.master_volume * 100), leftX, baseY + (int)(70 * uiScale), (int)(26 * uiScale), BLACK);
    DrawRectangle(leftX, baseY + (int)(105 * uiScale), sliderW, (int)(25 * uiScale), LIGHTGRAY);
    DrawRectangle(leftX, baseY + (int)(105 * uiScale), (int)(sliderW * game.master_volume), (int)(25 * uiScale), BLUE);

    /* Auto-mode on/off indicator */
    DrawText(TextFormat("Auto Mode: %s", game.auto_mode ? "ON" : "OFF"), leftX, baseY + (int)(155 * uiScale), (int)(26 * uiScale), BLACK);
    DrawRectangle(leftX, baseY + (int)(190 * uiScale), sliderW, (int)(25 * uiScale), LIGHTGRAY);
    DrawRectangle(leftX, baseY + (int)(190 * uiScale), (int)(sliderW * (game.auto_mode ? 1 : 0)), (int)(25 * uiScale), PURPLE);
    DrawText("Press M to toggle Auto/Manual", leftX, baseY + (int)(222 * uiScale), (int)(20 * uiScale), BLACK);

    /* Auto-advance interval slider */
    DrawText(TextFormat("Auto Interval: %.1f s", game.auto_interval), leftX, baseY + (int)(265 * uiScale), (int)(26 * uiScale), BLACK);
    DrawRectangle(leftX, baseY + (int)(300 * uiScale), sliderW, (int)(25 * uiScale), LIGHTGRAY);
    DrawRectangle(leftX, baseY + (int)(300 * uiScale), (int)(sliderW * (game.auto_interval / 5.0f)), (int)(25 * uiScale), ORANGE);
    DrawText("Up/Down: adjust interval (0.5~5.0s)", leftX, baseY + (int)(332 * uiScale), (int)(20 * uiScale), BLACK);

    /* Fullscreen toggle indicator */
    DrawText(TextFormat("Fullscreen: %s", game.fullscreen ? "ON" : "OFF"), leftX, baseY + (int)(375 * uiScale), (int)(26 * uiScale), BLACK);
    DrawRectangle(leftX, baseY + (int)(410 * uiScale), sliderW, (int)(25 * uiScale), LIGHTGRAY);
    DrawRectangle(leftX, baseY + (int)(410 * uiScale), (int)(sliderW * (game.fullscreen ? 1 : 0)), (int)(25 * uiScale), GREEN);
    DrawText("Press F to toggle Fullscreen", leftX, baseY + (int)(442 * uiScale), (int)(20 * uiScale), BLACK);

    /* "Press B to go back" — pinned at bottom center */
    int backFontSize = (int)(26 * uiScale);
    int backW = MeasureText("Press B to go back", backFontSize);
    DrawText("Press B to go back", (screenWidth - backW) / 2, screenHeight - (int)(50 * uiScale), backFontSize, DARKGRAY);
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

    /* Toggle fullscreen mode (切换全屏模式) */
    if (IsKeyPressed(KEY_F)) {
        HandleFullscreenToggle();
    }

    /* Press B to return to the title screen (按 B 返回标题画面) */
    if (IsKeyPressed(KEY_B)) {
        SaveSettings();
        game.state = STATE_TITLE;
    }
}

/* ==================== Settings Persistence (设置持久化) ==================== */

/* Load settings from data/settings.json (从 data/settings.json 加载设置) */
static void LoadSettings(void) {
    char *json_str = LoadFileText("data/settings.json");
    if (!json_str) {
        TraceLog(LOG_WARNING, "Failed to load settings.json");
        return;
    }

    cJSON *root = cJSON_Parse(json_str);
    UnloadFileText(json_str);

    if (!root) {
        TraceLog(LOG_WARNING, "JSON parse error in settings.json");
        return;
    }

    /* Load volume setting (加载音量设置) */
    cJSON *volume = cJSON_GetObjectItem(root, "master_volume");
    if (volume && volume->type == cJSON_Number) {
        game.master_volume = (float)volume->valuedouble;
    }

    /* Load auto mode setting (加载自动模式设置) */
    cJSON *auto_mode = cJSON_GetObjectItem(root, "auto_mode");
    if (auto_mode && auto_mode->type == cJSON_True) {
        game.auto_mode = true;
    } else {
        game.auto_mode = false;
    }

    /* Load auto interval setting (加载自动间隔设置) */
    cJSON *auto_interval = cJSON_GetObjectItem(root, "auto_interval");
    if (auto_interval && auto_interval->type == cJSON_Number) {
        game.auto_interval = (float)auto_interval->valuedouble;
    }

    /* Load fullscreen setting (加载全屏设置) */
    cJSON *fullscreen = cJSON_GetObjectItem(root, "fullscreen");
    if (fullscreen && fullscreen->type == cJSON_True) {
        game.fullscreen = true;
    } else {
        game.fullscreen = false;
    }

    /* Load window dimensions (加载窗口尺寸) */
    cJSON *width = cJSON_GetObjectItem(root, "window_width");
    if (width && width->type == cJSON_Number) {
        game.window_width = width->valueint;
    } else {
        game.window_width = 1280;
    }

    cJSON *height = cJSON_GetObjectItem(root, "window_height");
    if (height && height->type == cJSON_Number) {
        game.window_height = height->valueint;
    } else {
        game.window_height = 720;
    }

    cJSON_Delete(root);
    TraceLog(LOG_INFO, "Settings loaded successfully");
}

/* Save settings to data/settings.json (保存设置到 data/settings.json) */
static void SaveSettings(void) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return;

    /* Save all current settings (保存所有当前设置) */
    cJSON_AddNumberToObject(root, "master_volume", game.master_volume);
    cJSON_AddNumberToObject(root, "text_speed", 30);  /* Placeholder for future use (未来使用的占位符) */
    cJSON_AddBoolToObject(root, "auto_mode", game.auto_mode);
    cJSON_AddNumberToObject(root, "auto_interval", game.auto_interval);
    cJSON_AddBoolToObject(root, "fullscreen", game.fullscreen);
    cJSON_AddNumberToObject(root, "window_width", game.window_width);
    cJSON_AddNumberToObject(root, "window_height", game.window_height);

    /* Convert to JSON string and write to file (转换为 JSON 字符串并写入文件) */
    char *json_str = cJSON_Print(root);
    if (json_str) {
        FILE *file = fopen("data/settings.json", "w");
        if (file) {
            fprintf(file, "%s", json_str);
            fclose(file);
            TraceLog(LOG_INFO, "Settings saved successfully");
        }
        free(json_str);
    }

    cJSON_Delete(root);
}

/* Toggle fullscreen mode and update window (切换全屏模式并更新窗口) */
static void HandleFullscreenToggle(void) {
    game.fullscreen = !game.fullscreen;
    ToggleFullscreen();  /* raylib's built-in fullscreen toggle */
    SaveSettings();
    
    if (game.fullscreen) {
        TraceLog(LOG_INFO, "Switched to fullscreen mode");
    } else {
        TraceLog(LOG_INFO, "Switched to windowed mode");
    }
}
