import { useEffect, useState } from 'react';

const FIELDS = [
  { key: 'questions', label: 'Questions file', note: 'YEAR: lines, then QID | marks | text', required: true },
  { key: 'syllabus', label: 'Syllabus file', note: 'UNIT:, TOPIC: and ALIAS: lines', required: true },
  { key: 'prereqs', label: 'Prerequisites file', note: 'Topic A -> Topic B (optional)', required: false },
];

export default function UploadDialog({ onClose, onUpload, onSample }) {
  const [files, setFiles] = useState({});
  const [names, setNames] = useState({});
  const [dragging, setDragging] = useState(false);

  useEffect(() => {
    const onKey = (e) => e.key === 'Escape' && onClose();
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [onClose]);

  const read = async (key, file) => {
    if (!file) return;
    const text = await file.text();
    setFiles((f) => ({ ...f, [key]: text }));
    setNames((n) => ({ ...n, [key]: file.name }));
  };

  
  const onDrop = async (e) => {
    e.preventDefault();
    setDragging(false);
    for (const file of e.dataTransfer.files) {
      const n = file.name.toLowerCase();
      await read(n.includes('syllabus') ? 'syllabus' : n.includes('prereq') ? 'prereqs' : 'questions', file);
    }
  };

  const ready = files.questions && files.syllabus;

  return (
    <div className="fixed inset-0 z-40 flex items-center justify-center bg-ink-950/50 p-4" onClick={onClose}>
      <div
        role="dialog" aria-modal="true" aria-label="Load papers"
        onClick={(e) => e.stopPropagation()}
        className="animate-rise w-full max-w-lg rounded-2xl bg-white p-6 shadow-xl dark:bg-ink-900"
      >
        <h2 className="font-serif text-xl font-semibold">Load your papers</h2>
        <p className="mt-1 text-sm text-ink-500 dark:text-ink-300">Choose the text files, or drop them all into the box below.</p>

        <div
          onDragOver={(e) => { e.preventDefault(); setDragging(true); }}
          onDragLeave={() => setDragging(false)}
          onDrop={onDrop}
          className={`mt-4 space-y-3 rounded-xl border-2 border-dashed p-4 transition ${dragging ? 'border-ink-500 bg-ink-50 dark:bg-ink-800' : 'border-ink-200 dark:border-ink-700'}`}
        >
          {FIELDS.map((f) => (
            <label key={f.key} className="flex items-center justify-between gap-3">
              <span>
                <span className="block text-sm font-medium">{f.label}{f.required && <span className="text-pen-500"> *</span>}</span>
                <span className="block text-xs text-ink-500 dark:text-ink-300">{names[f.key] || f.note}</span>
              </span>
              <span className="cursor-pointer rounded-lg border border-ink-200 px-3 py-1.5 text-sm hover:bg-ink-50 dark:border-ink-700 dark:hover:bg-ink-800">
                {names[f.key] ? 'Change' : 'Choose'}
                <input type="file" accept=".txt,text/plain" className="sr-only" onChange={(e) => read(f.key, e.target.files[0])} />
              </span>
            </label>
          ))}
        </div>

        <div className="mt-5 flex flex-wrap items-center gap-2">
          <button
            disabled={!ready}
            onClick={() => onUpload(files)}
            className="rounded-lg bg-ink-800 px-4 py-2 text-sm font-medium text-white hover:bg-ink-700 disabled:opacity-40 dark:bg-ink-100 dark:text-ink-900"
          >
            Analyse my papers
          </button>
          <button onClick={onSample} className="rounded-lg border border-ink-200 px-4 py-2 text-sm hover:bg-ink-50 dark:border-ink-700 dark:hover:bg-ink-800">
            Use sample data
          </button>
          <button onClick={onClose} className="ml-auto text-sm text-ink-500 hover:underline dark:text-ink-300">Cancel</button>
        </div>
      </div>
    </div>
  );
}
