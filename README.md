# No Way! — Software Design Document & README

**A narrative-driven visual novel game built with C and Raylib 5.5**

Developed by **Gameme Lab**

---

## Table of Contents

1. [About the Game](#about-the-game)
2. [How to Play](#how-to-play)
3. [Project Structure](#project-structure)
4. [System Architecture](#system-architecture)
5. [Code Design — State Machine](#code-design--state-machine)
6. [Scene & Dialogue System](#scene--dialogue-system)
7. [Save / Load System](#save--load-system)
8. [Key Logic & Algorithms](#key-logic--algorithms)
9. [Complete Game Flow](#complete-game-flow)
10. [Tech Stack](#tech-stack)

---

## About the Game

**No Way!** is an interactive visual novel where a sentient game character called **"Game"** refuses to let the player play. Through branching storylines, puzzle-solving, and 4 unique minigames, the player uncovers the truth — a villain named **"Mr. Glitch"** stole Game's source code, leaving it broken and incomplete. The player must help Game fight back and reclaim its stolen code.

### Key Features

- **Branching Narrative** — Multiple story paths with 2+ endings driven by player choices
- **Dynamic Dialogue System** — Character portraits, dialogue boxes, auto/manual advance modes
- **4 Unique Minigames** — Escape room, open-world exploration, bullet-hell shooter, platformer
- **Final Boss Battle** — Multi-phase fight with meta-narrative UI destruction mechanic
- **Interactive UI** — Hover effects, pointer cursors, responsive button scaling
- **Settings Panel** — Volume control, auto-advance toggle, interval slider, fullscreen toggle
- **Save/Load System** — Press V anytime to save; resume from title screen with "Continue"
- **JSON-based Scenes** — All story data lives in `data/scenes.json` for easy editing

### Story Summary

The player opens "No Way!" expecting a normal game, but is greeted by the game character "Game" who insists: "You shouldn't be playing this." Through persistence, the player enters a backstage escape room, discovers a collapsed world, explores an open field, navigates a warehouse in space, and eventually confronts Mr. Glitch — the virus who stole Game's code. The outcome depends on the player's choices and skill.

---

## How to Play

### Option A: Run the Pre-Built Executable (Recommended)

```
1. Download or extract the entire "No_Way" folder
2. Double-click  game.exe
3. Enjoy!
```

**Important**: Keep `libraylib.dll` and `glfw3.dll` in the same folder as `game.exe`.
The `UI/` and `data/` folders must also remain alongside the executable.
No additional software installation is required.

### Option B: Compile from Source

**Prerequisites**: MSYS2 (https://www.msys2.org/) with the `ucrt64` environment and Raylib installed via pacman.

```bash
# Step 1: Open a terminal (Command Prompt or MSYS2 UCRT64)
# Step 2: Navigate to the project folder
cd path\to\No_Way

# Step 3: Compile the game (this also copies required DLLs)
.\compile.bat

# Step 4: Run the game
.\game.exe
```

### Controls Reference

| Context | Key / Action | Function |
|:---|:---|:---|
| Title Screen | Click buttons / ENTER | Start new game |
| Title Screen | Click "Continue" | Load saved game |
| Dialogue | Click / SPACE | Advance to next line |
| Choices | Click / Number keys 1-9 | Select a story branch |
| Name Input | Type + ENTER | Confirm player name |
| Minigame 1 | Click items / arrows | Interact, navigate walls |
| Minigame 2 | W/A/S/D + Click | Move character, interact |
| Minigame 3 | W/A/S/D | Move plane |
| Minigame 3 | Hold SPACE or J | Fire bullets |
| Minigame 4 | A/D | Move left/right |
| Minigame 4 | SPACE | Jump + swap world color |
| Boss Battle | W/A/S/D | Dodge bullets |
| Boss Battle | Click on Boss | Attack (when weapon ready) |
| Settings | Left/Right arrows | Adjust volume |
| Settings | Up/Down arrows | Adjust auto-interval |
| Settings | M | Toggle auto/manual mode |
| Settings | F | Toggle fullscreen |
| Settings | B | Back to title |
| Anytime | V | Quick save |
| Anytime | ESC | Return to title |

---

## Project Structure

```
No_Way/
├── src/                        # All source code (10 modules + 1 library)
│   ├── main.c                  # Entry point — window init and game loop
│   ├── game.c                  # Core state machine, rendering, input dispatch
│   ├── game.h                  # GameState enum, GameContext struct, public API
│   ├── scene.c                 # JSON scene parser — dialogues & branching choices
│   ├── scene.h                 # Scene, Dialogue, Choice data structures
│   ├── save.c                  # Save/load system (JSON serialization)
│   ├── save.h                  # Save system interface
│   ├── minigame.c              # Minigame 1: Backstage escape room (puzzle)
│   ├── minigame.h              # Minigame 1 structs & interface
│   ├── minigame2.c             # Minigame 2: Open-world exploration (adventure)
│   ├── minigame2.h             # Minigame 2 interface
│   ├── minigame3.c             # Minigame 3: Vertical shooter (bullet-hell)
│   ├── minigame3.h             # Minigame 3 interface
│   ├── minigame4.c             # Minigame 4: Platformer (world-swap mechanic)
│   ├── minigame4.h             # Minigame 4 interface
│   ├── warehouse.c             # Warehouse hub scene (3-room navigation)
│   ├── warehouse.h             # Warehouse interface
│   ├── bossbattle.c            # Final boss: Mr. Glitch (multi-phase fight)
│   ├── bossbattle.h            # Boss battle interface
│   └── cJSON/                  # Third-party JSON parser library
│       ├── cJSON.c
│       └── cJSON.h
├── data/
│   ├── scenes.json             # Story script — all scenes, dialogues, choices
│   └── settings.json           # Persistent user settings (volume, fullscreen, etc.)
├── UI/                         # All image assets (38 files)
│   ├── red curtain.jpg         # Title screen background
│   ├── no way.png              # Game logo
│   ├── Gameme Lab.png          # Studio logo
│   ├── player_sprite.png       # 4x4 sprite sheet (4 directions x 4 frames)
│   ├── bg_wall_0/1/2.png       # Minigame 1 wall backgrounds
│   ├── map.png                 # Minigame 2 world map
│   ├── background_image *.png  # Story scene backgrounds
│   └── ... (buttons, items, portraits)
├── compile.bat                 # Build script (GCC + auto DLL bundling)
├── game.exe                    # Compiled game binary
├── libraylib.dll               # Raylib runtime library (bundled)
├── glfw3.dll                   # GLFW window library (bundled)
└── README.md                   # This document
```

---

## System Architecture

### Module Dependency Diagram

The game is organized into 10 source modules plus one third-party library.
Each module follows the `Init -> Update -> Draw -> Unload` lifecycle pattern.

```
                         ┌──────────────┐
                         │   main.c     │
                         │ Entry Point  │
                         └──────┬───────┘
                                │
                         ┌──────▼───────┐
                    ┌────┤   game.c     ├────┐
                    │    │ State Machine │    │
                    │    └──┬───┬───┬───┘    │
                    │       │   │   │        │
              ┌─────▼──┐   │   │   │   ┌────▼─────┐
              │scene.c  │   │   │   │   │  save.c  │
              │JSON     │   │   │   │   │Save/Load │
              │Parser   │   │   │   │   └────┬─────┘
              └────┬────┘   │   │   │        │
                   │        │   │   │        │
                   ▼        │   │   │        ▼
              ┌─────────┐   │   │   │   ┌─────────┐
              │  cJSON   │◄─┘   │   └──►│  cJSON   │
              │(Library) │      │       │(Library) │
              └──────────┘      │       └──────────┘
                                │
          ┌──────────┬──────────┼──────────┬──────────┐
          │          │          │          │          │
    ┌─────▼────┐┌────▼─────┐┌──▼───────┐  │  ┌──────▼──────┐
    │minigame.c││minigame2 ││warehouse │  │  │bossbattle.c │
    │Escape    ││Explore   ││Hub Scene │  │  │Boss Fight   │
    │Room      ││Adventure ││3 rooms   │  │  │Mr. Glitch   │
    └──────────┘└──────────┘└──┬───┬───┘  │  └─────────────┘
                               │   │      │
                         ┌─────▼┐ ┌▼──────┘
                         │mg3.c │ │mg4.c
                         │Shoot │ │Platform
                         └──────┘ └───────
```

### Module Lifecycle Pattern

Every game module follows a consistent 4-function interface:

| Function | Purpose | When Called |
|:---|:---|:---|
| `InitXxx()` | Load textures, initialize state | On state entry |
| `UpdateXxx()` | Per-frame input handling & logic | Every frame from `UpdateGame()` |
| `DrawXxx()` | Per-frame rendering | Every frame from `DrawGame()` |
| `UnloadXxx()` | Free textures & memory | On state exit or game shutdown |

---

## Code Design — State Machine

The entire game is driven by a **finite state machine** defined in `game.h`. The global `GameContext` struct holds all runtime state. Each frame, `UpdateGame()` and `DrawGame()` read `game.state` and dispatch to the appropriate handler.

### GameState Enum

```c
typedef enum GameState {
    STATE_TITLE,        // Title / splash screen
    STATE_NAME_INPUT,   // Player name input screen
    STATE_PLAYING,      // Story dialogue playback
    STATE_CHOICE,       // Branching choice overlay
    STATE_SETTINGS,     // Settings / options menu
    STATE_MINIGAME,     // Minigame 1: escape room
    STATE_MINIGAME2,    // Minigame 2: exploration
    STATE_WAREHOUSE,    // Warehouse hub (3 rooms)
    STATE_MINIGAME3,    // Minigame 3: shooter
    STATE_MINIGAME4,    // Minigame 4: platformer
    STATE_BOSSBATTLE    // Final boss fight
} GameState;
```

### State Transition Diagram

```
                              ┌─── Click Menu ──── STATE_SETTINGS ───┐
                              │                         │            │
                              │                      Press B         │
                              │                         │            │
 [START] ──► STATE_TITLE ─────┼─── Click Start ──► STATE_NAME_INPUT  │
                 ▲            │                         │            │
                 │            │                      ENTER           │
                 │            │                         │            │
                 │            └─── Continue ──► STATE_PLAYING ◄──────┘
                 │                                  │       ▲
                 │                    ┌──────────────┤       │
                 │                    │              │       │
                 │              has choices?    has next_scene?
                 │                    │              │       │
                 │                    ▼              │       │
                 │             STATE_CHOICE ─────────┘  Player picks
                 │                                         choice
                 │
                 │     next_scene triggers minigame transitions:
                 │
                 │     "MINIGAME"   ──► STATE_MINIGAME ─── puzzle done ──► STATE_PLAYING
                 │     "MINIGAME2"  ──► STATE_MINIGAME2 ── QTE result ──► STATE_PLAYING
                 │     "WAREHOUSE"  ──► STATE_WAREHOUSE ─┬─► STATE_MINIGAME3 ──┐
                 │                                       └─► STATE_MINIGAME4 ──┤
                 │                                       ◄─── result ──────────┘
                 │                                       ── both cleared ──► STATE_PLAYING
                 │     "BOSSBATTLE" ──► STATE_BOSSBATTLE ── win/lose ──► STATE_PLAYING
                 │
                 └──────── scene ends with no next / ESC pressed
```

### GameContext Struct (Key Fields)

| Field | Type | Purpose |
|:---|:---|:---|
| `state` | `GameState` | Current state machine state |
| `current_scene` | `Scene*` | Pointer to active scene data |
| `dialogue_index` | `int` | Current line in the dialogue array |
| `player_name` | `char[21]` | Player's chosen name (max 20 chars) |
| `master_volume` | `float` | Volume level 0.0 – 1.0 |
| `auto_mode` | `bool` | Auto-advance dialogue on/off |
| `auto_interval` | `float` | Seconds between auto-advance steps |
| `fullscreen` | `bool` | Fullscreen mode on/off |
| `currentBackground` | `Texture2D` | Cached background texture |
| `currentPortrait` | `Texture2D` | Cached speaker portrait texture |

---

## Scene & Dialogue System

### How It Works

1. **At startup**, `LoadScenesFromJSON("data/scenes.json")` parses the entire story script into an array of `Scene` structs using the **cJSON** library.
2. **During gameplay**, `UpdatePlaying()` reads `game.current_scene` and advances through its `dialogues[]` array.
3. **At the end of a scene**:
   - If `choices[]` is non-empty -> transition to `STATE_CHOICE`
   - If `next_scene` is set -> transition to the target (scene or minigame)
   - If neither -> return to title

### JSON Schema

```json
{
  "scenes": [
    {
      "id": "scene1",
      "background": "computer.jpg",
      "dialogues": [
        { "speaker": "???", "text": "You shouldn't be playing this." },
        { "speaker": "Player", "text": "Who's that?" }
      ],
      "choices": [
        { "text": "Keep questioning", "next_scene": "scene2" },
        { "text": "Walk away", "next_scene": "title" }
      ],
      "next_scene": null
    }
  ]
}
```

### Data Structures (scene.h)

```c
typedef struct Dialogue {
    char *speaker;       // Character name ("Player" is replaced with actual name)
    char *text;          // The dialogue line
} Dialogue;

typedef struct Choice {
    char *text;           // Display text for this option
    char *next_scene_id;  // Scene ID to jump to
} Choice;

typedef struct Scene {
    char *id;              // Unique identifier (e.g. "scene1")
    char *background;      // Background image filename in UI/
    Dialogue *dialogues;   // Array of dialogue lines
    int dialogue_count;
    Choice *choices;       // Array of branching choices
    int choice_count;
    char *next_scene_id;   // Auto-transition target (when no choices)
} Scene;
```

### Special next_scene Values

| Value | Effect |
|:---|:---|
| `"scene2"`, `"scene3"`, etc. | Normal scene transition |
| `"MINIGAME"` | Launch Minigame 1 (escape room) |
| `"MINIGAME2"` | Launch Minigame 2 (exploration) |
| `"WAREHOUSE"` | Launch warehouse hub scene |
| `"BOSSBATTLE"` | Launch final boss battle |
| `"title"` | Return to title screen |
| `null` / not set | Return to title screen |

---

## Save / Load System

### Overview

The save system (`save.c`) provides instant save/load using JSON serialization via cJSON.

### Save Format (data/save.json)

```json
{
    "player_name": "Louis",
    "state": 2,
    "scene_id": "scene3",
    "dialogue_index": 1
}
```

### Algorithm

**Saving** (triggered by pressing V):
1. Serialize `player_name`, `game.state`, `current_scene->id`, and `dialogue_index` into a cJSON object
2. Write to `data/save.json` using standard C file I/O

**Loading** (triggered by clicking "Continue"):
1. Parse `data/save.json` with cJSON
2. Restore all fields to `GameContext`
3. If the saved state was a minigame, call the appropriate `InitXxx()` to reload textures

### Minigame State Restoration

```c
switch (game.state) {
    case STATE_MINIGAME:    InitMinigame();  break;
    case STATE_MINIGAME2:   InitMinigame2(); break;
    case STATE_WAREHOUSE:   InitWarehouse(); break;
    case STATE_MINIGAME3:   InitMinigame3(); break;
    default: break; // STATE_PLAYING auto-handled by UpdatePlaying()
}
```

---

## Key Logic & Algorithms

### Minigame 1: Backstage Escape Room

**Type**: Point-and-click puzzle across 3 navigable walls

**Puzzle Solution Chain**:

```
1. Examine picture in frame (Wall 0) -> take picture out
2. Put picture back -> go to typewriter (Wall 1)
3. Type "time" -> type "walk" -> type "bride" -> nail spawns (Wall 2)
4. Pick up nail (Wall 2) -> pick up hammer (Wall 2)
5. Use nail on picture/frame -> use hammer on nailed picture
6. Key drops -> pick up key -> use key on door (Wall 1) -> EXIT
```

**Key Algorithm — Dynamic Item Layout**:

Each item is positioned using a `Layout` struct with center-point proportions:

```c
typedef struct {
    float cx;     // Center X as ratio of screen width  (0.0-1.0)
    float cy;     // Center Y as ratio of screen height (0.0-1.0)
    float scale;  // Size multiplier relative to original texture
} ItemLayout;
```

`UpdateItemRects()` recalculates pixel-perfect bounding boxes whenever the window resizes, ensuring click detection always matches visual position.

**Key Algorithm — Typewriter Password**:
- Player types into a text buffer using `GetCharPressed()` (Raylib character input)
- On ENTER, input is lowercased and compared against `{"time", "walk", "bride"}` in sequence
- Step counter advances on match; all 3 correct -> nail spawns

---

### Minigame 2: Open-World Exploration

**Type**: Top-down adventure with WASD movement, inventory, and NPC interaction

**Quest Chain**:

```
1. Receive empty bucket from "Game" (auto)
2. Find rabbit -> click rabbit -> sapling drops
3. Pick up sapling into inventory
4. Find grass patch -> click to till soil
5. Select sapling from inventory -> click soil -> plant
6. Walk to river -> select empty bucket -> click -> bucket fills with water
7. Select water bucket -> click planted sapling -> tree grows
8. "Game" gets angry -> QTE sequence begins
9. QTE: Click "Struggle!" button within 3 seconds
   - Success -> scene5 (story continues)
   - Failure -> ending1 (player swallowed by void)
```

**Key Algorithm — Sprite Animation**:

The player sprite uses a 4x4 sprite sheet (4 cols = animation frames, 4 rows = directions):

```c
float frameWidth  = playerTex.width / 4;
float frameHeight = playerTex.height / 4;
// Source rectangle selects the current frame and direction
Rectangle sourceRec = {
    currentFrame * frameWidth,   // Column = animation frame
    currentDir * frameHeight,    // Row = direction (Down/Up/Left/Right)
    frameWidth, frameHeight
};
```

Frame advances every 0.15 seconds while the player is moving.

**Key Algorithm — River Interaction**:

Three overlapping rectangles define the river zone. The player must be standing inside one AND have an empty bucket selected to fill it:

```c
bool isInRiver = false;
for (int i = 0; i < 3; i++) {
    if (CheckCollisionPointRec(playerPos, riverRects[i])) {
        isInRiver = true; break;
    }
}
if (isInRiver && selectedItem == ITEM_BUCKET_EMPTY) {
    inventory[selectedSlot] = ITEM_BUCKET_WATER;
}
```

---

### Minigame 3: Vertical Shooter (Bullet-Hell)

**Type**: Top-down shooter with enemy waves, item drops, and meta-game interference

**Core Loop**:

```
1. Player moves with WASD, shoots with SPACE/J (hold for auto-fire)
2. Enemies spawn from top, fly downward, fire aimed bullets
3. Destroying enemies -> data fragments drop -> player collects them
4. At 50% progress -> META EVENT 1: controls reverse for 5 seconds
5. At 80% progress -> META EVENT 2: gravity anomaly pulls player upward
6. At 100% progress -> EXIT zone appears at top of screen
7. Fly into EXIT -> return to warehouse (combat cleared)
```

**Enemy Types**:

| Type | HP | Fire Pattern | Shoot Interval |
|:---|:---|:---|:---|
| Type 1 (common) | 2 | Single aimed shot | 1.2s |
| Type 2 (elite) | 4 | 3-way spread (60 degree arc) | 2.0s |

**Meta-Game Events** (the game character "fighting back"):

```c
// At 50% progress: invert player controls for 5 seconds
int dir = (reverseTimer > 0) ? -1 : 1;
player.pos.x += PLAYER_SPEED * dt * dir;  // Movement reversed!

// At 80% progress: gravity pulls player toward center-top
if (metaEvent2) {
    float pullSpeed = 80.0f * dt;
    if (player.pos.x < screenWidth/2) player.pos.x += pullSpeed;
    else player.pos.x -= pullSpeed;
    player.pos.y -= pullSpeed;  // Pulled upward
}
```

---

### Minigame 4: Platformer (World-Swap)

**Type**: Precision platformer with a color-swap mechanic

**Core Mechanic**: Pressing SPACE does TWO things simultaneously:
1. **Jump** (instant) — velocity set to -550 px/s upward
2. **World swap** (delayed 0.15s) — background color toggles between red and black

**Platform Rules**:

| Background | Solid Platforms | Ghost Platforms |
|:---|:---|:---|
| Red background | Black pillars | Red pillars (pass-through) |
| Black background | Red pillars | Black pillars (pass-through) |

**Level Design**: 8 symmetrical pillars arranged in 4 pairs, each split at a different height:

```c
int splits[4] = { 310, 400, 470, 530 };  // Cut points for each pair
// Left 4 pillars at X = {120, 250, 380, 510}
// Right 4 pillars at X = {710, 840, 970, 1100}
// Gap in the middle for navigation
```

**Key Algorithm — Separated Axis Collision**:

Physics uses separated X/Y collision detection to prevent corner-sticking:

```c
// Step 1: Move X, check collisions, resolve
playerPos.x += velocity.x * dt;
for (each solid platform) {
    if (collision) push player out horizontally;
}
// Step 2: Move Y, check collisions, resolve
playerPos.y += velocity.y * dt;
for (each solid platform) {
    if (collision) {
        if (falling) land on top, isGrounded = true;
        if (rising) bump head, velocity.y = 0;
    }
}
```

---

### Boss Battle: Mr. Glitch

**Type**: Multi-phase boss fight combining bullet-hell dodging with a meta-narrative

**Phase Progression**:

```
INTRO ──► SURVIVAL ──► REPAIRING ──► COUNTER ──► Result
  │           │             │            │          │
  │     Dodge bullets  Collect 5     Click Boss   Win: scene10
  │     UI buttons     code          3 hits to    Lose: ending2
  │     get destroyed  fragments     kill
  │
  Boss taunts player
```

**Phase Details**:

| Phase | What Happens |
|:---|:---|
| INTRO | Mr. Glitch dialogue: "Your UI is completely useless here!" |
| SURVIVAL | Boss fires aimed 3-way spread shots. Three fake ATTACK buttons act as destructible shields. When a button's HP reaches 0, it shatters and scatters 3 code fragments with random velocity + friction physics. |
| REPAIRING | Player walks over fragments to collect them. At 5/5, "Game" recompiles them: "Recompiling fragments... Success!" |
| COUNTER | Player clicks on Boss within range to deal damage. Boss still fires bullets. 3 hits = victory. |
| DEFEATED | Mr. Glitch: "IMPOSSIBLE... NOOOOOOO!" -> transition to story ending |

**Bullet Aim Algorithm**:

```c
// Calculate angle from boss to player
float baseAngle = atan2f(player.y - boss.y, player.x - boss.x);
// Spread 3 bullets across 45 degree arc centered on the player
for (int i = 0; i < BOSS_BULLET_COUNT; i++) {
    float angle = baseAngle + (i - halfCount) * angleStep;
    bullet.velocity = { cos(angle) * SPEED, sin(angle) * SPEED };
}
```

**Fragment Physics**:

```c
// Fragments scatter with random initial velocity
float angle = random(0, 360) * PI / 180;
float speed = random(400, 700);
piece.velocity = { cos(angle) * speed, sin(angle) * speed };
piece.friction = 0.92f;  // Decelerates each frame, eventually stops

// Each frame: move and apply friction
piece.pos += piece.velocity * dt;
piece.velocity *= piece.friction;
```

---

## Complete Game Flow

```
                            ┌──────────────────┐
                            │   TITLE SCREEN   │
                            │ Start/Continue/  │
                            │ Settings/Exit    │
                            └───────┬──────────┘
                                    │
                        ┌───────────┼───────────┐
                        │           │           │
                   Click Start   Continue    Settings
                        │        (load)      (volume,
                        ▼           │        fullscreen)
                ┌───────────────┐   │
                │  ENTER NAME   │   │
                │ (max 20 char) │   │
                └───────┬───────┘   │
                        │           │
                        ▼           │
                ┌───────────────┐◄──┘
                │   SCENE 1     │
                │ "You shouldn't│
                │ be playing"   │
                └───────┬───────┘
                        │
                    ┌───┴───┐
                    │CHOICE │
               ┌────┤       ├────┐
               │    └───────┘    │
          Walk away      Keep questioning
               │                 │
               ▼                 ▼
          Return to         SCENE 2
          Title             "You can't play"
                                 │
                                 ▼
                        ┌─────────────────┐
                        │  MINIGAME 1     │
                        │ Escape Room     │
                        │ (3 walls,       │
                        │  item puzzles)  │
                        └────────┬────────┘
                                 │ Door unlocked
                                 ▼
                        SCENE 3-4 (World collapses)
                                 │
                                 ▼
                        ┌─────────────────┐
                        │  MINIGAME 2     │
                        │ Open-World      │
                        │ Exploration     │
                        │ (rabbit, tree)  │
                        └────────┬────────┘
                                 │
                            ┌────┴────┐
                            │  QTE    │
                            │Struggle!│
                            │ (3 sec) │
                            └┬──────┬─┘
                       Fail  │      │  Success
                             ▼      ▼
                        ENDING 1   SCENE 5-6
                        (Expelled)      │
                                        ▼
                        ┌──────────────────────┐
                        │   WAREHOUSE HUB      │
                        │  Cockpit | Storage |  │
                        │          Maze         │
                        └──┬────────────────┬───┘
                           │                │
                    ┌──────▼──────┐  ┌──────▼──────┐
                    │ MINIGAME 3  │  │ MINIGAME 4  │
                    │ Shooter     │  │ Platformer  │
                    │ (bullet-    │  │ (world-swap │
                    │  hell)      │  │  mechanic)  │
                    └──────┬──────┘  └──────┬──────┘
                           │                │
                           └────────┬───────┘
                                    │ Both cleared
                                    ▼
                        SCENE 7-9 (Meet Mr. Glitch)
                                    │
                                    ▼
                        ┌──────────────────────┐
                        │    BOSS BATTLE       │
                        │ vs Mr. Glitch        │
                        │ (4-phase fight)      │
                        └──┬────────────────┬──┘
                           │                │
                      Win  │                │ Lose
                           ▼                ▼
                     SCENE 10-11        ENDING 2
                     "Thank you,        (System
                      user."            Deletion)
                           │
                           ▼
                     FINAL SCENE
                     "I'm a terrible,
                      awful game..."
                           │
                           ▼
                     THANK YOU SCREEN
                           │
                           ▼
                     Return to Title
```

---

## Tech Stack

| Component | Technology | Details |
|:---|:---|:---|
| Language | C | Compiled with GCC (MSYS2 ucrt64) |
| Graphics | Raylib 5.5 | Window management, rendering, input |
| Window System | GLFW 3 | Used internally by Raylib |
| JSON Parser | cJSON | Embedded in `src/cJSON/`, single `.c`/`.h` |
| Build System | Batch script | `compile.bat` — single-command build + DLL bundling |
| Platform | Windows | Tested on Windows 10/11 |
| Resolution | 1280 x 720 | Scales with `GetUIScale()` for different resolutions |
| Frame Rate | 60 FPS | Locked via `SetTargetFPS(60)` |

---

**Gameme Lab (c) 2026**
