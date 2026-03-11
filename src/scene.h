#ifndef SCENE_H
#define SCENE_H

#include "raylib.h"
#include <stdlib.h>

// 对话结构
typedef struct Dialogue {
    char *speaker;
    char *text;
} Dialogue;

// 选项结构
typedef struct Choice {
    char *text;            // 选项文本
    char *next_scene_id;   // 跳转的场景ID
} Choice;

// 场景结构
typedef struct Scene {
    char *id;
    char *background;
    Dialogue *dialogues;
    int dialogue_count;
    Choice *choices;        // 选项数组
    int choice_count;       // 选项数量
} Scene;

// 全局场景列表
extern Scene *g_scenes;
extern int g_scene_count;

// 函数声明
void LoadScenesFromJSON(const char *filename);
void UnloadScenes(void);
Scene* GetSceneByID(const char *id);

#endif