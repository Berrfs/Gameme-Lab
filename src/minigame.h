/* minigame.h
   Code updated by Louis, at 11:20AM 2026/03/23 */
#ifndef MINIGAME_H
#define MINIGAME_H

#include "raylib.h"

#define MAX_INVENTORY 5      // Number of inventory slots
#define MAX_ITEMS 7         // Total items in scene (picture, typewriter, script, door, key, nail, hammer)
#define WALL_COUNT 3         // Total number of walls

// Item Structure
typedef struct {
    int id;                  // Unique item identifier
    const char* name;        // Item name (for display)
    Texture2D texture;       // Item icon (displayed in inventory after pickup)
    int wallIndex;           // Which wall the item appears on (0, 1, 2)
    Rectangle interactRect;  // Clickable area on that wall (relative to screen coords)
    bool isPickedUp;         // Has the item been picked up?
    bool visible;            // Is it currently visible on the wall?
} Item;

// Dialogue Line (simple dialogue within the minigame)
typedef struct {
    const char* speaker;
    const char* text;
} MiniDialogue;

// Main Minigame Context (internal to minigame.c)
typedef struct MinigameContext {
    int currentWall;                     // Current wall index 0~2
    Texture2D wallTextures[WALL_COUNT];  // Background textures for the 3 walls
    Texture2D arrowLeft, arrowRight;     // Left/Right arrow textures
    Texture2D inventorySlot;              // Inventory slot background texture
    
    Item items[MAX_ITEMS];               // Definitions for all items
    int itemCount;
    
    int inventory[MAX_INVENTORY];         // Currently held item IDs, -1 means empty
    int selectedSlot;                     // Currently selected inventory slot index, -1 means none
    
    // Dialogue related variables
    MiniDialogue currentDialogue;
    bool showDialogue;
    float dialogueTimer;                   // Timer for auto-hide, or manual click to close
    
    // Additional states...
} MinigameContext;

// Global Minigame Context Functions (Managed by minigame.c, not accessed directly by game.c)
void InitMinigame(void);
void UpdateMinigame(void);
void DrawMinigame(void);
void UnloadMinigame(void);

#endif
