import { useEffect, useState } from 'react';
import { pct } from '../utils.js';

const WEIGHT_NAMES = [
  { name: 'Marks', hint: 'how many marks the topic carried' },
  { name: 'Years asked', hint: 'in how many years it appeared' },
  { name: 'Recency', hint: 'how recently it was asked' },
  { name: 'Repeats', hint: 'how often its questions came back' },
];
const DEFAULTS = [35, 25, 20, 20];

function Stat({ label, value, tone, onClick }) {
  return (
    <button
      onClick={onClick}
      className="rounded-xl border border-ink-100 bg-white p-4 text-left transition hover:-translate-y-0.5 hover:shadow-md dark:border-ink-800 dark:bg-ink-900"
    >
      <div className={`font-serif text-3xl font-semibold ${tone === 'pen' && value > 0 ? 'text-pen-500 dark:text-pen-400' : ''}`}>
        {value}
      </div>
      <div className="mt-1 text-sm text-ink-500 dark:text-ink-300">{label}</div>
    </button>
  );
}

export default function Overview({ data, busy, onWeights, onOpenTopic, onOpenTab }) {
  const s = data.summary;
  const max = Math.max(...data.topics.map((t) => t.priority), 0.0001);

  
  const [weights, setWeights] = useState(data.weightInputs);
  useEffect(() => setWeights(data.weightInputs), [data.weightInputs]);
  useEffect(() => {
    if (weights.every((w, i) => w === data.weightInputs[i])) return undefined;
    const id = setTimeout(() => onWeights(weights), 300);
    return () => clearTimeout(id);
  }, [weights]); 

  const total = weights.reduce((a, b) => a + b, 0) || 1;
  const bt = data.backtest;

  return (
    <div className="space-y-6">
      <section className="grid grid-cols-2 gap-3 md:grid-cols-5">
        <Stat label="Questions" value={s.questions} onClick={() => onOpenTab('questions')} />
        <Stat label="Topics" value={s.topics} onClick={() => onOpenTab('study')} />
        <Stat label="Waiting for review" value={data.review.length} tone="pen" onClick={() => onOpenTab('review')} />
        <Stat label="Repeated questions" value={data.repeat_groups.length} onClick={() => onOpenTab('repeats')} />
        <Stat label="Data issues" value={data.issues.length} tone="pen" onClick={() => onOpenTab('issues')} />
      </section>

      <div className="grid gap-6 lg:grid-cols-5">
        {}
        <section className="rounded-xl border border-ink-100 bg-white p-5 lg:col-span-3 dark:border-ink-800 dark:bg-ink-900">
          <div className="mb-4 flex items-baseline justify-between gap-3">
            <h2 className="font-serif text-lg font-semibold">Topic priority</h2>
            <span className="text-xs text-ink-500 dark:text-ink-300">Click a topic to see its study path</span>
          </div>
          <ol className="space-y-1.5">
            {data.ranking.map((t, i) => {
              const tp = data.topics[t];
              return (
                <li key={t}>
                  <button
                    onClick={() => onOpenTopic(t)}
                    title={`Marks ${tp.marks}, asked in ${tp.years} year(s), last asked ${tp.last_year ?? 'never'}`}
                    className="group grid w-full grid-cols-[1.6rem_minmax(0,12rem)_1fr_3.2rem] items-center gap-3 rounded-lg px-2 py-1.5 text-left hover:bg-ink-50 dark:hover:bg-ink-800"
                  >
                    <span className="text-xs tabular-nums text-ink-400">{i + 1}</span>
                    <span className="truncate text-sm">{tp.name}</span>
                    <span className="h-2.5 overflow-hidden rounded-full bg-ink-100 dark:bg-ink-800">
                      <span
                        className={`block h-full rounded-full transition-[width] duration-700 ease-out ${i < bt.top_k ? 'bg-pen-500' : 'bg-ink-500 group-hover:bg-ink-600 dark:bg-ink-300'}`}
                        style={{ width: `${(tp.priority / max) * 100}%` }}
                      />
                    </span>
                    <span className="text-right text-sm tabular-nums">{tp.priority.toFixed(3)}</span>
                  </button>
                </li>
              );
            })}
          </ol>
          <p className="mt-3 text-xs text-ink-500 dark:text-ink-300">
            Red bars are the top {bt.top_k}. Topics with 0 were not seen in the papers; still cover their basics.
          </p>
        </section>

        {}
        <section className="space-y-5 lg:col-span-2">
          <div className="rounded-xl border border-ink-100 bg-white p-5 dark:border-ink-800 dark:bg-ink-900">
            <div className="flex items-baseline justify-between">
              <h2 className="font-serif text-lg font-semibold">What-if: priority weights</h2>
              {busy && <span className="text-xs text-ink-400">updating...</span>}
            </div>
            <p className="mb-4 mt-1 text-sm text-ink-500 dark:text-ink-300">
              Drag a slider. The C engine recalculates the ranking, the study plan and the backtest.
            </p>
            <div className="space-y-4">
              {WEIGHT_NAMES.map((w, i) => (
                <label key={w.name} className="block">
                  <div className="flex justify-between text-sm">
                    <span title={w.hint}>{w.name}</span>
                    <span className="tabular-nums text-ink-500 dark:text-ink-300">{Math.round((weights[i] / total) * 100)}%</span>
                  </div>
                  <input
                    type="range" min="0" max="100" value={weights[i]}
                    onChange={(e) => setWeights((ws) => ws.map((x, j) => (j === i ? Number(e.target.value) : x)))}
                    className="mt-1 w-full"
                    aria-label={`${w.name} weight`}
                  />
                </label>
              ))}
            </div>
            <button
              onClick={() => setWeights(DEFAULTS)}
              className="mt-4 rounded-lg border border-ink-200 px-3 py-1.5 text-sm hover:bg-ink-50 dark:border-ink-700 dark:hover:bg-ink-800"
            >
              Reset to 35 / 25 / 20 / 20
            </button>
          </div>

          <div className="rounded-xl border border-ink-100 bg-white p-5 dark:border-ink-800 dark:bg-ink-900">
            <h2 className="font-serif text-lg font-semibold">Backtest</h2>
            {bt.score === null ? (
              <p className="mt-2 text-sm text-ink-500 dark:text-ink-300">Needs questions with marks from at least two years.</p>
            ) : (
              <>
                <p className="mt-1 text-sm text-ink-500 dark:text-ink-300">
                  Ranked using {s.min_year}–{s.max_year - 1} only: how much of {s.max_year}'s marks fall in the top {bt.top_k} topics?
                </p>
                {[['These weights', bt.score, 'bg-pen-500'], ['Past marks only', bt.baseline, 'bg-ink-300 dark:bg-ink-600']].map(([label, v, color]) => (
                  <div key={label} className="mt-3">
                    <div className="flex justify-between text-sm"><span>{label}</span><span className="font-semibold tabular-nums">{pct(v)}</span></div>
                    <div className="mt-1 h-2.5 overflow-hidden rounded-full bg-ink-100 dark:bg-ink-800">
                      <div className={`h-full rounded-full transition-[width] duration-700 ${color}`} style={{ width: pct(v) }} />
                    </div>
                  </div>
                ))}
                <p className="mt-3 text-xs text-ink-500 dark:text-ink-300">With few years of data, treat this as a rough check.</p>
              </>
            )}
          </div>
        </section>
      </div>
    </div>
  );
}
