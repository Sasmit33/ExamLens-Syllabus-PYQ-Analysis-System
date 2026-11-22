import { useMemo, useState } from 'react';
import { cx, topicName } from '../utils.js';

const COLUMNS = [
  { key: 'year', label: 'Year', get: (q) => q.year ?? -1 },
  { key: 'qid', label: 'Id', get: (q) => q.qid },
  { key: 'marks', label: 'Marks', get: (q) => q.marks ?? -1 },
  { key: 'topic', label: 'Topic', get: (q, d) => topicName(d, q.topic) },
  { key: 'confidence', label: 'Confidence', get: (q) => q.confidence },
  { key: 'text', label: 'Question', get: (q) => q.text },
];

export default function Questions({ data, onReview }) {
  const [query, setQuery] = useState('');
  const [filter, setFilter] = useState('all');
  const [sort, setSort] = useState({ key: 'year', dir: 1 });

  const rows = useMemo(() => {
    const q = query.trim().toLowerCase();
    let list = data.questions.filter((x) => !x.duplicate);
    if (filter === 'review') list = list.filter((x) => x.needs_review);
    if (filter === 'none') list = list.filter((x) => x.topic === null);
    if (filter === 'nomarks') list = list.filter((x) => x.marks === null);
    if (q) list = list.filter((x) => `${x.text} ${x.qid} ${x.year} ${topicName(data, x.topic)}`.toLowerCase().includes(q));
    const col = COLUMNS.find((c) => c.key === sort.key);
    return [...list].sort((a, b) => {
      const va = col.get(a, data), vb = col.get(b, data);
      return (va > vb ? 1 : va < vb ? -1 : a.id - b.id) * sort.dir;
    });
  }, [data, query, filter, sort]);

  return (
    <div className="space-y-4">
      <div className="flex flex-wrap gap-3">
        <input
          value={query}
          onChange={(e) => setQuery(e.target.value)}
          placeholder="Search question text, topic, id or year"
          className="min-w-64 flex-1 rounded-lg border border-ink-200 bg-white px-3 py-2 text-sm outline-none focus:border-ink-500 dark:border-ink-700 dark:bg-ink-900"
        />
        <select
          value={filter}
          onChange={(e) => setFilter(e.target.value)}
          className="rounded-lg border border-ink-200 bg-white px-3 py-2 text-sm dark:border-ink-700 dark:bg-ink-900"
        >
          <option value="all">All questions</option>
          <option value="review">Needs review</option>
          <option value="none">No topic</option>
          <option value="nomarks">Missing marks</option>
        </select>
      </div>

      <div className="overflow-x-auto rounded-xl border border-ink-100 bg-white dark:border-ink-800 dark:bg-ink-900">
        <table className="w-full text-sm">
          <thead className="border-b border-ink-100 text-left text-xs text-ink-500 dark:border-ink-800 dark:text-ink-300">
            <tr>
              {COLUMNS.map((c) => (
                <th key={c.key} className="px-3 py-2.5 font-semibold">
                  <button
                    onClick={() => setSort((s) => ({ key: c.key, dir: s.key === c.key ? -s.dir : 1 }))}
                    className="hover:text-ink-900 dark:hover:text-white"
                  >
                    {c.label} {sort.key === c.key ? (sort.dir > 0 ? '▲' : '▼') : ''}
                  </button>
                </th>
              ))}
            </tr>
          </thead>
          <tbody>
            {rows.map((q) => (
              <tr
                key={q.id}
                onClick={q.needs_review ? () => onReview(q.id) : undefined}
                className={cx(
                  'border-b border-ink-50 align-top last:border-0 dark:border-ink-800/60',
                  q.needs_review && 'cursor-pointer bg-pen-50/60 hover:bg-pen-50 dark:bg-pen-600/10 dark:hover:bg-pen-600/20',
                )}
                title={q.needs_review ? 'Click to review this question' : undefined}
              >
                <td className="px-3 py-2 tabular-nums">{q.year ?? '-'}</td>
                <td className="px-3 py-2 font-medium">{q.qid}</td>
                <td className="px-3 py-2">
                  <span className={cx('rounded border px-1.5 text-xs', q.marks === null ? 'border-pen-400 text-pen-500' : 'border-ink-200 dark:border-ink-700')}>
                    {q.marks ?? '?'}
                  </span>
                </td>
                <td className="px-3 py-2">
                  {topicName(data, q.topic)}
                  {q.or_pair && <span className="ml-2 rounded bg-ink-50 px-1.5 text-xs text-ink-500 dark:bg-ink-800 dark:text-ink-300">OR</span>}
                </td>
                <td className="px-3 py-2">
                  <div className="flex items-center gap-2">
                    <span className="h-1.5 w-14 overflow-hidden rounded-full bg-ink-100 dark:bg-ink-800">
                      <span className={cx('block h-full', q.needs_review ? 'bg-pen-500' : 'bg-emerald-600')} style={{ width: `${q.confidence * 100}%` }} />
                    </span>
                    <span className="tabular-nums">{q.confidence.toFixed(2)}</span>
                  </div>
                </td>
                <td className="px-3 py-2 font-serif">{q.text}</td>
              </tr>
            ))}
            {!rows.length && (
              <tr><td colSpan={COLUMNS.length} className="px-3 py-8 text-center text-ink-500">No questions match.</td></tr>
            )}
          </tbody>
        </table>
      </div>
      <p className="text-xs text-ink-500 dark:text-ink-300">Highlighted rows have low confidence: click one to review it.</p>
    </div>
  );
}
