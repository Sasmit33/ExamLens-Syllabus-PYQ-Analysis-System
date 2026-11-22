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

## Interactive features (v2)

| Feature | Where | How it works |
|---|---|---|
| Summary cards | Overview | Questions, topics, pending reviews, repeats, data issues |
| Animated priority chart | Overview | Custom `PriorityChart` widget (QPainter); bars slide to new values; hover for details; click a bar to open its study path |
| What-if weight sliders | Overview | Change marks / years / recency / repeats weights; ranking, study plan and backtest update live (`engine_set_weights`) |
| Prerequisite graph | Study plan | Custom `GraphView` widget; topics in layers, colour = priority; click a topic to highlight its prerequisites (orange) and what it leads to (green) |
| Dark mode | Toolbar, Ctrl+D | Switches the application palette |
| Review shortcuts | Review queue | Ctrl+Enter assign, Ctrl+G accept guess, Ctrl+N no topic |
| Drag and drop | Main window | Drop syllabus.txt / questions.txt / prereqs.txt to load and run |
| Run shortcut | Ctrl+R | Runs the analysis |
| Import PDFs | File bar button, toolbar, Ctrl+I, or drop PDFs | Converts a syllabus PDF and question paper PDFs to text, then runs the analysis |

OOP used in the new widgets: both inherit `QWidget` and override virtual event handlers
(`paintEvent`, `mousePressEvent`, `mouseMoveEvent`, ...) and communicate through signals (`topicClicked`).

## Import PDFs (v3)

The app can start from PDFs instead of hand-written `.txt` files.

1. One-time setup (macOS / Linux):
   ```
   python3 -m venv ~/.venvs/examlens
   ~/.venvs/examlens/bin/pip install -r ../exam_analyzer/tools/requirements.txt
   ```
   On Windows: `pip install pdfplumber` (or the same venv steps with `Scripts\pip`).
2. In the app: **Import PDFs...** (or Ctrl+I, or drag the PDFs onto the window).
3. Pick the syllabus PDF, add the question papers, check the year next to each paper
   (guessed from the file name, e.g. `paper_2024.pdf`), then **Convert and analyze**.

How it works:

- `pdfimportdialog.h/.cpp` is the window that collects the PDFs and years.
- `pdfconverter.h/.cpp` runs `exam_analyzer/tools/pdf_convert.py` with `QProcess`
  (once for the syllabus, once for all papers) and reads its error message on failure.
- The converted `syllabus.txt` and `questions.txt` are saved in the app's data folder
  (shown in the status bar) and loaded through the normal C engine path.
- Python is found in this order: `EXAMLENS_PYTHON` variable, `~/.venvs/examlens`,
  then `python3` / `python` / `py`. The first one with pdfplumber is used.
- Python only converts PDF to text. All analysis stays in C.

Limits: scanned (image-only) PDFs need OCR and show a clear error. A syllabus PDF has
no keyword lists, so check the topics and add keywords after `|` in the converted file if a
question maps to the wrong topic.
