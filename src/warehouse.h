#ifndef WAREHOUSE_H
#define WAREHOUSE_H

#include "raylib.h"

void InitWarehouse(void);
void UpdateWarehouse(void);
void DrawWarehouse(void);
void UnloadWarehouse(void);
// 【新增】：用于接收 Minigame3 的结果
void NotifyWarehouseMinigame3(bool success);

#endif