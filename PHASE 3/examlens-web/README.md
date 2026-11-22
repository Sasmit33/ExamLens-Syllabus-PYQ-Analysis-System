# ExamLens web dashboard (React + Tailwind)

An interactive website for the ExamLens C engine, running on your own computer (localhost).

```
Browser (React + Tailwind)  ->  Node server (server/server.js)  ->  C engine (exam_analyzer --json)
```

All analysis (KMP, Trie, hash map, edit distance, heap, BFS/DFS/topological sort) still happens in C.
The Node server only runs the C program and passes its JSON result to the website.

## Requirements

- **Node.js 18 or newer** (`node --version`)
- **gcc** on PATH (to compile the C engine automatically the first time)
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

Also: **Load papers** (upload or drag in your own questions/syllabus/prereqs files), **dark mode**, works on phones.

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
| POST | `/api/use-sample` | | Switch back to the sample data |

## Troubleshooting

| Problem | Fix |
|---|---|
| `gcc was not found on PATH` | Run `$env:Path += ";C:\msys64\ucrt64\bin"` (Windows) in the same terminal first |
| `Backend not found` | Put `examlens-web` next to `exam_analyzer` |
| `npm` is not recognized | Install Node.js LTS from nodejs.org and restart VS Code |
| Port already in use | Close the other terminal running the server, or set `PORT=3001` |
| "Cannot reach the local server" in the page | The terminal running `npm run dev` was closed; start it again |
