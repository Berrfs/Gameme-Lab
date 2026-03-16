/* game.c — Core game logic: state machine, rendering, and input handling.
   Manages title screen, name input, story playback, choices, and settings.
   Code updated by 周沐格, at 05:12PM 2026/03/14 */

#include "game.h"
#include "raylib.h"
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
}

/* ========== Per-Frame Update Dispatch ========== */
void UpdateGame(void) {
    switch (game.state) {
        case STATE_TITLE:       HandleTitleInput(); break;
        case STATE_NAME_INPUT:  HandleNameInput(); break;
        case STATE_PLAYING:     UpdatePlaying(); break;
        case STATE_CHOICE:      HandleChoiceInput(); break;
        case STATE_SETTINGS:    HandleSettingsInput(); break;
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

    /* Dark background overlay */
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 50, 50, 50, 255 });

    /* Prompt text */
    const char *prompt = "Please enter your name (English only, max 20 chars):";
    DrawText(prompt, screenWidth/2 - MeasureText(prompt, 40)/2, screenHeight/2 - 100, 40, WHITE);

    /* Input box */
    int boxWidth = 600;
    int boxHeight = 60;
    int boxX = screenWidth/2 - boxWidth/2;
    int boxY = screenHeight/2 - boxHeight/2;
    DrawRectangle(boxX, boxY, boxWidth, boxHeight, LIGHTGRAY);

    /* Current typed name */
    int fontSize = 40;
    int textWidth = MeasureText(game.player_name, fontSize);
    DrawText(game.player_name, boxX + 10, boxY + 10, fontSize, BLACK);

    /* Blinking cursor (toggles every 0.5 seconds) */
    if (((int)(GetTime() * 2) % 2) == 0) {
        int caretX = boxX + 10 + textWidth;
        DrawRectangle(caretX, boxY + 10, 3, fontSize, BLACK);
    }

    /* Hint text below the box */
    DrawText("Press ENTER to confirm", screenWidth/2 - 150, boxY + 80, 20, GRAY);
}

/* Handle keyboard input for the name entry screen */
static void HandleNameInput(void) {
    /* Read typed characters from the OS input queue */
    int key = GetCharPressed();
    while (key > 0) {
        /* Only allow English letters (A-Z, a-z) and space */
        if ((key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z') || key == ' ') {
            int len = strlen(game.player_name);
            if (len < 20) {
                game.player_name[len] = (char)key;
                game.player_name[len+1] = '\0';
            }
        }
        key = GetCharPressed();
    }

    /* Backspace — delete the last character */
    if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = strlen(game.player_name);
        if (len > 0) {
            game.player_name[len-1] = '\0';
        }
    }

    /* Enter — confirm name and start the game */
    if (IsKeyPressed(KEY_ENTER)) {
        /* Fall back to "Player" if the field is empty */
        if (strlen(game.player_name) == 0) {
            strcpy(game.player_name, "Player");
        }
        /* Transition to the story playback state */
        game.state = STATE_PLAYING;
        game.dialogue_index = 0;
        game.auto_timer = 0.0f;
    }
}

/* ==================== Story Playback ==================== */

/* Draw the current dialogue line (or end-of-scene message) */
static void DrawPlaying(void) {
    Scene *sc = game.current_scene;
    if (!sc) return;

    /* Temporary placeholder background (solid color) */
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), LIGHTGRAY);
    DrawText(TextFormat("Background: %s", sc->background ? sc->background : "none"), 40, 40, 20, BLACK);

    if (game.dialogue_index < sc->dialogue_count) {
        Dialogue *d = &sc->dialogues[game.dialogue_index];
        /* If the speaker tag is "Player", substitute with the actual player name */
        const char *speaker = d->speaker;
        if (strcmp(speaker, "Player") == 0) {
            speaker = game.player_name;
        }
        DrawText(speaker, 100, 1000, 40, MAROON);
        DrawText(d->text, 100, 1060, 40, DARKGRAY);
    } else {
        DrawText("End of scene. Press ESC to title.", 400, 600, 40, BLACK);
    }

    /* Display current advance mode (AUTO / MANUAL) */
    DrawText(TextFormat("Mode: %s", game.auto_mode ? "AUTO" : "MANUAL"), 20, 20, 30, BLACK);
}

/* Advance dialogue automatically or on click, and handle scene transitions */
static void UpdatePlaying(void) {
    Scene *sc = game.current_scene;
    if (!sc) return;

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
                    game.state = STATE_TITLE;
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
                    game.state = STATE_TITLE;
                }
            }
        }
    }

    /* Global shortcuts available during playback */
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