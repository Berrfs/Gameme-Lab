/* minigame2.c — Outdoor exploration minigame module.
   The player explores a small open-world area, interacts with objects
   (rabbit, grass, river, soil), grows a tree, and faces a QTE boss event.
   Uses WASD movement with sprite-sheet animation and an inventory toolbar.
   Code updated by Joan (周沐格), at 10:00PM 2026/03/24 */

#include "minigame2.h"
#include "game.h"
#include "scene.h"
#include "raylib.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

extern GameContext game;

/* Maximum number of queued dialogue lines for the paging system */
#define MAX_DIALOGUE_LINES 10

/* ---- Inventory constants & item IDs ---- */
#define MG2_MAX_INVENTORY 5       /* Number of toolbar slots */
#define ITEM_BUCKET_EMPTY 1       /* Empty iron bucket */
#define ITEM_BUCKET_WATER 2       /* Bucket filled with water */
#define ITEM_GRASS 3              /* Blade of grass dropped by rabbit */

/* Game-phase state machine for minigame 2 */
typedef enum {
    MG2_INTRO,          /* Opening dialogue — "Game" speaks and gives the bucket */
    MG2_PLAYING,        /* Free exploration — player can walk, interact, collect */
    MG2_GROWING,        /* Tree is growing — screen shake effect */
    MG2_BOSS_ANGRY,     /* "Game" is furious — triggers boss dialogue */
    MG2_QTE             /* Quick-Time Event — black hole + struggle button */
} MG2State;

/* Cardinal directions matching the sprite-sheet row order */
typedef enum { DIR_DOWN = 0, DIR_UP = 1, DIR_LEFT = 2, DIR_RIGHT = 3 } Direction;

/* Single dialogue line used in the paging dialogue queue */
typedef struct {
    char speaker[64];   /* Character name displayed above the text */
    char text[256];     /* Dialogue body text */
} DialogueLine;

/* ======== Internal state — file-scoped static struct ======== */
static struct {
    /* Textures */
    Texture2D bgMap;            /* Background map texture */
    Texture2D playerTex;        /* Player sprite-sheet (4×4 grid) */
    Texture2D treeTex;          /* Fully-grown tree texture */
    Texture2D holeTex;          /* Black-hole texture for QTE */

    /* Inventory toolbar textures */
    Texture2D inventorySlotTex;     /* Slot background image */
    Texture2D itemBucketEmptyTex;   /* Empty bucket icon */
    Texture2D itemBucketWaterTex;   /* Full bucket icon */
    Texture2D itemGrassTex;         /* Grass blade icon */

    int inventory[MG2_MAX_INVENTORY]; /* Item IDs held (-1 = empty) */
    int selectedSlot;                 /* Currently selected slot index (-1 = none) */

    /* Player movement & animation */
    Vector2 playerPos;      /* Current position in screen space */
    float speed;            /* Movement speed (pixels/sec) */
    Direction currentDir;   /* Facing direction (selects sprite row) */
    int currentFrame;       /* Current animation frame column (0-3) */
    float frameTimer;       /* Accumulator for frame advance timing */
    bool isMoving;          /* True while WASD keys are held */

    /* Interactive objects */
    Vector2 rabbitPos;      /* Rabbit spawn position */
    bool rabbitActive;      /* True until the player clicks the rabbit */
    bool grassBladeDropped; /* True once the rabbit leaves a grass blade */

    Vector2 grassPatchPos;  /* Grass/soil/tree patch position */
    bool isSoil;            /* True after grass has been tilled */
    bool hasTree;           /* True once the tree has been planted */

    Rectangle riverRect;    /* Clickable river area for filling the bucket */

    /* Game-phase & UI state */
    MG2State state;

    /* Paging dialogue system */
    DialogueLine dialogueQueue[MAX_DIALOGUE_LINES]; /* Queued lines */
    int dialogueCount;          /* Total queued lines */
    int currentDialogueIndex;   /* Index of the line being displayed */
    bool showDialogue;          /* True while dialogue box is visible */

    /* Top notification banner */
    float notificationTimer;    /* Seconds remaining for notification */
    char notificationText[128]; /* Text shown in the banner */

    /* QTE & screen-shake */
    float shakeTimer;           /* Remaining shake duration */
    float qteTimer;             /* Countdown for the QTE struggle */
} mg2 = {0};

/* ---- Forward declarations for private helpers ---- */
static void ShowDialog(const char* speaker, const char* text);
static void ShowNotification(const char* text);
static float GetDistance(Vector2 p1, Vector2 p2);
static void ExitMinigame2(const char* nextScene);
static void AddToInventory(int itemId);

/* ========== Initialization ========== */
void InitMinigame2(void) {
    /* Load world & character textures */
    mg2.bgMap = LoadTexture("UI/map.png");
    mg2.playerTex = LoadTexture("UI/player_sprite.png"); 
    mg2.treeTex = LoadTexture("UI/tree.png");
    mg2.holeTex = LoadTexture("UI/blackhole.png");

    /* Load inventory toolbar textures (fallback to colored rectangles if missing) */
    mg2.inventorySlotTex = LoadTexture("UI/slot.png");
    mg2.itemBucketEmptyTex = LoadTexture("UI/bucket_empty.png");
    mg2.itemBucketWaterTex = LoadTexture("UI/bucket_water.png");
    mg2.itemGrassTex = LoadTexture("UI/grass_blade.png");

    /* Clear all inventory slots */
    for (int i = 0; i < MG2_MAX_INVENTORY; i++) mg2.inventory[i] = -1;
    mg2.selectedSlot = -1;

    /* Place the player at screen center */
    mg2.playerPos = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    mg2.speed = 250.0f;
    mg2.currentDir = DIR_DOWN;
    mg2.currentFrame = 0;
    mg2.frameTimer = 0.0f;
    mg2.isMoving = false;

    /* Position interactive objects relative to window size */
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    mg2.rabbitPos = (Vector2){ sw * 0.2f, sh * 0.3f };
    mg2.rabbitActive = true;
    mg2.grassBladeDropped = false;

    mg2.grassPatchPos = (Vector2){ sw * 0.4f, sh * 0.6f };
    mg2.isSoil = false;
    mg2.hasTree = false;

    mg2.riverRect = (Rectangle){ sw * 0.7f, 0, sw * 0.3f, (float)sh };
    
    /* Set initial game phase and clear UI state */
    mg2.state = MG2_INTRO;
    mg2.showDialogue = false;
    mg2.dialogueCount = 0;        
    mg2.currentDialogueIndex = 0; 
    mg2.notificationTimer = 0.0f;
    mg2.shakeTimer = 0.0f;

    /* Queue opening dialogue lines (displayed page-by-page on click) */
    ShowDialog("Game", "Fine. You want to play a game? Then play.");
    ShowDialog("Game", "Enough. This little space is plenty for you to play with.");
    ShowDialog("Game", "If you're truly bored, go rustle through the grass or tease the rabbits.");
}

/* ========== Per-Frame Update ========== */
void UpdateMinigame2(void) {
    float dt = GetFrameTime();
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    Vector2 mouse = GetMousePosition();

    /* Tick down the notification timer */
    if (mg2.notificationTimer > 0) mg2.notificationTimer -= dt;

    /* --- Dialogue paging: intercept all input while dialogue is open --- */
    if (mg2.showDialogue) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            mg2.currentDialogueIndex++;
            if (mg2.currentDialogueIndex >= mg2.dialogueCount) {
                /* All pages read — close dialogue and advance phase */
                mg2.showDialogue = false; 
                MG2State finishedState = mg2.state;
                mg2.dialogueCount = 0;
                mg2.currentDialogueIndex = 0;

                if (finishedState == MG2_INTRO) {
                    /* After intro dialogue, give the player an empty bucket */
                    mg2.state = MG2_PLAYING;
                    AddToInventory(ITEM_BUCKET_EMPTY);
                    ShowNotification("Obtained: Iron Bucket x1");
                } else if (finishedState == MG2_BOSS_ANGRY) {
                    /* After boss rage dialogue, start QTE countdown */
                    mg2.state = MG2_QTE;
                    mg2.qteTimer = 3.0f; 
                }
            }
        }
        return; /* Block all other input while reading dialogue */
    }

    /* --- QTE phase: countdown + struggle button --- */
    if (mg2.state == MG2_QTE) {
        mg2.qteTimer -= dt;
        mg2.shakeTimer = 1.0f;     /* Keep screen shaking */
        Rectangle btnRect = { sw/2.0f - 100, sh/2.0f + 100, 200, 60 };
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, btnRect)) {
            ExitMinigame2("scene5");   /* Escaped! Proceed to scene5 */
        }
        if (mg2.qteTimer <= 0) {
            ExitMinigame2("ending1");  /* Failed — bad ending */
        }
        return;
    }

    /* --- Screen shake after tree growth --- */
    if (mg2.shakeTimer > 0) {
        mg2.shakeTimer -= dt;
        if (mg2.shakeTimer <= 0 && mg2.state == MG2_GROWING) {
            /* Shake ended → "Game" gets angry */
            mg2.state = MG2_BOSS_ANGRY;
            ShowDialog("Game", "Arrrrr! What are you doing in my game?");
            ShowDialog("Game", "Now take your ridiculous tree and get off my server!");
        }
    }

    /* --- Player WASD movement & animation --- */
    if (mg2.state == MG2_PLAYING || mg2.state == MG2_GROWING) {
        mg2.isMoving = false;
        Vector2 nextPos = mg2.playerPos;

        /* Read directional input */
        if (IsKeyDown(KEY_W)) { nextPos.y -= mg2.speed * dt; mg2.currentDir = DIR_UP; mg2.isMoving = true; }
        if (IsKeyDown(KEY_S)) { nextPos.y += mg2.speed * dt; mg2.currentDir = DIR_DOWN; mg2.isMoving = true; }
        if (IsKeyDown(KEY_A)) { nextPos.x -= mg2.speed * dt; mg2.currentDir = DIR_LEFT; mg2.isMoving = true; }
        if (IsKeyDown(KEY_D)) { nextPos.x += mg2.speed * dt; mg2.currentDir = DIR_RIGHT; mg2.isMoving = true; }

        /* Clamp to screen bounds (with small padding) */
        if (nextPos.x > 30 && nextPos.x < sw - 30) mg2.playerPos.x = nextPos.x;
        if (nextPos.y > 50 && nextPos.y < sh - 20) mg2.playerPos.y = nextPos.y;

        /* Advance walk animation frame at fixed interval */
        if (mg2.isMoving) {
            mg2.frameTimer += dt;
            if (mg2.frameTimer > 0.15f) {
                mg2.currentFrame = (mg2.currentFrame + 1) % 4;
                mg2.frameTimer = 0.0f;
            }
        } else {
            mg2.currentFrame = 0;  /* Reset to idle frame when stationary */
        }

        /* --- Inventory toolbar click handling --- */
        bool clickedUI = false;
        int slotSize = 80;
        int startX = 100;
        int slotY = sh - 120;
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            for (int slot = 0; slot < MG2_MAX_INVENTORY; slot++) {
                Rectangle slotRect = { startX + slot * (slotSize + 10), slotY, slotSize, slotSize };
                if (CheckCollisionPointRec(mouse, slotRect)) {
                    clickedUI = true;
                    /* Toggle slot selection */
                    if (mg2.inventory[slot] != -1) {
                        mg2.selectedSlot = (mg2.selectedSlot == slot) ? -1 : slot;
                    } else {
                        mg2.selectedSlot = -1;
                    }
                    break;
                }
            }
        }

        /* --- World object interaction (only when toolbar wasn't clicked) --- */
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !clickedUI) {
            float interactRange = 150.0f;

            /* 1. Rabbit — click to scare away, then pick up dropped grass */
            if (GetDistance(mg2.playerPos, mg2.rabbitPos) < interactRange) {
                if (mg2.rabbitActive) {
                    mg2.rabbitActive = false;
                    mg2.grassBladeDropped = true;
                    ShowNotification("The rabbit hopped away and left a blade of grass.");
                } else if (mg2.grassBladeDropped) {
                    mg2.grassBladeDropped = false;
                    AddToInventory(ITEM_GRASS);
                    ShowNotification("Obtained: Blade of Grass");
                }
            }

            /* 2. Grass/Soil — till the grass, then plant with water bucket */
            if (GetDistance(mg2.playerPos, mg2.grassPatchPos) < interactRange) {
                if (!mg2.isSoil) {
                    mg2.isSoil = true;
                    ShowNotification("Grass removed. It turned into tilled soil.");
                } else if (mg2.isSoil && !mg2.hasTree) {
                    /* Must have a full water bucket selected */
                    if (mg2.selectedSlot != -1 && mg2.inventory[mg2.selectedSlot] == ITEM_BUCKET_WATER) {
                        mg2.hasTree = true;
                        mg2.inventory[mg2.selectedSlot] = ITEM_BUCKET_EMPTY; /* Water used up */
                        mg2.selectedSlot = -1;
                        
                        mg2.state = MG2_GROWING;
                        mg2.shakeTimer = 2.0f;
                        ShowNotification("You poured water. A massive tree is growing!");
                    } else {
                        ShowNotification("The soil looks dry. It needs water.");
                    }
                }
            }

            /* 3. River — fill an empty bucket with water */
            if (CheckCollisionPointRec(mg2.playerPos, mg2.riverRect)) {
                if (mg2.selectedSlot != -1 && mg2.inventory[mg2.selectedSlot] == ITEM_BUCKET_EMPTY) {
                    mg2.inventory[mg2.selectedSlot] = ITEM_BUCKET_WATER;
                    mg2.selectedSlot = -1;
                    ShowNotification("Bucket filled with water.");
                }
            }
        }
    }
}

/* ========== Per-Frame Rendering ========== */
void DrawMinigame2(void) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    
    /* Screen-shake offset (random jitter while timer is active) */
    float offsetX = 0, offsetY = 0;
    if (mg2.shakeTimer > 0) {
        offsetX = GetRandomValue(-10, 10);
        offsetY = GetRandomValue(-10, 10);
    }

    /* Draw full-screen background map */
    DrawTexturePro(mg2.bgMap, 
        (Rectangle){0, 0, (float)mg2.bgMap.width, (float)mg2.bgMap.height}, 
        (Rectangle){offsetX, offsetY, (float)sw, (float)sh}, 
        (Vector2){0,0}, 0.0f, WHITE);

    /* Draw rabbit (pink circle) or dropped grass blade (green circle) */
    if (mg2.rabbitActive) {
        DrawCircleV((Vector2){mg2.rabbitPos.x + offsetX, mg2.rabbitPos.y + offsetY}, 20, PINK);
        DrawText("Rabbit", mg2.rabbitPos.x - 20 + offsetX, mg2.rabbitPos.y - 40 + offsetY, 20, BLACK);
    } else if (mg2.grassBladeDropped) {
        DrawCircleV((Vector2){mg2.rabbitPos.x + offsetX, mg2.rabbitPos.y + offsetY}, 10, GREEN);
    }

    /* Draw grass patch / tilled soil / tree */
    if (!mg2.isSoil) {
        DrawRectangle(mg2.grassPatchPos.x - 30 + offsetX, mg2.grassPatchPos.y - 30 + offsetY, 60, 60, DARKGREEN);
    } else {
        DrawRectangle(mg2.grassPatchPos.x - 30 + offsetX, mg2.grassPatchPos.y - 30 + offsetY, 60, 60, BROWN);
        if (mg2.hasTree) {
            if (mg2.treeTex.id > 0) {
                DrawTexture(mg2.treeTex, mg2.grassPatchPos.x - mg2.treeTex.width/2 + offsetX, mg2.grassPatchPos.y - mg2.treeTex.height + offsetY, WHITE);
            } else {
                /* Fallback: draw a simple rectangle + circle tree */
                DrawRectangle(mg2.grassPatchPos.x - 20 + offsetX, mg2.grassPatchPos.y - 200 + offsetY, 40, 200, MAROON);
                DrawCircle(mg2.grassPatchPos.x + offsetX, mg2.grassPatchPos.y - 200 + offsetY, 80, GREEN);
            }
        }
    }

    /* Draw the player character using sprite-sheet animation */
    if (mg2.playerTex.id > 0) {
        float frameWidth = (float)mg2.playerTex.width / 4;    /* 4 columns */
        float frameHeight = (float)mg2.playerTex.height / 4;  /* 4 rows */
        Rectangle sourceRec = { mg2.currentFrame * frameWidth, mg2.currentDir * frameHeight, frameWidth, frameHeight };
        Rectangle destRec = { mg2.playerPos.x + offsetX, mg2.playerPos.y + offsetY, frameWidth * 0.5, frameHeight * 0.5 };
        destRec.x -= destRec.width / 2;    /* Center horizontally */
        destRec.y -= destRec.height / 2;   /* Center vertically */
        DrawTexturePro(mg2.playerTex, sourceRec, destRec, (Vector2){0,0}, 0.0f, WHITE);
    } else {
        /* Fallback: blue rectangle placeholder */
        DrawRectangle(mg2.playerPos.x - 20 + offsetX, mg2.playerPos.y - 40 + offsetY, 40, 80, BLUE);
    }

    /* Draw QTE overlay (black hole + struggle button + countdown) */
    if (mg2.state == MG2_QTE) {
        DrawRectangle(0, 0, sw, sh, (Color){0,0,0, 150});
        DrawCircle(sw/2, sh/2, 150 + GetRandomValue(-10, 10), BLACK);
        DrawText("PULLING YOU IN...", sw/2 - 150, sh/2 - 200, 40, RED);
        Rectangle btnRect = { sw/2.0f - 100, sh/2.0f + 100, 200, 60 };
        DrawRectangleRec(btnRect, RED);
        DrawRectangleLinesEx(btnRect, 3, WHITE);
        DrawText("Struggle!", btnRect.x + 35, btnRect.y + 15, 30, WHITE);
        DrawText(TextFormat("%.1f", mg2.qteTimer), sw/2 - 30, sh/2, 50, WHITE);
    }

    /* Draw top notification banner */
    if (mg2.notificationTimer > 0) {
        int tw = MeasureText(mg2.notificationText, 24);
        DrawRectangle(sw/2 - tw/2 - 20, 20, tw + 40, 50, (Color){0,0,0,200});
        DrawText(mg2.notificationText, sw/2 - tw/2, 33, 24, YELLOW);
    }

    /* ---- Draw bottom inventory toolbar (consistent with minigame 1 style) ---- */
    int slotSize = 80;
    int startX = 100;
    int slotY = sh - 120;
    for (int slot = 0; slot < MG2_MAX_INVENTORY; slot++) {
        Rectangle slotRect = { startX + slot * (slotSize + 10), slotY, slotSize, slotSize };

        /* Draw slot background */
        if (mg2.inventorySlotTex.id > 0) {
            DrawTexturePro(mg2.inventorySlotTex,
                (Rectangle){ 0, 0, (float)mg2.inventorySlotTex.width, (float)mg2.inventorySlotTex.height },
                (Rectangle){ slotRect.x, slotRect.y, slotRect.width, slotRect.height },
                (Vector2){ 0, 0 }, 0.0f, WHITE);
        } else {
            DrawRectangleRec(slotRect, LIGHTGRAY);
            DrawRectangleLinesEx(slotRect, 2, DARKGRAY);
        }

        /* Draw item icon inside the slot */
        if (mg2.inventory[slot] != -1) {
            int itemId = mg2.inventory[slot];
            Texture2D tex = {0};
            Color fallbackColor = BLANK;

            /* Map item ID to texture and fallback color */
            if (itemId == ITEM_BUCKET_EMPTY) { tex = mg2.itemBucketEmptyTex; fallbackColor = GRAY; }
            else if (itemId == ITEM_BUCKET_WATER) { tex = mg2.itemBucketWaterTex; fallbackColor = BLUE; }
            else if (itemId == ITEM_GRASS) { tex = mg2.itemGrassTex; fallbackColor = GREEN; }

            if (tex.id > 0) {
                /* Scale the icon to fit within the slot (with 10px padding) */
                float scale = fminf((slotSize - 10) / (float)tex.width, (slotSize - 10) / (float)tex.height);
                int drawW = (int)(tex.width * scale);
                int drawH = (int)(tex.height * scale);
                int drawX = slotRect.x + (slotSize - drawW) / 2;
                int drawY = slotRect.y + (slotSize - drawH) / 2;
                DrawTextureEx(tex, (Vector2){ drawX, drawY }, 0.0f, scale, WHITE);
            } else {
                /* Fallback: colored rectangle if texture is missing */
                DrawRectangle(slotRect.x + 20, slotRect.y + 20, 40, 40, fallbackColor);
            }
        }

        /* Highlight selected slot with yellow border */
        if (slot == mg2.selectedSlot) {
            DrawRectangleLinesEx(slotRect, 4, YELLOW);
        }
    }

    /* Draw paging dialogue box at the bottom of the screen */
    if (mg2.showDialogue) {
        int boxY = sh - 200;
        DialogueLine *currentLine = &mg2.dialogueQueue[mg2.currentDialogueIndex];

        DrawRectangle(20, boxY, sw - 40, 180, (Color){0,0,0,220});
        DrawText(currentLine->speaker, 50, boxY + 20, 30, RED);
        DrawText(currentLine->text, 50, boxY + 70, 24, WHITE);
        
        /* Show page indicator */
        if (mg2.currentDialogueIndex < mg2.dialogueCount - 1) {
            DrawText("Click for next page...", sw - 280, boxY + 145, 20, GRAY);
        } else {
            DrawText("Click to continue", sw - 250, boxY + 145, 20, YELLOW);
        }
    }
}

/* ========== Resource Cleanup ========== */
void UnloadMinigame2(void) {
    if (mg2.bgMap.id > 0) UnloadTexture(mg2.bgMap);
    if (mg2.playerTex.id > 0) UnloadTexture(mg2.playerTex);
    if (mg2.treeTex.id > 0) UnloadTexture(mg2.treeTex);
    if (mg2.holeTex.id > 0) UnloadTexture(mg2.holeTex);
    
    /* Unload inventory UI textures */
    if (mg2.inventorySlotTex.id > 0) UnloadTexture(mg2.inventorySlotTex);
    if (mg2.itemBucketEmptyTex.id > 0) UnloadTexture(mg2.itemBucketEmptyTex);
    if (mg2.itemBucketWaterTex.id > 0) UnloadTexture(mg2.itemBucketWaterTex);
    if (mg2.itemGrassTex.id > 0) UnloadTexture(mg2.itemGrassTex);

    memset(&mg2, 0, sizeof(mg2));  /* Zero-out all state for safe re-initialization */
}

/* ========== Private Helper Functions ========== */

/* Add an item to the first empty inventory slot */
static void AddToInventory(int itemId) {
    for (int i = 0; i < MG2_MAX_INVENTORY; i++) {
        if (mg2.inventory[i] == -1) {
            mg2.inventory[i] = itemId;
            break;
        }
    }
}

/* Queue a dialogue line into the paging system (displayed on next render frame) */
static void ShowDialog(const char* speaker, const char* text) {
    if (mg2.dialogueCount < MAX_DIALOGUE_LINES) {
        strncpy(mg2.dialogueQueue[mg2.dialogueCount].speaker, speaker, 63);
        strncpy(mg2.dialogueQueue[mg2.dialogueCount].text, text, 255);
        mg2.dialogueCount++;
        mg2.showDialogue = true;
    }
}

/* Display a temporary notification banner at the top of the screen */
static void ShowNotification(const char* text) {
    strncpy(mg2.notificationText, text, 127);
    mg2.notificationTimer = 3.0f;
}

/* Euclidean distance between two 2D points */
static float GetDistance(Vector2 p1, Vector2 p2) {
    return sqrtf((p1.x - p2.x)*(p1.x - p2.x) + (p1.y - p2.y)*(p1.y - p2.y));
}

/* Transition out of minigame 2 into a story scene (or ending) */
static void ExitMinigame2(const char* nextScene) {
    char target[32];
    strncpy(target, nextScene, 31);  /* Copy before UnloadMinigame2 destroys state */
    UnloadMinigame2();
    game.current_scene = GetSceneByID(target);
    if (game.current_scene) {
        game.state = STATE_PLAYING;
        game.dialogue_index = 0;
        game.auto_timer = 0.0f;
    } else {
        game.state = STATE_TITLE;  /* Fallback if scene not found */
    }
}