import { useEffect, useState } from 'react';
import { cx } from '../utils.js';

const FIELDS = [
  { key: 'questions', label: 'Questions file', note: 'YEAR: lines, then QID | marks | text', required: true },
  { key: 'syllabus', label: 'Syllabus file', note: 'UNIT:, TOPIC: and ALIAS: lines', required: true },
  { key: 'prereqs', label: 'Prerequisites file', note: 'Topic A -> Topic B (optional)', required: false },
];

const MAX_PDF_BYTES = 30 * 1024 * 1024;

// "paper_2024.pdf" or "DS-Dec-2023.pdf" -> 2024 / 2023; '' when there is no year in the name.
const guessYear = (name) => {
  const m = name.match(/(?:^|\D)((?:19|20)\d\d)(?:\D|$)/);
  return m ? m[1] : '';
};

const isPdf = (file) => file.type === 'application/pdf' || file.name.toLowerCase().endsWith('.pdf');

// File -> base64 text, so the PDF can travel inside the JSON request.
async function toBase64(file) {
  const bytes = new Uint8Array(await file.arrayBuffer());
  let binary = '';
  for (let i = 0; i < bytes.length; i += 0x8000) binary += String.fromCharCode(...bytes.subarray(i, i + 0x8000));
  return btoa(binary);
}

export default function UploadDialog({ busy, error, onClose, onUpload, onUploadPdf, onSample }) {
  const [mode, setMode] = useState('pdf');
  const [dragging, setDragging] = useState(false);
  const [hint, setHint] = useState('');

  // Text mode
  const [files, setFiles] = useState({});
  const [names, setNames] = useState({});

  // PDF mode
  const [syllabusPdf, setSyllabusPdf] = useState(null);
  const [papers, setPapers] = useState([]);   // { id, file, year }

  useEffect(() => {
    const onKey = (e) => e.key === 'Escape' && !busy && onClose();
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [onClose, busy]);

  const readText = async (key, file) => {
    if (!file) return;
    const text = await file.text();
    setFiles((f) => ({ ...f, [key]: text }));
    setNames((n) => ({ ...n, [key]: file.name }));
  };

  const addPapers = (list) => {
    const pdfs = [...list].filter(isPdf);
    if (pdfs.length < list.length) setHint('Only PDF files can be added as question papers.');
    setPapers((p) => [
      ...p,
      ...pdfs.map((file) => ({ id: `${file.name}-${file.size}-${Math.random()}`, file, year: guessYear(file.name) })),
    ]);
  };

  const onDrop = async (e) => {
    e.preventDefault();
    setDragging(false);
    setHint('');
    const dropped = [...e.dataTransfer.files];
    if (mode === 'pdf') {
      const syl = dropped.find((f) => isPdf(f) && f.name.toLowerCase().includes('syllabus'));
      if (syl) setSyllabusPdf(syl);
      addPapers(dropped.filter((f) => f !== syl));
      return;
    }
    for (const file of dropped) {
      const n = file.name.toLowerCase();
      await readText(n.includes('syllabus') ? 'syllabus' : n.includes('prereq') ? 'prereqs' : 'questions', file);
    }
  };

  const convertPdfs = async () => {
    setHint('');
    if (!syllabusPdf) return setHint('Choose the syllabus PDF.');
    if (papers.length === 0) return setHint('Add at least one question paper PDF.');
    const missing = papers.find((p) => !/^(19|20)\d\d$/.test(String(p.year).trim()));
    if (missing) return setHint(`Enter the exam year for ${missing.file.name}.`);
    const total = syllabusPdf.size + papers.reduce((s, p) => s + p.file.size, 0);
    if (total > MAX_PDF_BYTES) return setHint('These PDFs are larger than 30 MB in total. Try fewer papers.');

    onUploadPdf({
      syllabus: { name: syllabusPdf.name, data: await toBase64(syllabusPdf) },
      papers: await Promise.all(
        papers.map(async (p) => ({ name: p.file.name, year: Number(p.year), data: await toBase64(p.file) })),
      ),
    });
  };

  const textReady = files.questions && files.syllabus;
  const button = 'rounded-lg border border-ink-200 px-3 py-1.5 text-sm hover:bg-ink-50 dark:border-ink-700 dark:hover:bg-ink-800';

  return (
    <div className="fixed inset-0 z-40 flex items-center justify-center bg-ink-950/50 p-4" onClick={() => !busy && onClose()}>
      <div
        role="dialog" aria-modal="true" aria-label="Load papers"
        onClick={(e) => e.stopPropagation()}
        className="animate-rise max-h-[90vh] w-full max-w-lg overflow-y-auto rounded-2xl bg-white p-6 shadow-xl dark:bg-ink-900"
      >
        <h2 className="font-serif text-xl font-semibold">Load your papers</h2>

        <div className="mt-3 inline-flex rounded-lg border border-ink-200 p-0.5 text-sm dark:border-ink-700" role="tablist">
          {[['pdf', 'PDF files'], ['text', 'Text files']].map(([id, label]) => (
            <button
              key={id}
              role="tab"
              aria-selected={mode === id}
              onClick={() => { setMode(id); setHint(''); }}
              className={cx('rounded-md px-3 py-1', mode === id ? 'bg-ink-800 text-white dark:bg-ink-100 dark:text-ink-900' : 'text-ink-600 dark:text-ink-300')}
            >
              {label}
            </button>
          ))}
        </div>

        <p className="mt-2 text-sm text-ink-500 dark:text-ink-300">
          {mode === 'pdf'
            ? 'Choose the syllabus PDF and the question paper PDFs, or drop them all into the box.'
            : 'Choose the text files, or drop them all into the box below.'}
        </p>

        <div
          onDragOver={(e) => { e.preventDefault(); setDragging(true); }}
          onDragLeave={() => setDragging(false)}
          onDrop={onDrop}
          className={`mt-4 space-y-3 rounded-xl border-2 border-dashed p-4 transition ${dragging ? 'border-ink-500 bg-ink-50 dark:bg-ink-800' : 'border-ink-200 dark:border-ink-700'}`}
        >
          {mode === 'text' && FIELDS.map((f) => (
            <label key={f.key} className="flex items-center justify-between gap-3">
              <span>
                <span className="block text-sm font-medium">{f.label}{f.required && <span className="text-pen-500"> *</span>}</span>
                <span className="block text-xs text-ink-500 dark:text-ink-300">{names[f.key] || f.note}</span>
              </span>
              <span className={cx('cursor-pointer', button)}>
                {names[f.key] ? 'Change' : 'Choose'}
                <input type="file" accept=".txt,text/plain" className="sr-only" onChange={(e) => readText(f.key, e.target.files[0])} />
              </span>
            </label>
          ))}

          {mode === 'pdf' && (
            <>
              <label className="flex items-center justify-between gap-3">
                <span className="min-w-0">
                  <span className="block text-sm font-medium">Syllabus PDF<span className="text-pen-500"> *</span></span>
                  <span className="block truncate text-xs text-ink-500 dark:text-ink-300">{syllabusPdf ? syllabusPdf.name : 'Units and topics'}</span>
                </span>
                <span className={cx('shrink-0 cursor-pointer', button)}>
                  {syllabusPdf ? 'Change' : 'Choose'}
                  <input
                    type="file" accept=".pdf,application/pdf" className="sr-only"
                    onChange={(e) => { const f = e.target.files[0]; if (f) setSyllabusPdf(f); e.target.value = ''; }}
                  />
                </span>
              </label>

              <div>
                <div className="flex items-center justify-between gap-3">
                  <span>
                    <span className="block text-sm font-medium">Question papers<span className="text-pen-500"> *</span></span>
                    <span className="block text-xs text-ink-500 dark:text-ink-300">One PDF per year; check each year</span>
                  </span>
                  <span className={cx('shrink-0 cursor-pointer', button)}>
                    Add papers
                    <input
                      type="file" accept=".pdf,application/pdf" multiple className="sr-only"
                      onChange={(e) => { addPapers(e.target.files); e.target.value = ''; }}
                    />
                  </span>
                </div>

                {papers.length > 0 && (
                  <ul className="mt-2 space-y-1.5">
                    {papers.map((p) => (
                      <li key={p.id} className="flex items-center gap-2 rounded-lg bg-ink-50 px-2.5 py-1.5 text-sm dark:bg-ink-800">
                        <span className="min-w-0 flex-1 truncate" title={p.file.name}>{p.file.name}</span>
                        <input
                          aria-label={`Exam year for ${p.file.name}`}
                          inputMode="numeric" maxLength={4} placeholder="Year"
                          value={p.year}
                          onChange={(e) => {
                            const year = e.target.value.replace(/\D/g, '');
                            setPapers((all) => all.map((x) => (x.id === p.id ? { ...x, year } : x)));
                          }}
                          className="w-16 rounded-md border border-ink-200 bg-white px-2 py-1 text-center dark:border-ink-700 dark:bg-ink-900"
                        />
                        <button
                          onClick={() => setPapers((all) => all.filter((x) => x.id !== p.id))}
                          aria-label={`Remove ${p.file.name}`}
                          className="px-1 text-ink-500 hover:text-pen-500 dark:text-ink-300"
                        >
                          ×
                        </button>
                      </li>
                    ))}
                  </ul>
                )}
              </div>
            </>
          )}
        </div>

        {(hint || (error && !busy)) && (
          <p role="alert" className="mt-3 whitespace-pre-wrap text-sm text-pen-600 dark:text-pen-100">{hint || error}</p>
        )}

        <div className="mt-5 flex flex-wrap items-center gap-2">
          {mode === 'pdf' ? (
            <button
              disabled={busy}
              onClick={convertPdfs}
              className="rounded-lg bg-ink-800 px-4 py-2 text-sm font-medium text-white hover:bg-ink-700 disabled:opacity-40 dark:bg-ink-100 dark:text-ink-900"
            >
              {busy ? 'Converting…' : 'Convert and analyse'}
            </button>
          ) : (
            <button
              disabled={!textReady || busy}
              onClick={() => onUpload(files)}
              className="rounded-lg bg-ink-800 px-4 py-2 text-sm font-medium text-white hover:bg-ink-700 disabled:opacity-40 dark:bg-ink-100 dark:text-ink-900"
            >
              Analyse my papers
            </button>
          )}
          <button disabled={busy} onClick={onSample} className="rounded-lg border border-ink-200 px-4 py-2 text-sm hover:bg-ink-50 disabled:opacity-40 dark:border-ink-700 dark:hover:bg-ink-800">
            Use sample data
          </button>
          <button disabled={busy} onClick={onClose} className="ml-auto text-sm text-ink-500 hover:underline disabled:opacity-40 dark:text-ink-300">Cancel</button>
        </div>
      </div>
    </div>
  );
}
