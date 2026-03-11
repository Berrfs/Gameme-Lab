#include "scene.h"
#include "cJSON/cJSON.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>

Scene *g_scenes = NULL;
int g_scene_count = 0;

void LoadScenesFromJSON(const char *filename) {
    // 1. 读取文件
    char *json_str = LoadFileText(filename);
    if (!json_str) {
        TraceLog(LOG_ERROR, "Failed to load scenes file: %s", filename);
        return;
    }

    // 2. 解析 JSON
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        TraceLog(LOG_ERROR, "JSON parse error in %s", filename);
        UnloadFileText(json_str);
        return;
    }

    // 3. 获取 scenes 数组
    cJSON *scenes_array = cJSON_GetObjectItem(root, "scenes");
    if (!cJSON_IsArray(scenes_array)) {
        TraceLog(LOG_ERROR, "No scenes array found");
        cJSON_Delete(root);
        UnloadFileText(json_str);
        return;
    }

    // 4. 统计场景数量
    g_scene_count = cJSON_GetArraySize(scenes_array);
    g_scenes = (Scene*)malloc(sizeof(Scene) * g_scene_count);

    // 5. 遍历每个场景
    for (int i = 0; i < g_scene_count; i++) {
        cJSON *scene_item = cJSON_GetArrayItem(scenes_array, i);
        Scene *sc = &g_scenes[i];

        // 获取 id
        cJSON *id_json = cJSON_GetObjectItem(scene_item, "id");
        sc->id = id_json ? strdup(id_json->valuestring) : NULL;

        // 获取 background
        cJSON *bg_json = cJSON_GetObjectItem(scene_item, "background");
        sc->background = bg_json ? strdup(bg_json->valuestring) : NULL;

        // 解析 dialogues 数组
        cJSON *dialogs_array = cJSON_GetObjectItem(scene_item, "dialogues");
        sc->dialogue_count = cJSON_GetArraySize(dialogs_array);
        sc->dialogues = (Dialogue*)malloc(sizeof(Dialogue) * sc->dialogue_count);

        for (int j = 0; j < sc->dialogue_count; j++) {
            cJSON *dial_item = cJSON_GetArrayItem(dialogs_array, j);
            Dialogue *dl = &sc->dialogues[j];

            cJSON *speaker = cJSON_GetObjectItem(dial_item, "speaker");
            dl->speaker = speaker ? strdup(speaker->valuestring) : NULL;

            cJSON *text = cJSON_GetObjectItem(dial_item, "text");
            dl->text = text ? strdup(text->valuestring) : NULL;
        }
    }

    // 6. 清理
    cJSON_Delete(root);
    UnloadFileText(json_str);

    TraceLog(LOG_INFO, "Loaded %d scenes from %s", g_scene_count, filename);
}

void UnloadScenes(void) {
    for (int i = 0; i < g_scene_count; i++) {
        Scene *sc = &g_scenes[i];
        free(sc->id);
        free(sc->background);
        for (int j = 0; j < sc->dialogue_count; j++) {
            free(sc->dialogues[j].speaker);
            free(sc->dialogues[j].text);
        }
        free(sc->dialogues);
    }
    free(g_scenes);
    g_scenes = NULL;
    g_scene_count = 0;
}

Scene* GetSceneByID(const char *id) {
    for (int i = 0; i < g_scene_count; i++) {
        if (strcmp(g_scenes[i].id, id) == 0)
            return &g_scenes[i];
    }
    return NULL;
}