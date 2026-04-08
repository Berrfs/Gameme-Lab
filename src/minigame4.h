/* minigame4.h — Public interface for Minigame 4 (platformer with world-swap mechanic).
   Player jumps between red/black pillars; pressing SPACE triggers a jump and
   a delayed world-color swap, toggling which pillars are solid vs. ghosted.
   Code updated by 周沐格, at 09:24PM 2026/04/03 */

#ifndef MINIGAME4_H
#define MINIGAME4_H

#include <stdbool.h> // 需要用到 bool 类型

void InitMinigame4(void);
void UpdateMinigame4(void);
void DrawMinigame4(void);
void UnloadMinigame4(void);

#endif // MINIGAME4_H