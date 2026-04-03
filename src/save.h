/* save.h — Save / load system interface (planned feature).
   Currently declares stub functions to satisfy the linker.
   Code updated by 周沐格, at 05:05PM 2026/04/03 */
#ifndef SAVE_H
#define SAVE_H
#include <stdbool.h>

void SaveGame(void);     /* 存档：记录场景、对话索引、玩家名、游戏状态 */
void LoadGame(void);     /* 读档：恢复状态并初始化必要的小游戏资源 */
bool SaveExists(void);   /* 检查 data/save.json 是否存在 */

#endif
