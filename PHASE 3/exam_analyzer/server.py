
"""
Local web dashboard for the Exam Question Analyzer.

Run from the exam_analyzer folder:
    python server.py
Then open http://localhost:8000 (it opens automatically).

Uses only Python's standard library - nothing to pip install.
All analysis is done by the C program; this server just runs it and
passes the JSON result to the browser.
"""
import json
import os
import shutil
import subprocess
import sys
import threading
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SRC = ROOT / "src"
WEB = ROOT / "web"
DATA = ROOT / "data"
EXE = ROOT / ("exam_analyzer.exe" if os.name == "nt" else "exam_analyzer")

QUESTIONS = DATA / "questions.txt"
SYLLABUS = DATA / "syllabus.txt"
PREREQS = DATA / "prereqs.txt"
OVERRIDES = DATA / "overrides.txt"   

HOST = "127.0.0.1"                   
PORT = 8000
MAX_BODY = 10_000

lock = threading.Lock()              


def ensure_built():
    """Compile the C backend if the .exe is missing or older than the sources."""
    sources = sorted(SRC.glob("*.c"))
    newest = max(p.stat().st_mtime for p in sources)
    if EXE.exists() and EXE.stat().st_mtime >= newest:
        return
    gcc = shutil.which("gcc")
    if not gcc:
        sys.exit("The C program is not compiled and gcc was not found on PATH.\n"
                 "Compile it first, or add gcc to PATH (see earlier steps).")
    print("Compiling the C backend...")
    cmd = [gcc, "-std=c11", "-O2", "-o", str(EXE), *map(str, sources)]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        sys.exit("Compilation failed:\n" + result.stderr)


def run_analysis():
    """Run the C program with --json and return (http_status, data)."""
    cmd = [str(EXE), str(QUESTIONS), str(SYLLABUS)]
    if PREREQS.exists():
        cmd.append(str(PREREQS))
    cmd.append("--json")
    if OVERRIDES.exists():
        cmd += ["--overrides", str(OVERRIDES)]
    try:
        result = subprocess.run(cmd, capture_output=True, timeout=60)
    except subprocess.TimeoutExpired:
        return 500, {"error": "The analysis took too long."}
    try:
        data = json.loads(result.stdout.decode("utf-8", errors="replace"))
    except json.JSONDecodeError:
        return 500, {"error": "The C program did not return valid JSON.",
                     "details": result.stderr.decode("utf-8", errors="replace")}
    return (200 if result.returncode == 0 else 400), data


def save_review(year, qid, topic_name):
    """Store one decision, replacing any earlier decision for the same question."""
    keep = []
    if OVERRIDES.exists():
        for line in OVERRIDES.read_text(encoding="utf-8").splitlines():
            parts = [p.strip() for p in line.split("|")]
            if len(parts) >= 2 and parts[0] == str(year) and parts[1].lower() == qid.lower():
                continue
            keep.append(line)
    keep.append(f"{year} | {qid} | {topic_name if topic_name else '-'}")
    OVERRIDES.write_text("\n".join(keep) + "\n", encoding="utf-8")


def valid_field(value):
    return isinstance(value, str) and value.strip() and not any(c in value for c in "|\r\n")


class Handler(BaseHTTPRequestHandler):
    def send_json(self, status, data):
        body = json.dumps(data).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path in ("/", "/index.html"):
            body = (WEB / "index.html").read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        elif self.path == "/api/analysis":
            with lock:
                self.send_json(*run_analysis())
        else:
            self.send_json(404, {"error": "Not found"})

    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        if length > MAX_BODY:
            return self.send_json(413, {"error": "Request too large"})
        try:
            body = json.loads(self.rfile.read(length) or b"{}")
        except json.JSONDecodeError:
            return self.send_json(400, {"error": "Invalid JSON"})

        if self.path == "/api/review":
            year, qid, topic = body.get("year"), body.get("qid"), body.get("topic")
            if not isinstance(year, int) or not valid_field(qid) or not (topic is None or valid_field(topic)):
                return self.send_json(400, {"error": "Need year (number), qid and topic (or null)."})
            with lock:
                save_review(year, qid.strip(), topic.strip() if topic else None)
                return self.send_json(*run_analysis())

        if self.path == "/api/reviews/reset":
            with lock:
                if OVERRIDES.exists():
                    OVERRIDES.unlink()
                return self.send_json(*run_analysis())

        self.send_json(404, {"error": "Not found"})

    def log_message(self, fmt, *args):
        pass                                  


def main():
    for f in (QUESTIONS, SYLLABUS, WEB / "index.html"):
        if not f.exists():
            sys.exit(f"Missing file: {f}")
    ensure_built()
    try:
        server = ThreadingHTTPServer((HOST, PORT), Handler)
    except OSError:
        sys.exit(f"Port {PORT} is already in use. Close the other server or change PORT in server.py.")
    url = f"http://localhost:{PORT}"
    print(f"Dashboard running at {url}  (press Ctrl+C to stop)")
    if "--no-browser" not in sys.argv:
        webbrowser.open(url)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopped.")


if __name__ == "__main__":
    main()
