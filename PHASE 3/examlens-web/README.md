# ExamLens web dashboard (React + Tailwind)

An interactive website for the ExamLens C engine, running on your own computer (localhost).

```
Browser (React + Tailwind)  ->  Node server (server/server.js)  ->  C engine (exam_analyzer --json)
```

All analysis (KMP, Trie, hash map, edit distance, heap, BFS/DFS/topological sort) still happens in C.
The Node server only runs the C program and passes its JSON result to the website.

## Requirements

- **Node.js 18 or newer** (`node --version`)
- **gcc** on PATH (to compile the C engine automatically the first time; on a Mac, `xcode-select --install` provides it)
- **Python 3 + pdfplumber**, only for loading PDFs (see below)
- Folder layout: `examlens-web` must sit **next to** `exam_analyzer`

```
ExamAnalyzerProject/
├── exam_analyzer/     C engine
└── examlens-web/      this website
```

## Run it

```
cd examlens-web
npm install
npm run dev
```

Or one server, no live reload:

```
npm start
```

Press **Ctrl+C** in the terminal to stop.

## Features

| Tab | What you can do |
|---|---|
| Overview | Summary cards; animated priority ranking; **what-if sliders** that re-run the C engine with new weights; live backtest |
| Questions | Search, filter (needs review, no topic, missing marks) and sort every question |
| Review queue | Pick the correct topic from chips; keyboard: arrows, Enter, G, N; decisions are saved and priorities update |
| Study plan | Interactive prerequisite graph: click a topic to highlight what to study first and what it leads to |
| Repeated | Questions repeated across years |
| Data issues | Validation problems in your files |

Also: **Load papers** (PDF files or text files, upload or drag in), **dark mode**, works on phones.

## Load PDFs (Phase 3)

**Load papers** opens on the **PDF files** tab:

1. Choose the syllabus PDF.
2. **Add papers**: one question paper PDF per year. The year is read from the file name (`paper_2024.pdf`); type it if the box is empty.
3. **Convert and analyse**.

The server runs `exam_analyzer/tools/pdf_convert.py` (Python, pdfplumber) to turn the PDFs into
`syllabus.txt` and `questions.txt`, then runs the C engine on them. Python only converts PDF to text;
all analysis stays in C. Prerequisites are not read from PDFs, so the study plan has no arrows for PDF uploads.

One-time Python setup:

```
# macOS / Linux
python3 -m venv ~/.venvs/examlens
~/.venvs/examlens/bin/pip install -r ../exam_analyzer/tools/requirements.txt

# Windows
pip install pdfplumber
```

The server finds Python in this order: `EXAMLENS_PYTHON`, `~/.venvs/examlens`, then `python3` / `python` / `py`,
and uses the first one that has pdfplumber. Scanned (image-only) PDFs need OCR and show a clear error.

## Files

| File | Purpose |
|---|---|
| `server/server.js` | Local API, built-in Node modules only; listens on localhost only |
| `src/App.jsx` | App state, header, tabs, toasts |
| `src/components/*.jsx` | One component per tab, plus the upload dialog |
| `src/utils.js` | Helpers, including the graph layout |
| `src/index.css` | Tailwind import and theme colours |

## API (used by the website)

| Method | Path | Body | Does |
|---|---|---|---|
| GET | `/api/analysis` | | Run the engine and return JSON |
| POST | `/api/weights` | `{ "weights": [35,25,20,20] }` | Re-run with new priority weights |
| POST | `/api/review` | `{ "year": 2022, "qid": "Q2", "topic": "Stacks" }` | Save a review decision (`topic: null` = no topic) |
| POST | `/api/reviews/reset` | | Clear saved decisions |
| POST | `/api/upload` | `{ "questions": "...", "syllabus": "...", "prereqs": "..." }` | Analyse uploaded files |
| POST | `/api/upload-pdf` | `{ "syllabus": {name, data}, "papers": [{name, year, data}] }` (data = base64) | Convert the PDFs with the Python parser, then analyse |
| POST | `/api/use-sample` | | Switch back to the sample data |

## Troubleshooting

| Problem | Fix |
|---|---|
| `gcc was not found on PATH` | Run `$env:Path += ";C:\msys64\ucrt64\bin"` (Windows) in the same terminal first |
| `Backend not found` | Put `examlens-web` next to `exam_analyzer` |
| `npm` is not recognized | Install Node.js LTS from nodejs.org and restart VS Code |
| Port already in use | Close the other terminal running the server, or set `PORT=3001` |
| "Cannot reach the local server" in the page | The terminal running `npm run dev` was closed; start it again |
| `Cannot find module @rollup/rollup-...` | `node_modules` was copied from another computer. Delete it and run `npm install` again |
| "pdfplumber is not installed" when loading PDFs | Do the one-time Python setup above, then try again (no restart needed) |
