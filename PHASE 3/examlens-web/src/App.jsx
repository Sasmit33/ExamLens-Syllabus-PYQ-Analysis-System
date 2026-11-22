import { useCallback, useEffect, useState } from 'react';
import { api } from './api.js';
import { cx } from './utils.js';
import Overview from './components/Overview.jsx';
import Questions from './components/Questions.jsx';
import Review from './components/Review.jsx';
import StudyPlan from './components/StudyPlan.jsx';
import Repeats from './components/Repeats.jsx';
import Issues from './components/Issues.jsx';
import UploadDialog from './components/UploadDialog.jsx';

const TABS = [
  { id: 'overview', label: 'Overview' },
  { id: 'questions', label: 'Questions' },
  { id: 'review', label: 'Review queue' },
  { id: 'study', label: 'Study plan' },
  { id: 'repeats', label: 'Repeated' },
  { id: 'issues', label: 'Data issues' },
];

export default function App() {
  const [data, setData] = useState(null);
  const [error, setError] = useState('');
  const [busy, setBusy] = useState(false);
  const [tab, setTab] = useState('overview');
  const [topic, setTopic] = useState(null);          
  const [reviewId, setReviewId] = useState(null);    
  const [toast, setToast] = useState('');
  const [uploadOpen, setUploadOpen] = useState(false);
  const [dark, setDark] = useState(() => document.documentElement.classList.contains('dark'));

  
  const run = useCallback(async (request, message) => {
    setBusy(true);
    try {
      const next = await request();
      setData(next);
      setError('');
      if (message) setToast(message);
      return true;
    } catch (err) {
      setError(err.message);
      return false;
    } finally {
      setBusy(false);
    }
  }, []);

  useEffect(() => { run(api.analysis); }, [run]);

  useEffect(() => {
    if (!toast) return undefined;
    const id = setTimeout(() => setToast(''), 2600);
    return () => clearTimeout(id);
  }, [toast]);

  useEffect(() => {
    document.documentElement.classList.toggle('dark', dark);
    try { localStorage.setItem('examlens-theme', dark ? 'dark' : 'light'); } catch {  }
  }, [dark]);

  const openTopic = (t) => { setTopic(t); setTab('study'); };
  const openReview = (q) => { setReviewId(q); setTab('review'); };

  const badge = (id) => {
    if (!data) return null;
    if (id === 'review') return data.review.length || null;
    if (id === 'issues') return data.issues.length || null;
    return null;
  };

  return (
    <div className="min-h-screen bg-stone-50 text-ink-900 transition-colors dark:bg-ink-950 dark:text-ink-50">
      {}
      <header className="sticky top-0 z-20 border-b border-ink-100 bg-stone-50/85 backdrop-blur dark:border-ink-800 dark:bg-ink-950/85">
        <div className="mx-auto flex max-w-7xl flex-wrap items-center gap-4 px-5 py-3">
          <div className="flex items-center gap-3">
            <img src="/favicon.svg" alt="" className="h-8 w-8" />
            <div>
              <h1 className="font-serif text-xl font-semibold leading-tight">ExamLens</h1>
              <p className="text-xs text-ink-500 dark:text-ink-300">Syllabus and past-paper analysis</p>
            </div>
          </div>

          {data && (
            <span className="rounded-full border border-ink-200 px-3 py-1 text-xs text-ink-600 dark:border-ink-700 dark:text-ink-200">
              {data.source === 'uploaded' ? 'Your uploaded files' : 'Sample data'} · {data.summary.min_year}–{data.summary.max_year}
            </span>
          )}

          <div className="ml-auto flex items-center gap-2">
            {busy && <span className="h-4 w-4 animate-spin rounded-full border-2 border-ink-300 border-t-ink-700 dark:border-ink-600 dark:border-t-ink-100" aria-label="Working" />}
            <button
              onClick={() => setUploadOpen(true)}
              className="rounded-lg bg-ink-800 px-3.5 py-2 text-sm font-medium text-white hover:bg-ink-700 dark:bg-ink-100 dark:text-ink-900 dark:hover:bg-white"
            >
              Load papers
            </button>
            <button
              onClick={() => setDark((d) => !d)}
              className="rounded-lg border border-ink-200 px-3 py-2 text-sm hover:bg-ink-50 dark:border-ink-700 dark:hover:bg-ink-900"
              aria-label="Toggle dark mode"
            >
              {dark ? 'Light' : 'Dark'}
            </button>
          </div>
        </div>

        {}
        <nav className="mx-auto flex max-w-7xl gap-1 overflow-x-auto px-5" role="tablist">
          {TABS.map((t) => (
            <button
              key={t.id}
              role="tab"
              aria-selected={tab === t.id}
              onClick={() => setTab(t.id)}
              className={cx(
                'relative flex items-center gap-2 whitespace-nowrap px-3 py-2.5 text-sm transition-colors',
                tab === t.id
                  ? 'font-semibold text-ink-900 dark:text-white'
                  : 'text-ink-500 hover:text-ink-800 dark:text-ink-300 dark:hover:text-white',
              )}
            >
              {t.label}
              {badge(t.id) && (
                <span className="rounded-full bg-pen-50 px-1.5 text-xs font-semibold text-pen-600 dark:bg-pen-600/25 dark:text-pen-100">
                  {badge(t.id)}
                </span>
              )}
              {tab === t.id && <span className="absolute inset-x-2 -bottom-px h-0.5 rounded bg-ink-800 dark:bg-ink-100" />}
            </button>
          ))}
        </nav>
      </header>

      {}
      <main className="mx-auto max-w-7xl px-5 py-6">
        {error && (
          <div role="alert" className="mb-5 whitespace-pre-wrap rounded-lg border border-pen-400 bg-pen-50 px-4 py-3 text-sm text-pen-600 dark:bg-pen-600/15 dark:text-pen-100">
            {error}
          </div>
        )}

        {!data ? (
          <div className="grid gap-4 sm:grid-cols-3">
            {[0, 1, 2].map((i) => <div key={i} className="h-28 animate-pulse rounded-xl bg-ink-100 dark:bg-ink-900" />)}
          </div>
        ) : (
          <div key={tab} className="animate-rise">
            {tab === 'overview' && (
              <Overview
                data={data}
                busy={busy}
                onWeights={(w) => run(() => api.setWeights(w))}
                onOpenTopic={openTopic}
                onOpenTab={setTab}
              />
            )}
            {tab === 'questions' && <Questions data={data} onReview={openReview} />}
            {tab === 'review' && (
              <Review
                data={data}
                selected={reviewId}
                onSelect={setReviewId}
                onDecide={(q, t) =>
                  run(() => api.review(q.year, q.qid, t === null ? null : data.topics[t].name),
                      `${q.qid} saved as ${t === null ? 'no topic' : data.topics[t].name}`)}
                onReset={() => run(api.resetReviews, 'All review decisions cleared')}
              />
            )}
            {tab === 'study' && <StudyPlan data={data} selected={topic} onSelect={setTopic} />}
            {tab === 'repeats' && <Repeats data={data} />}
            {tab === 'issues' && <Issues data={data} />}
          </div>
        )}
      </main>

      {uploadOpen && (
        <UploadDialog
          onClose={() => setUploadOpen(false)}
          onUpload={async (files) => {
            if (await run(() => api.upload(files), 'Your papers were analysed')) setUploadOpen(false);
          }}
          onSample={async () => {
            if (await run(api.useSample, 'Switched to the sample data')) setUploadOpen(false);
          }}
        />
      )}

      {toast && (
        <div role="status" className="animate-rise fixed bottom-5 right-5 z-30 rounded-lg bg-ink-900 px-4 py-2.5 text-sm text-white shadow-lg dark:bg-ink-100 dark:text-ink-900">
          {toast}
        </div>
      )}
    </div>
  );
}
