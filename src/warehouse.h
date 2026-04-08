/* warehouse.h — Public interface for the warehouse hub scene.
   The warehouse is a 3-room hub (cockpit / storage / maze) that connects
   Minigame 3 (shooter) and Minigame 4 (platformer) via navigation arrows.
   Code updated by 周沐格, at 09:24PM 2026/04/03 */

#ifndef WAREHOUSE_H
#define WAREHOUSE_H

#include "raylib.h"

void InitWarehouse(void);
void UpdateWarehouse(void);
void DrawWarehouse(void);
void UnloadWarehouse(void);
// 【新增】：用于接收 Minigame3 的结果
void NotifyWarehouseMinigame3(bool success);
void NotifyWarehouseMinigame4(bool success);

#endif