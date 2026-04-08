/* minigame3.h — Public interface for Minigame 3 (vertical shooter / bullet-hell).
   Player controls a plane with WASD, shoots enemies with SPACE/J,
   collects data fragments, and faces meta-game interference events.
   Code updated by 周沐格, at 09:24PM 2026/04/03 */

#ifndef MINIGAME3_H
#define MINIGAME3_H

#include "raylib.h"

void InitMinigame3(void);
void UpdateMinigame3(void);
void DrawMinigame3(void);
void UnloadMinigame3(void);

#endif