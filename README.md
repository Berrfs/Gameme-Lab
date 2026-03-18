<div align="center">

# 🎭 No Way!

**A narrative-driven visual novel game built with C and Raylib**

[![Status](https://img.shields.io/badge/Status-Active-2ea44f?style=for-the-badge)]()
[![Year](https://img.shields.io/badge/Year-2026-0366d6?style=for-the-badge)]()
[![Made with](https://img.shields.io/badge/Made_with-Raylib_5.5-e23237?style=for-the-badge)]()
[![Language](https://img.shields.io/badge/Language-C-555555?style=for-the-badge)]()

<br>

*Developed by* **Gameme Lab**

</div>

---

## 🎮 About

**No Way!** is an interactive visual novel game featuring branching storylines, character dialogues, and minigames. Players make choices that shape the narrative and explore different story paths.

### ✨ Key Features

- 🎭 **Branching Narrative** — Multiple story paths driven by player choices
- 💬 **Dynamic Dialogue System** — Character portraits and dialogue boxes with auto/manual advance
- 🕹️ **Minigames** — Integrated minigame modules for gameplay variety
- 🖱️ **Interactive UI** — Hover effects, pointer cursors, and responsive button scaling
- 🔧 **Settings Panel** — Volume control, auto-advance toggle, and interval slider
- 📦 **JSON-based Scenes** — Easily editable story data in `data/scenes.json`

---

## 🏗️ Project Structure

```
No_Way/
├── src/
│   ├── main.c          # Entry point — window init and game loop
│   ├── game.c          # Core logic — state machine, rendering, input
│   ├── game.h          # Game context, states, and public API
│   ├── scene.c         # Scene loader — parses JSON into dialogue/choice data
│   ├── scene.h         # Scene, Dialogue, and Choice data structures
│   ├── minigame.c      # Minigame module — gameplay logic and rendering
│   ├── minigame.h      # Minigame public interface
│   ├── save.c          # Save/load system (WIP)
│   ├── save.h          # Save system interface
│   └── cJSON/          # cJSON library for JSON parsing
├── data/
│   └── scenes.json     # Story script — scenes, dialogues, and choices
├── UI/                 # Textures — backgrounds, buttons, portraits
├── compile.bat         # Build script (MSYS2/ucrt64 + GCC)
├── game.exe            # Compiled game binary
└── README.md
```

---

## 🔧 Tech Stack

| Component | Technology |
| :--- | :--- |
| **Language** | C (GCC via MSYS2/ucrt64) |
| **Graphics** | Raylib 5.5 |
| **JSON Parser** | cJSON |
| **Platform** | Windows |
| **Resolution** | 1280 × 720 |

---

## 🚀 Build & Run

```bash
# Compile the game
.\compile.bat

# Run the game
.\game.exe
```

> **Prerequisite**: [MSYS2](https://www.msys2.org/) with `ucrt64` environment and Raylib installed.

---

## 📋 Project Documentation

### 📝 Stage 1: Conceptualization & Strategy

| Document | Description | Link |
| :--- | :--- | :---: |
| **Rough Plan** | Initial ideation & raw concepts | [🔗 View](https://www.kdocs.cn/l/cdzxmcNtNxf6?from=docs) |
| **Project Plan** | Detailed roadmap & deliverables | [🔗 View](https://www.kdocs.cn/l/ckIK2QcgHzsv) |
| **Project Log** | Meeting summaries & activity tracking | [🔗 View](https://www.kdocs.cn/l/cfQhVSEEViyf?from=docs) |
| **Personal Log** | Individual contribution logs | [🔗 View](https://www.kdocs.cn/l/cfTSfjVqr7rd) |
| **Presentation** | Rough Plan Presentation (Canva) | [🔗 View](https://www.canva.cn/design/DAHDiDCoSmg/j36IBqvwtroE1759VAGbew/edit?utm_content=DAHDiDCoSmg&utm_campaign=designshare&utm_medium=link2&utm_source=sharebutton) |

### 📝 Stage 2: Development

| Document | Description | Link |
| :--- | :--- | :---: |
| **Personal Log** | Development progress & notes | [🔗 View](https://www.kdocs.cn/l/cqYyHC00vOlp) |

---

## 👥 Team

<div align="center">

**Gameme Lab © 2026**

</div>
