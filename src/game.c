#include "game.h"
#include "raylib.h"
#include <stdio.h>

GameContext game;

static void DrawTitle(void);
static void DrawPlaying(void);
static void DrawSettings(void);
static void HandleTitleInput(void);
static void HandlePlayingInput(void);
static void HandleSettingsInput(void);

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

void InitGame(void) {
    // Load data scene dari JSON
    LoadScenesFromJSON("data/scenes.json");

    // Inisialisasi state game
    game.state = STATE_TITLE;
    game.current_scene = GetSceneByID("scene1");
    game.dialogue_index = 0;

    // Setting default
    game.master_volume = 0.8f;
    game.text_speed = 30;

    // Load semua texture UI untuk title screen
    game.titleBackground = LoadTexture("UI/red curtain.jpg");
    game.titleLogo = LoadTexture("UI/no way.png");
    game.gamemeLabLogo = LoadTexture("UI/Gameme Lab.png");
    game.btnStart = LoadTexture("UI/touch to start.png");
    game.btnMenu = LoadTexture("UI/menu.png");
    game.btnExit = LoadTexture("UI/exit.png");
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

    // Unload semua texture UI
    UnloadTexture(game.titleBackground);
    UnloadTexture(game.titleLogo);
    UnloadTexture(game.gamemeLabLogo);
    UnloadTexture(game.btnStart);
    UnloadTexture(game.btnMenu);
    UnloadTexture(game.btnExit);
}

// --- TITLE SCREEN ---
static void DrawTitle(void) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // 1. Background merah (stretch fullscreen)
    DrawTexturePro(
        game.titleBackground,
        (Rectangle){ 0, 0, (float)game.titleBackground.width, (float)game.titleBackground.height },
        (Rectangle){ 0, 0, (float)screenWidth, (float)screenHeight },
        (Vector2){ 0, 0 }, 0.0f, WHITE
    );

    // 2. Logo "NO WAY!" papan kayu — kiri tengah (~50% lebar layar)
    //    PNG asli: 2048x1828, target ~410px lebar
    float logoScale = 0.20f;
    int logoW = (int)(game.titleLogo.width * logoScale);   // ~410px
    int logoH = (int)(game.titleLogo.height * logoScale);  // ~366px
    int logoX = 10;
    int logoY = (screenHeight - logoH) / 2 - 10;
    DrawTextureEx(game.titleLogo, (Vector2){ (float)logoX, (float)logoY }, 0.0f, logoScale, WHITE);

    // 3. Logo "GamemeLab" — kanan atas
    //    PNG asli: 450x134, target ~180px lebar
    float glScale = 0.40f;
    int glW = (int)(game.gamemeLabLogo.width * glScale);
    int glX = screenWidth - glW - 25;
    int glY = 15;
    DrawTextureEx(game.gamemeLabLogo, (Vector2){ (float)glX, (float)glY }, 0.0f, glScale, WHITE);

    // Area kanan untuk tombol-tombol (center di area kanan layar)
    int rightAreaCenterX = screenWidth * 3 / 4;  // titik tengah area kanan

    // 4. Tombol "Touch to start" — kanan, di bawah GamemeLab
    //    PNG asli: 1279x325, target ~250px lebar
    float startScale = 0.195f;
    int startW = (int)(game.btnStart.width * startScale);   // ~250px
    int startH = (int)(game.btnStart.height * startScale);  // ~63px
    int startX = rightAreaCenterX - startW / 2;
    int startY = 220;
    if (IsButtonHovered(game.btnStart, startX, startY, startScale)) {
        float hs = startScale * 1.05f;
        int hx = startX - (int)((game.btnStart.width * hs - startW) / 2);
        int hy = startY - (int)((game.btnStart.height * hs - startH) / 2);
        DrawTextureEx(game.btnStart, (Vector2){ (float)hx, (float)hy }, 0.0f, hs, WHITE);
    } else {
        DrawTextureEx(game.btnStart, (Vector2){ (float)startX, (float)startY }, 0.0f, startScale, WHITE);
    }

    // 5. Tombol "Menu" — center kanan, di bawah touch to start
    //    PNG asli: 2730x1535, target ~150px lebar
    float menuScale = 0.055f;
    int menuW = (int)(game.btnMenu.width * menuScale);   // ~150px
    int menuH = (int)(game.btnMenu.height * menuScale);  // ~84px
    int menuX = rightAreaCenterX - menuW / 2;
    int menuY = startY + startH + 20;
    if (IsButtonHovered(game.btnMenu, menuX, menuY, menuScale)) {
        float hs = menuScale * 1.05f;
        int hx = menuX - (int)((game.btnMenu.width * hs - menuW) / 2);
        int hy = menuY - (int)((game.btnMenu.height * hs - menuH) / 2);
        DrawTextureEx(game.btnMenu, (Vector2){ (float)hx, (float)hy }, 0.0f, hs, WHITE);
    } else {
        DrawTextureEx(game.btnMenu, (Vector2){ (float)menuX, (float)menuY }, 0.0f, menuScale, WHITE);
    }

    // 6. Tombol "Exit" — center kanan, di bawah Menu
    //    PNG asli: 2598x1098, target ~143px lebar
    float exitScale = 0.055f;
    int exitW = (int)(game.btnExit.width * exitScale);   // ~143px
    int exitH = (int)(game.btnExit.height * exitScale);  // ~60px
    int exitX = rightAreaCenterX - exitW / 2;
    int exitY = menuY + menuH + 15;
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

    // Posisi & skala tombol (HARUS SAMA dengan DrawTitle)
    float startScale = 0.195f;
    int startW = (int)(game.btnStart.width * startScale);
    int startH = (int)(game.btnStart.height * startScale);
    int startX = rightAreaCenterX - startW / 2;
    int startY = 220;

    float menuScale = 0.055f;
    int menuW = (int)(game.btnMenu.width * menuScale);
    int menuH = (int)(game.btnMenu.height * menuScale);
    int menuX = rightAreaCenterX - menuW / 2;
    int menuY = startY + startH + 20;

    float exitScale = 0.055f;
    int exitW = (int)(game.btnExit.width * exitScale);
    int exitX = rightAreaCenterX - exitW / 2;
    int exitY = menuY + menuH + 15;

    // Klik "Touch to start" -> mulai game
    if (IsButtonClicked(game.btnStart, startX, startY, startScale)) {
        game.state = STATE_PLAYING;
        game.dialogue_index = 0;
    }

    // Klik "Menu" -> ke settings
    if (IsButtonClicked(game.btnMenu, menuX, menuY, menuScale)) {
        game.state = STATE_SETTINGS;
    }

    // Klik "Exit" -> tutup window
    if (IsButtonClicked(game.btnExit, exitX, exitY, exitScale)) {
        CloseWindow();
    }

    // Keyboard shortcut tetap bisa dipakai
    if (IsKeyPressed(KEY_ENTER)) {
        game.state = STATE_PLAYING;
        game.dialogue_index = 0;
    }
    if (IsKeyPressed(KEY_S)) {
        game.state = STATE_SETTINGS;
    }
}

// --- PLAYING (cerita berjalan) ---
static void DrawPlaying(void) {
    Scene *sc = game.current_scene;
    if (!sc) return;

    // Gambar background (sementara pakai warna)
    DrawRectangle(0, 0, 800, 600, LIGHTGRAY);
    DrawText(TextFormat("Background: %s", sc->background ? sc->background : "none"), 20, 20, 10, BLACK);

    // Gambar dialog saat ini
    if (game.dialogue_index < sc->dialogue_count) {
        Dialogue *d = &sc->dialogues[game.dialogue_index];
        // Nama pembicara
        DrawText(d->speaker, 50, 500, 20, MAROON);
        // Isi dialog
        DrawText(d->text, 50, 530, 20, DARKGRAY);
    } else {
        // Dialog habis
        DrawText("End of scene. Press ESC to title.", 200, 300, 20, BLACK);
    }

    // Petunjuk kontrol
    DrawText("Left Click: Next dialogue | ESC: Title | S: Settings", 10, 570, 10, BLACK);
}

static void HandlePlayingInput(void) {
    // Klik mouse kiri atau spasi = lanjut dialog
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE)) {
        game.dialogue_index++;
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

// --- SETTINGS ---
static void DrawSettings(void) {
    DrawText("Settings", 350, 100, 30, BLACK);

    // Volume
    DrawText(TextFormat("Master Volume: %.0f%%", game.master_volume * 100), 200, 200, 20, BLACK);
    DrawRectangle(200, 230, 400, 20, LIGHTGRAY);
    DrawRectangle(200, 230, (int)(400 * game.master_volume), 20, BLUE);

    // Text speed
    DrawText(TextFormat("Text Speed: %d", game.text_speed), 200, 280, 20, BLACK);
    DrawRectangle(200, 310, 400, 20, LIGHTGRAY);
    DrawRectangle(200, 310, (int)(400 * game.text_speed / 60.0f), 20, GREEN);

    DrawText("Left/Right: adjust volume | Up/Down: adjust speed", 150, 400, 15, BLACK);
    DrawText("Press B to go back", 300, 450, 20, DARKGRAY);
}

static void HandleSettingsInput(void) {
    // Atur volume
    if (IsKeyPressed(KEY_RIGHT)) {
        game.master_volume += 0.05f;
        if (game.master_volume > 1.0f) game.master_volume = 1.0f;
    }
    if (IsKeyPressed(KEY_LEFT)) {
        game.master_volume -= 0.05f;
        if (game.master_volume < 0.0f) game.master_volume = 0.0f;
    }

    // Atur text speed
    if (IsKeyPressed(KEY_UP)) {
        game.text_speed += 5;
        if (game.text_speed > 60) game.text_speed = 60;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        game.text_speed -= 5;
        if (game.text_speed < 5) game.text_speed = 5;
    }

    // Kembali ke title
    if (IsKeyPressed(KEY_B)) {
        game.state = STATE_TITLE;
    }
}