#!/usr/bin/env python3
"""
pdf_convert.py - turns a question paper PDF or a syllabus PDF into the
plain-text format the C engine reads. The C engine does all the analysis;
this script only extracts and tidies the text.

Usage:
    python pdf_convert.py --type syllabus syllabus.pdf -o data/syllabus.txt
    python pdf_convert.py --type questions --year 2023,2024 p2023.pdf p2024.pdf -o data/questions.txt

Then open the .txt files in the Qt app (Browse) and press Run analysis.
Without -o the text is printed to the screen. Errors go to stderr with exit code 1.

Needs: pip install pdfplumber
"""
import argparse
import re
import sys

try:
    import pdfplumber
except ImportError:
    sys.exit("pdfplumber is not installed. Run: pip install pdfplumber")

# ---------- shared cleanup ----------

CHAR_FIXES = {
    "ﬁ": "fi", "ﬂ": "fl", "ﬀ": "ff", "ﬃ": "ffi", "ﬄ": "ffl",
    "‘": "'", "’": "'", "“": '"', "”": '"',
    "–": "-", "—": "-", "−": "-", "•": " ", "●": " ",
    " ": " ", "\t": " ", "|": "/",           # a pipe would break the field format
}


def clean(line):
    for bad, good in CHAR_FIXES.items():
        line = line.replace(bad, good)
    return re.sub(r"\s{2,}", " ", line).strip()


def pdf_lines(path):
    """Every non-empty line of every page, cleaned."""
    try:
        with pdfplumber.open(path) as pdf:
            for page in pdf.pages:
                for raw in (page.extract_text() or "").splitlines():
                    line = clean(raw)
                    if line:
                        yield line
    except Exception as err:  # bad or encrypted PDF
        sys.exit(f"Could not read PDF: {err}")


# ---------- question papers ----------

JUNK = re.compile(
    r"^(page\s*\d+(\s*of\s*\d+)?|roll\s*no|time\s*[:\-]|max(imum)?\.?\s*marks|total\s*marks|"
    r"note\s*[:\-]|attempt\s+(all|any)|all\s+questions|end\s+semester|mid\s*semester|"
    r"b\.?\s*tech|university|institute|college|subject\s*(code|name)?\s*[:\-]|"
    r"course\s*(code|name)?\s*[:\-]|semester\s*[:\-]?\s*[ivx0-9]+|[-_=]{3,})",
    re.IGNORECASE,
)
LABEL = re.compile(r"^(?:Q\.?\s*)?(\d{1,3})\s*[.)]?\s*(?:\(?([a-h])\))?\s*[.):\-]?\s+", re.IGNORECASE)
SUBPART = re.compile(r"^\(?([a-h])\)\s+", re.IGNORECASE)
MARKS = re.compile(
    r"\s*[(\[]\s*(\d{1,3})\s*(?:marks?|m)?\s*[)\]]\s*$|\s*(\d{1,3})\s*marks?\s*$",
    re.IGNORECASE,
)
OR_LINE = re.compile(r"^\(?\s*OR\s*\)?$", re.IGNORECASE)


def group_questions(lines):
    """Join wrapped lines so each block is one question. '<OR>' marks the alternative."""
    blocks, current, after_or = [], None, False
    for line in lines:
        if JUNK.match(line):
            continue
        if OR_LINE.match(line):
            if current:
                blocks.append(current)
            current, after_or = None, True
            continue
        if LABEL.match(line) or SUBPART.match(line) or after_or:
            if current:
                blocks.append(current)
            current = ("<OR>" if after_or else "") + line
            after_or = False
        elif current is not None:
            current += " " + line
    if current:
        blocks.append(current)
    return blocks


def parse_block(block, last_main):
    marks = ""
    m = MARKS.search(block)
    if m:
        marks = next(g for g in m.groups() if g)
        block = block[: m.start()].rstrip()
    m = LABEL.match(block)
    if m:
        main, sub = m.group(1), (m.group(2) or "").lower()
        return main + sub, marks, block[m.end():].strip(), main
    m = SUBPART.match(block)
    if m:
        return last_main + m.group(1).lower(), marks, block[m.end():].strip(), last_main
    return None, marks, block.strip(), last_main


def convert_questions(path, year):
    out = [f"YEAR: {year}"]
    last_main, count = "0", 0
    for block in group_questions(pdf_lines(path)):
        is_or = block.startswith("<OR>")
        if is_or:
            block = block[4:]
        label, marks, text, main = parse_block(block, last_main)
        if not text:
            continue
        if is_or:
            label = (label or last_main) + "-or"
        elif label:
            last_main = main
        else:
            label = "?"                      # the C validator will report it
        out.append(f"Q{label} | {marks} | {text}")
        count += 1
    if count == 0:
        sys.exit("No questions found. The PDF may be a scanned image (needs OCR).")
    return "\n".join(out) + "\n"


# ---------- syllabus ----------

UNIT = re.compile(r"^unit\s*[-:.]?\s*([ivx]+|\d+)\b\s*[-:.]?\s*(.*)$", re.IGNORECASE)
ROMAN = {"i": 1, "ii": 2, "iii": 3, "iv": 4, "v": 5, "vi": 6, "vii": 7, "viii": 8, "ix": 9, "x": 10}
SKIP_SYL = re.compile(
    r"^(course\s*(code|name|outcomes?)|credits?|l\s*t\s*p|text\s*books?|reference|"
    r"co\s*\d|page\s*\d|syllabus|contact\s*hours|hours?\s*[:\-]?\s*\d+$)",
    re.IGNORECASE,
)


def split_topics(text):
    """'Stacks: push, pop; Queues - circular queue' -> ['Stacks', 'push', 'pop', ...]"""
    text = re.sub(r"\(\s*\d+\s*(hrs?|hours?|l)\s*\)", "", text, flags=re.IGNORECASE)
    parts = re.split(r"[,;:]|\s-\s|\.\s", text)
    topics = []
    for p in parts:
        p = p.strip(" .-")
        if 3 <= len(p) <= 80 and re.search(r"[A-Za-z]", p):
            topics.append(p)
    return topics


FILLER = re.compile(r"^(introduction\s+to|basics\s+of|overview\s+of|concepts?\s+of|types\s+of|"
                    r"applications?\s+of|implementation\s+of)\s+", re.IGNORECASE)

# Common short forms in CS syllabi. Students write these; syllabi spell them out.
DEFAULT_ALIASES = [
    "bst = binary search tree", "bfs = breadth first search", "dfs = depth first search",
    "dp = dynamic programming", "ll = linked list", "mst = minimum spanning tree",
]


def singular(phrase):
    """'binary search trees' -> 'binary search tree' (simple rule, no dictionary)."""
    return " ".join(w[:-1] if len(w) > 3 and w.endswith("s") and not w.endswith("ss") else w
                    for w in phrase.split())


def keywords_for(topic):
    """Extra keywords so a question that says 'stack' matches the topic 'Stacks'."""
    base = FILLER.sub("", topic.lower()).strip()
    kws = []
    for k in (base, singular(base)):
        if k and k != topic.lower() and k not in kws:
            kws.append(k)
    return kws


def convert_syllabus(path):
    out = [f"ALIAS: {a}" for a in DEFAULT_ALIASES]
    unit, seen, count = 0, set(), 0
    for line in pdf_lines(path):
        m = UNIT.match(line)
        if m:
            u = m.group(1).lower()
            unit = ROMAN.get(u, int(u) if u.isdigit() else unit + 1)
            out.append(f"UNIT: {unit}")
            line = m.group(2)
            if not line:
                continue
        if unit == 0 or SKIP_SYL.match(line):
            continue                         # text before Unit 1 is course info
        for topic in split_topics(line):
            key = topic.lower()
            if key in seen:
                continue
            seen.add(key)
            kws = keywords_for(topic)
            out.append(f"TOPIC: {topic}" + (" | " + ", ".join(kws) if kws else ""))
            count += 1
    if count == 0:
        sys.exit("No units or topics found. Check that the PDF has 'Unit 1' style headings.")
    return "\n".join(out) + "\n"


def main():
    ap = argparse.ArgumentParser(description="Convert syllabus / question paper PDFs to ExamLens text files.")
    ap.add_argument("pdfs", nargs="+", help="one syllabus PDF, or one or more question paper PDFs")
    ap.add_argument("--type", choices=["questions", "syllabus"], required=True)
    ap.add_argument("--year", help="years for the question PDFs, comma-separated, same order (e.g. 2023,2024)")
    ap.add_argument("-o", "--output", help="output .txt file (default: print to screen)")
    args = ap.parse_args()

    if args.type == "syllabus":
        if len(args.pdfs) != 1:
            sys.exit("Give exactly one syllabus PDF.")
        text = convert_syllabus(args.pdfs[0])
    else:
        try:
            years = [int(y) for y in args.year.split(",")] if args.year else []
        except ValueError:
            sys.exit("--year must be numbers separated by commas, e.g. --year 2023,2024")
        if len(years) != len(args.pdfs):
            sys.exit(f"Give one --year per question PDF ({len(args.pdfs)} PDFs, {len(years)} years).")
        for y in years:
            if not 1900 <= y <= 2100:
                sys.exit(f"Year {y} looks wrong.")
        text = "\n".join(convert_questions(pdf, y) for pdf, y in zip(args.pdfs, years))

    if args.output:
        with open(args.output, "w", encoding="utf-8", newline="\n") as f:
            f.write(text)
        print(f"Wrote {args.output}", file=sys.stderr)
    else:
        sys.stdout.write(text)


if __name__ == "__main__":
    main()
