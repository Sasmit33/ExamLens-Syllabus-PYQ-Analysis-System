# Exam Question Analyzer - Qt dashboard

A Qt 6 Widgets frontend for the C backend in `../exam_analyzer`.

## Folder layout (required)

```
ExamAnalyzerProject/
├── exam_analyzer/      C backend (do not move)
└── ExamAnalyzerUI/     this Qt project
```

CMakeLists.txt finds the backend at `../exam_analyzer/src`.

## Build and run (Windows, Qt Creator)

1. Install Qt with the Qt Online Installer: select Qt 6.x > MinGW 64-bit, plus Qt Creator.
2. Qt Creator > File > Open File or Project > select `ExamAnalyzerUI/CMakeLists.txt`.
3. Choose the Desktop Qt 6.x MinGW 64-bit kit > Configure Project.
4. Press the green Run button (Ctrl+R).
5. In the app, check the three file paths (filled in automatically when found) and press Run analysis.

## How the backend is integrated

- CMakeLists.txt compiles every backend `.c` file except `main.c` into the app.
- `analyzerengine.h/.cpp` wraps the C `Engine` in a C++ class (RAII: constructor creates,
  destructor frees, copying disabled). The UI only talks to this class.
- The backend headers have `extern "C"` guards, so C++ links to the C functions directly.

## Tabs

| Tab | What it shows | Backend functions |
|---|---|---|
| Questions | All questions with topic, confidence, status; search and filters | `e->q`, mapping |
| Review queue | Flagged questions in FIFO order; assign a topic | `engine_set_topic`, then recompute |
| Priority | Ranked topics with score bars and backtest | `engine_rank_topics`, `engine_backtest` |
| Study plan | Topological order; BFS prerequisites and dependents of a topic | `engine_study_order`, `graph_bfs` |
| Repeated questions | Repeat groups across years | repetition groups |
| Data issues | Validation and loading problems | `e->issues` |
