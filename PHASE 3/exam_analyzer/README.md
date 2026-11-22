# Exam Question Analyzer — C backend

Reads past question papers (already in text form), maps every question to a syllabus topic, finds questions repeated across years, scores topic priority, and produces a study order that respects prerequisites.

## Build and run

```
make
make run
make test
```

Without `make` (Windows/MinGW works too):

```
gcc -std=c11 -O2 -o exam_analyzer src/*.c
./exam_analyzer data/questions.txt data/syllabus.txt data/prereqs.txt
```

The prerequisites file is optional.

## Web dashboard (localhost)

```
python server.py
```

Opens http://localhost:8000 in your browser. Needs only Python 3 (no packages to install).
The server compiles the C program automatically if gcc is on PATH, runs it with `--json`,
and saves review decisions to `data/overrides.txt`. Press Ctrl+C in the terminal to stop it.

- `server.py`: local server (Python standard library only, listens on localhost only)
- `web/index.html`: the dashboard (HTML, CSS and JavaScript, works offline)
- `src/json_out.c`: writes all results as JSON; every calculation still happens in C
- `src/overrides.c`: applies saved review decisions (`YEAR | QID | Topic`)

## Input formats

**questions.txt** — the team's existing format. `YEAR:` applies to every line below it. A blank marks field means missing. `Q3-or` is the alternative to `Q3`.

```
YEAR: 2024
Q1a | 5 | Define a data structure. Differentiate linear and non-linear structures.
Q3-or | 5 | Construct an AVL tree for 10, 20, 30, 40, 50 showing rotations.
Q4  |   | Write the Merge Sort algorithm and show its working on 38, 27, 43, 3, 9.
```

**syllabus.txt** — this format was chosen for the backend; adapt `parser.c` if yours differs.

```
ALIAS: bst = binary search tree
UNIT: 4
TOPIC: AVL trees | avl, avl tree, rotation, rotations, balance factor
```

**prereqs.txt** — `Topic A -> Topic B` means study A before B. Topic names must match the syllabus.

## Pipeline and files

| Stage | File | Data structures used |
|---|---|---|
| Parse + normalize (replaces the PDF step) | `parser.c`, `util.c`, `engine.c` | hash map (aliases, topic index) |
| Validation | `validate.c` | hash map (duplicate and id detection) |
| Question → topic mapping, review queue | `mapper.c` | KMP (phrases), Trie (words), edit distance (typos), FIFO queue |
| Repetition detection | `repetition.c` | word-level edit distance, union-find |
| Analytics, priority, ranking | `analytics.c` | max-heap |
| Prerequisites, study order | `graph.c` | adjacency list, BFS, DFS cycle detection, Kahn's algorithm with a max-heap |
| Core structures | `hashmap.c`, `trie.c`, `kmp.c`, `editdist.c`, `heap.c`, `queue.c` | — |

## Key design decisions

- **Raw and normalized text are both kept.** Normalized text is for matching; raw text is for display.
- **Missing marks are stored as -1, not 0.** `atoi("")` returns 0, which would silently create a 0-mark question.
- **`split_fields` instead of `strtok`.** `strtok` merges `||` and loses an empty field.
- **The same normalization is applied to questions, keywords and topic names**, including alias expansion. Otherwise matching fails silently.
- **Longest phrase first, with covered spans.** "binary search tree" is matched once; "binary search" and "tree" inside it are not counted again.
- **Confidence = best / (best + second best).** Ties give 0.5 and go to the review queue instead of being silently assigned.
- **Duplicates vs repetition.** The same text twice in *one* year is a data error and is ignored. The same question across *different* years is a signal and is grouped.
- **OR pairs count 0.5 each**, because a student answers only one.
- **Prerequisite cycles are reported**, not ignored. Topics stuck in a cycle are left out of the study order.

## Tunable constants (assumptions — tune and re-test)

| Constant | File | Default |
|---|---|---|
| `MIN_CONFIDENCE`, `MIN_SCORE` | `mapper.c` | 0.70, 1.0 |
| `FUZZY_MIN_LEN` | `mapper.c` | 6 |
| `SIM_THRESHOLD` | `repetition.c` | 0.60 |
| `W_MARKS`, `W_COVERAGE`, `W_RECENCY`, `W_REPEAT` | `analytics.c` | 0.35, 0.25, 0.20, 0.20 |

## Evaluation

`engine_backtest()` trains on all years before the latest one, then measures what share of the latest year's marks falls in the top-k topics. The baseline is ranking by past marks alone. Repeat-group credit is restricted to the training years so the test year does not leak in.

On the sample data: top-3 by priority covers 62% of 2024's marks, versus 38% for the baseline. The sample has only 3 years and 13 topics, so treat this as a sanity check. Re-run it on your real data.

## Known limitations

- Mapping is keyword-based. Questions spanning two topics (e.g. "time complexity of binary search") tie and need a human decision. That is why the review queue exists.
- Keyword lists decide quality. A topic with few keywords (Binary search trees in the sample) loses ties.
- The fuzzy fallback can create false matches. During testing, "notations" matched "rotations" at edit distance 1; the first-letter rule was added for that reason.
- Questions in the test year are labelled by the automatic mapper, so backtest results inherit mapping errors.
- Repetition comparison is O(Q² · L²). That is fine for a few hundred questions; for thousands, compare only questions that share a topic.
- Matching every phrase with KMP costs O(P · n) per question. Aho–Corasick (KMP failure links built on a Trie) does all phrases in one pass.

## Using it from the Qt dashboard

Every header has `extern "C"` guards, so C++ can call the backend directly. Add the `src/*.c` files except `main.c` to the Qt project:

```cpp
#include "engine.h"

Engine *e = engine_create();
engine_load_syllabus(e, "syllabus.txt");     
engine_load_questions(e, "questions.txt");
engine_load_prereqs(e, "prereqs.txt");
engine_validate(e);
engine_map_topics(e);
engine_detect_repetition(e);
engine_compute_analytics(e, e->max_year);
engine_study_order(e);



engine_free(e);
```
