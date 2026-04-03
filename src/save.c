/* save.c — Save / load system implementation (planned feature).
   Currently empty — serves as a placeholder for future persistence logic.
   Code updated by 周沐格, at 04:47PM 2026/04/03 */

#include "save.h"
#include "game.h"
#include "scene.h"
#include "minigame2.h"
#include "minigame3.h"
#include "warehouse.h"
#include "cJSON/cJSON.h"
#include <stdio.h>
#include <stdlib.h>

#define SAVE_PATH "data/save.json"

bool SaveExists(void) {
    return FileExists(SAVE_PATH);
}

void SaveGame(void) {
    cJSON *root = cJSON_CreateObject();
    
    // 保存基本信息
    cJSON_AddStringToObject(root, "player_name", game.player_name);
    cJSON_AddNumberToObject(root, "state", (double)game.state);
    
    // 如果在剧情中，保存场景ID
    if (game.current_scene && game.current_scene->id) {
        cJSON_AddStringToObject(root, "scene_id", game.current_scene->id);
    } else {
        // 如果当前没有场景（比如已经在小游戏中），存入一个空值，读档时靠 state 恢复
        cJSON_AddStringToObject(root, "scene_id", "NONE");
    }
    // 保存当前的对话索引
        cJSON_AddNumberToObject(root, "dialogue_index", game.dialogue_index);
    
    char *json_str = cJSON_Print(root);
    // 使用标准C库确保“覆盖写入”
    FILE *f = fopen(SAVE_PATH, "w");
    if (f) {
        fputs(json_str, f);
        fclose(f);
        TraceLog(LOG_INFO, "SAVE UPDATED: Scene %s, Index %d", 
                 game.current_scene ? game.current_scene->id : "None", 
                 game.dialogue_index);
    } else {
        TraceLog(LOG_ERROR, "FAILED TO UPDATE SAVE FILE!");
    }
    
    free(json_str);
    cJSON_Delete(root);
    TraceLog(LOG_INFO, "Game Saved to %s", SAVE_PATH);
}

void LoadGame(void) {
    char *json_str = LoadFileText(SAVE_PATH);
    if (!json_str) return;

    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        UnloadFileText(json_str);
        return;
    }

    // 恢复玩家名称
    cJSON *name = cJSON_GetObjectItem(root, "player_name");
    if (name) strcpy(game.player_name, name->valuestring);

    // 恢复状态
    cJSON *state = cJSON_GetObjectItem(root, "state");
    if (state) game.state = (GameState)state->valueint;

    // 恢复场景
    cJSON *s_id = cJSON_GetObjectItem(root, "scene_id");
    if (s_id) {
        game.current_scene = GetSceneByID(s_id->valuestring);
    }

    // 恢复对话索引
    cJSON *d_idx = cJSON_GetObjectItem(root, "dialogue_index");
    if (d_idx) game.dialogue_index = d_idx->valueint;

    // 特殊逻辑：如果是从小游戏状态恢复，需要重新初始化小游戏资源
    switch (game.state) {
        case STATE_MINIGAME:    InitMinigame(); break;
        case STATE_MINIGAME2:   InitMinigame2(); break;
        case STATE_WAREHOUSE:   InitWarehouse(); break;
        case STATE_MINIGAME3:   InitMinigame3(); break;
        default: break; // STATE_PLAYING 不需要特殊初始化，UpdatePlaying 会自动处理
    }
    
    cJSON_Delete(root);
    UnloadFileText(json_str);
    TraceLog(LOG_INFO, "Game Loaded from %s", SAVE_PATH);
}
/* TODO: Implement SaveGame() and LoadGame() using cJSON for serialization */
