/* scene.c — Scene loading, cleanup, and lookup implementation.
   Parses scene data (dialogues + choices) from a JSON file using cJSON.
   Code updated by 周沐格, at 08:21PM 2026/03/15 */

#include "scene.h"
#include "cJSON/cJSON.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>

/* Global scene storage */
Scene *g_scenes = NULL;
int g_scene_count = 0;

/* Load every scene from a JSON file into the global g_scenes array */
void LoadScenesFromJSON(const char *filename) {
    /* Read the entire file into a string using raylib's helper */
    char *json_str = LoadFileText(filename);
    if (!json_str) {
        TraceLog(LOG_ERROR, "Failed to load scenes file: %s", filename);
        return;
    }

    /* Parse the JSON string into a cJSON tree */
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        TraceLog(LOG_ERROR, "JSON parse error in %s", filename);
        UnloadFileText(json_str);
        return;
    }

    /* Retrieve the top-level "scenes" array */
    cJSON *scenes_array = cJSON_GetObjectItem(root, "scenes");
    if (!cJSON_IsArray(scenes_array)) {
        TraceLog(LOG_ERROR, "No scenes array found");
        cJSON_Delete(root);
        UnloadFileText(json_str);
        return;
    }

    /* Allocate memory for all scenes at once */
    g_scene_count = cJSON_GetArraySize(scenes_array);
    g_scenes = (Scene*)malloc(sizeof(Scene) * g_scene_count);

    for (int i = 0; i < g_scene_count; i++) {
        cJSON *scene_item = cJSON_GetArrayItem(scenes_array, i);
        Scene *sc = &g_scenes[i];

        /* Parse scene id */
        cJSON *id_json = cJSON_GetObjectItem(scene_item, "id");
        sc->id = id_json ? strdup(id_json->valuestring) : NULL;

        /* Parse background image filename */
        cJSON *bg_json = cJSON_GetObjectItem(scene_item, "background");
        sc->background = bg_json ? strdup(bg_json->valuestring) : NULL;

        /* 【新增】：解析无选项时的下一个场景 (next_scene) */
        cJSON *next_scene_json = cJSON_GetObjectItem(scene_item, "next_scene");
        sc->next_scene_id = next_scene_json ? strdup(next_scene_json->valuestring) : NULL;

        /* Parse the dialogues array for this scene */
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

        /* Parse the choices array for this scene */
        cJSON *choices_array = cJSON_GetObjectItem(scene_item, "choices");
        sc->choice_count = cJSON_GetArraySize(choices_array);
        sc->choices = (Choice*)malloc(sizeof(Choice) * sc->choice_count);
        for (int j = 0; j < sc->choice_count; j++) {
            cJSON *choice_item = cJSON_GetArrayItem(choices_array, j);
            Choice *ch = &sc->choices[j];
            cJSON *text = cJSON_GetObjectItem(choice_item, "text");
            ch->text = text ? strdup(text->valuestring) : NULL;
            cJSON *next = cJSON_GetObjectItem(choice_item, "next_scene");
            ch->next_scene_id = next ? strdup(next->valuestring) : NULL;
        }
    }

    /* Clean up the cJSON tree and the raw file text */
    cJSON_Delete(root);
    UnloadFileText(json_str);
    TraceLog(LOG_INFO, "Loaded %d scenes from %s", g_scene_count, filename);
}

/* Free all heap-allocated scene data */
void UnloadScenes(void) {
    for (int i = 0; i < g_scene_count; i++) {
        Scene *sc = &g_scenes[i];
        free(sc->id);
        free(sc->background);
        free(sc->next_scene_id); /* 【新增】：释放 next_scene_id 占用的内存 */

        /* Free every dialogue's strings, then the dialogues array itself */
        for (int j = 0; j < sc->dialogue_count; j++) {
            free(sc->dialogues[j].speaker);
            free(sc->dialogues[j].text);
        }
        free(sc->dialogues);

        /* Free every choice's strings, then the choices array itself */
        for (int j = 0; j < sc->choice_count; j++) {
            free(sc->choices[j].text);
            free(sc->choices[j].next_scene_id);
        }
        free(sc->choices);
    }
    free(g_scenes);
    g_scenes = NULL;
    g_scene_count = 0;
}

/* Linear search for a scene by its string ID. Returns NULL if not found */
Scene* GetSceneByID(const char *id) {
    for (int i = 0; i < g_scene_count; i++) {
        if (g_scenes[i].id != NULL && strcmp(g_scenes[i].id, id) == 0)
            return &g_scenes[i];
    }
    return NULL;
}
