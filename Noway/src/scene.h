#ifndef SCENE_H
#define SCENE_H

#include "raylib.h"
#include <stdlib.h>

// 对话结构
typedef struct Dialogue {
    char *speaker;   // 说话者
    char *text;      // 对话内容
} Dialogue;

// 场景结构（简化版：暂时只有背景和对话，无角色和选项）
typedef struct Scene {
    char *id;               // 场景唯一ID
    char *background;       // 背景图片文件名
    Dialogue *dialogues;    // 对话数组
    int dialogue_count;     // 对话数量
} Scene;

// 全局场景列表
extern Scene *g_scenes;
extern int g_scene_count;

// 函数声明
void LoadScenesFromJSON(const char *filename);
void UnloadScenes(void);
Scene* GetSceneByID(const char *id);

#endif