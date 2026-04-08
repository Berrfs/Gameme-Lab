/* minigame2.h — Public interface for Minigame 2 (open-world exploration).
   Top-down exploration with WASD movement, sprite animation, inventory system,
   and a multi-step quest (rabbit → sapling → water → tree → QTE escape).
   Code updated by 周沐格, at 09:24PM 2026/04/03 */

#ifndef MINIGAME2_H
#define MINIGAME2_H

#include "raylib.h"

void InitMinigame2(void);
void UpdateMinigame2(void);
void DrawMinigame2(void);
void UnloadMinigame2(void);

#endif // MINIGAME2_H