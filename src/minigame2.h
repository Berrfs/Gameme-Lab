/* minigame2.h — Public interface for Minigame 2 (outdoor exploration puzzle).
   Declares the lifecycle functions used by game.c to init, update, draw, and
   unload the second minigame.
   Code updated by Joan (周沐格), at 10:00PM 2026/03/24 */

#ifndef MINIGAME2_H
#define MINIGAME2_H

#include "raylib.h"

/* Core lifecycle functions — called by game.c state machine */
void InitMinigame2(void);    /* Load assets, set initial state */
void UpdateMinigame2(void);  /* Per-frame input & logic */
void DrawMinigame2(void);    /* Per-frame rendering */
void UnloadMinigame2(void);  /* Release all resources */

#endif // MINIGAME2_H