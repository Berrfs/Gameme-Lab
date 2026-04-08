/* bossbattle.h — Public interface for the boss battle module.
   Declares Init/Update/Draw/Unload lifecycle functions for the final boss fight
   against "Mr. Glitch", featuring bullet-hell mechanics and UI-destruction meta-narrative.
   Code updated by 周沐格, at 09:24PM 2026/04/03 */

#ifndef BOSSBATTLE_H
#define BOSSBATTLE_H

void InitBossBattle(void);
void UpdateBossBattle(void);
void DrawBossBattle(void);
void UnloadBossBattle(void);

#endif