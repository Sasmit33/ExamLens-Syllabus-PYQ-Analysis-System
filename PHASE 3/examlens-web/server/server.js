import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import http from 'node:http';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const WEB_ROOT = path.resolve(here, '..');
const ENGINE_DIR = path.resolve(process.env.EXAM_ANALYZER_DIR || path.join(WEB_ROOT, '..', 'exam_analyzer'));
const SRC_DIR = path.join(ENGINE_DIR, 'src');
const EXE = path.join(ENGINE_DIR, process.platform === 'win32' ? 'exam_analyzer.exe' : 'exam_analyzer');
const DIST = path.join(WEB_ROOT, 'dist');
const UPLOAD_DIR = path.join(WEB_ROOT, 'uploads');

const HOST = '127.0.0.1';                      
const PORT = Number(process.env.PORT || 3000);
const MAX_BODY = 2 * 1024 * 1024;              

const SAMPLE = {
  questions: path.join(ENGINE_DIR, 'data', 'questions.txt'),
  syllabus: path.join(ENGINE_DIR, 'data', 'syllabus.txt'),
  prereqs: path.join(ENGINE_DIR, 'data', 'prereqs.txt'),
};


let source = 'sample';                         
let weights = [35, 25, 20, 20];                

const files = () =>
  source === 'uploaded'
    ? {
        questions: path.join(UPLOAD_DIR, 'questions.txt'),
        syllabus: path.join(UPLOAD_DIR, 'syllabus.txt'),
        prereqs: path.join(UPLOAD_DIR, 'prereqs.txt'),
      }
    : SAMPLE;
const overridesFile = () =>
  path.join(source === 'uploaded' ? UPLOAD_DIR : path.join(ENGINE_DIR, 'data'), 'overrides.txt');


function ensureBuilt() {
  if (!fs.existsSync(path.join(SRC_DIR, 'engine.h'))) {
    console.error(`Backend not found at ${ENGINE_DIR}\nPut examlens-web next to the exam_analyzer folder.`);
    process.exit(1);
  }
  const sources = fs.readdirSync(SRC_DIR).filter((f) => f.endsWith('.c')).map((f) => path.join(SRC_DIR, f));
  const newest = Math.max(...sources.map((f) => fs.statSync(f).mtimeMs));
  if (fs.existsSync(EXE) && fs.statSync(EXE).mtimeMs >= newest) return;

  console.log('Compiling the C engine...');
  const r = spawnSync('gcc', ['-std=c11', '-O2', '-o', EXE, ...sources], { encoding: 'utf8' });
  if (r.error) {
    console.error('gcc was not found on PATH. Add it (see the README) or compile exam_analyzer yourself.');
    process.exit(1);
  }
  if (r.status !== 0) {
    console.error('Compilation failed:\n' + r.stderr);
    process.exit(1);
  }
}

function runAnalysis() {
  const f = files();
  const args = [f.questions, f.syllabus];
  if (fs.existsSync(f.prereqs)) args.push(f.prereqs);
  args.push('--json', '--weights', weights.join(','));
  if (fs.existsSync(overridesFile())) args.push('--overrides', overridesFile());

  const r = spawnSync(EXE, args, { encoding: 'utf8', timeout: 60_000, maxBuffer: 50 * 1024 * 1024 });
  if (r.error) return [500, { error: 'Could not run the C engine: ' + r.error.message }];
  try {
    const data = JSON.parse(r.stdout);
    data.source = source;
    data.weightInputs = weights;
    return [r.status === 0 ? 200 : 400, data];
  } catch {
    return [500, { error: 'The C engine did not return valid JSON.', details: r.stderr }];
  }
}

function saveReview(year, qid, topicName) {
  const file = overridesFile();
  const keep = fs.existsSync(file)
    ? fs.readFileSync(file, 'utf8').split(/\r?\n/).filter((line) => {
        const parts = line.split('|').map((p) => p.trim());
        return line.trim() && !(parts[0] === String(year) && (parts[1] || '').toLowerCase() === qid.toLowerCase());
      })
    : [];
  keep.push(`${year} | ${qid} | ${topicName || '-'}`);
  fs.writeFileSync(file, keep.join('\n') + '\n', 'utf8');
}


const validField = (v) => typeof v === 'string' && v.trim() !== '' && !/[|\r\n]/.test(v);

function readJson(req) {
  return new Promise((resolve, reject) => {
    let size = 0;
    const chunks = [];
    req.on('data', (c) => {
      size += c.length;
      if (size > MAX_BODY) { reject(new Error('too large')); req.destroy(); }
      else chunks.push(c);
    });
    req.on('end', () => {
      try { resolve(chunks.length ? JSON.parse(Buffer.concat(chunks).toString('utf8')) : {}); }
      catch { reject(new Error('invalid json')); }
    });
    req.on('error', reject);
  });
}

function send(res, status, data) {
  const body = JSON.stringify(data);
  res.writeHead(status, { 'Content-Type': 'application/json; charset=utf-8', 'Content-Length': Buffer.byteLength(body) });
  res.end(body);
}

const MIME = {
  '.html': 'text/html; charset=utf-8', '.js': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8', '.svg': 'image/svg+xml', '.png': 'image/png',
  '.ico': 'image/x-icon', '.json': 'application/json',
};

function serveStatic(req, res) {
  if (!fs.existsSync(DIST)) {
    res.writeHead(200, { 'Content-Type': 'text/plain; charset=utf-8' });
    return res.end('The website is not built yet. Run "npm run build" (or use "npm run dev" for development).');
  }
  const urlPath = decodeURIComponent(new URL(req.url, 'http://x').pathname);
  let file = path.normalize(path.join(DIST, urlPath));
  if (!file.startsWith(DIST)) { res.writeHead(403); return res.end(); }        
  if (!fs.existsSync(file) || fs.statSync(file).isDirectory()) file = path.join(DIST, 'index.html');
  res.writeHead(200, { 'Content-Type': MIME[path.extname(file)] || 'application/octet-stream' });
  fs.createReadStream(file).pipe(res);
}


async function handle(req, res) {
  const { pathname } = new URL(req.url, 'http://x');
  if (!pathname.startsWith('/api/')) return serveStatic(req, res);

  try {
    if (req.method === 'GET' && pathname === '/api/analysis') return send(res, ...runAnalysis());

    if (req.method === 'POST' && pathname === '/api/weights') {
      const body = await readJson(req);
      const w = body.weights;
      if (!Array.isArray(w) || w.length !== 4 || !w.every((x) => Number.isFinite(x) && x >= 0 && x <= 100))
        return send(res, 400, { error: 'weights must be four numbers from 0 to 100' });
      weights = w.map(Number);
      return send(res, ...runAnalysis());
    }

    if (req.method === 'POST' && pathname === '/api/review') {
      const { year, qid, topic } = await readJson(req);
      if (!Number.isInteger(year) || !validField(qid) || !(topic === null || validField(topic)))
        return send(res, 400, { error: 'Need year (number), qid and topic (or null).' });
      saveReview(year, qid.trim(), topic ? topic.trim() : null);
      return send(res, ...runAnalysis());
    }

    if (req.method === 'POST' && pathname === '/api/reviews/reset') {
      if (fs.existsSync(overridesFile())) fs.unlinkSync(overridesFile());
      return send(res, ...runAnalysis());
    }

    if (req.method === 'POST' && pathname === '/api/upload') {
      const { questions, syllabus, prereqs } = await readJson(req);
      if (typeof questions !== 'string' || typeof syllabus !== 'string' || !questions.trim() || !syllabus.trim())
        return send(res, 400, { error: 'Upload at least a questions file and a syllabus file.' });
      fs.mkdirSync(UPLOAD_DIR, { recursive: true });
      fs.writeFileSync(path.join(UPLOAD_DIR, 'questions.txt'), questions, 'utf8');
      fs.writeFileSync(path.join(UPLOAD_DIR, 'syllabus.txt'), syllabus, 'utf8');
      const pre = path.join(UPLOAD_DIR, 'prereqs.txt');
      if (typeof prereqs === 'string' && prereqs.trim()) fs.writeFileSync(pre, prereqs, 'utf8');
      else if (fs.existsSync(pre)) fs.unlinkSync(pre);
      const ov = path.join(UPLOAD_DIR, 'overrides.txt');
      if (fs.existsSync(ov)) fs.unlinkSync(ov);                 
      source = 'uploaded';
      return send(res, ...runAnalysis());
    }

    if (req.method === 'POST' && pathname === '/api/use-sample') {
      source = 'sample';
      return send(res, ...runAnalysis());
    }

    return send(res, 404, { error: 'Not found' });
  } catch (err) {
    return send(res, 400, { error: err.message === 'too large' ? 'Request too large' : 'Invalid request' });
  }
}

ensureBuilt();
http.createServer(handle).listen(PORT, HOST, () => {
  console.log(`ExamLens API running at http://localhost:${PORT}`);
  if (fs.existsSync(DIST)) console.log(`Website: http://localhost:${PORT}`);
});
