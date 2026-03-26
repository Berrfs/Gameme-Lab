/* scene.h — Data structures and API for the scene / dialogue system.
   Scenes, dialogues, and branching choices are loaded from JSON at runtime.
   Code updated by 周沐格, at 11:25AM 2026/03/24 */

#ifndef SCENE_H
#define SCENE_H

#include "raylib.h"
#include <stdlib.h>

/* A single line of dialogue spoken by one character */
typedef struct Dialogue {
    char *speaker;  /* Name of the speaking character */
    char *text;     /* The dialogue text content */
} Dialogue;

/* A branching choice the player can select */
typedef struct Choice {
    char *text;            /* Display text for this choice */
    char *next_scene_id;   /* ID of the scene to jump to */
} Choice;

/* A complete scene containing dialogues and optional choices */
typedef struct Scene {
    char *id;               /* Unique scene identifier */
    char *background;       /* Background image filename */
    Dialogue *dialogues;    /* Array of dialogue lines */
    int dialogue_count;     /* Number of dialogue entries */
    Choice *choices;        /* Array of branching choices */
    int choice_count;       /* Number of choice entries */
    char *next_scene_id;    /* Default next scene ID when no choices exist */
} Scene;

/* Global scene list — populated by LoadScenesFromJSON() */
extern Scene *g_scenes;
extern int g_scene_count;

/* Scene system functions */
void LoadScenesFromJSON(const char *filename);  /* Parse scenes from a JSON file */
void UnloadScenes(void);                        /* Free all scene memory */
Scene* GetSceneByID(const char *id);            /* Look up a scene by its string ID */

#endif
