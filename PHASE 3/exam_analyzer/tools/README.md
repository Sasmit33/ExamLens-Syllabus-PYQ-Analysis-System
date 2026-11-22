# PDF parser

Turns a syllabus PDF and question paper PDFs into the text files the C engine and the Qt app read.
Python only converts PDF to text. All analysis stays in C.

## Setup (once)

```
pip install -r requirements.txt
```

## Convert

Run from the `exam_analyzer` folder:

```
python tools/pdf_convert.py --type syllabus syllabus.pdf -o data/syllabus.txt
python tools/pdf_convert.py --type questions --year 2023,2024 paper_2023.pdf paper_2024.pdf -o data/questions.txt
```

- One syllabus PDF. It needs headings like `Unit 1` or `Unit I`.
- One or more question PDFs, with one year each, in the same order.
- Then open the Qt app, Browse to the two .txt files, and press Run analysis.

## What it handles

Question papers: headers, page numbers and instructions removed; wrapped lines joined;
labels `Q1.`, `Q1(a)`, `(b)`, `OR`; marks written as `(5 marks)`, `[5]` or `(5)`; blank marks left blank.

Syllabus: `Unit I` / `Unit 1` headings; topics split on commas and semicolons;
keywords added automatically (Stacks → stack); common aliases (bst, bfs, dfs).

## Limits

- Scanned PDFs (photos) have no text. They need OCR, which is not included.
- Open the .txt files once and check them. Add keywords after `|` on a TOPIC line to improve mapping.
