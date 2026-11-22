import { useEffect, useState } from 'react';
import { cx, topicName } from '../utils.js';

function Key({ children }) {
  return <kbd className="rounded border border-ink-200 bg-ink-50 px-1.5 py-0.5 font-sans text-xs dark:border-ink-700 dark:bg-ink-800">{children}</kbd>;
}

export default function Review({ data, selected, onSelect, onDecide, onReset }) {
  const queue = data.review;                                   
  const current = queue.includes(selected) ? selected : queue[0] ?? null;
  const q = current === null ? null : data.questions[current];
  const [choice, setChoice] = useState(q?.topic ?? null);

  useEffect(() => { setChoice(q?.topic ?? null); }, [current]); 

  
  useEffect(() => {
    const onKey = (e) => {
      if (!q || ['INPUT', 'TEXTAREA', 'SELECT'].includes(e.target.tagName)) return;
      const i = queue.indexOf(current);
      if (e.key === 'ArrowDown' && i < queue.length - 1) { onSelect(queue[i + 1]); e.preventDefault(); }
      else if (e.key === 'ArrowUp' && i > 0) { onSelect(queue[i - 1]); e.preventDefault(); }
      else if (e.key === 'Enter' && choice !== null) { onDecide(q, choice); e.preventDefault(); }
      else if (e.key.toLowerCase() === 'g') onDecide(q, q.topic);
      else if (e.key.toLowerCase() === 'n') onDecide(q, null);
    };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [q, queue, current, choice, onDecide, onSelect]);

  return (
    <div className="grid gap-6 lg:grid-cols-[minmax(0,22rem)_1fr]">
      {}
      <aside>
        <div className="mb-3 flex items-center justify-between">
          <h2 className="font-serif text-lg font-semibold">
            {queue.length ? `${queue.length} to review` : 'All reviewed'}
          </h2>
          <button onClick={onReset} className="text-xs text-ink-500 underline-offset-2 hover:underline dark:text-ink-300">
            Clear saved decisions
          </button>
        </div>
        <ol className="space-y-1.5">
          {queue.map((id, i) => {
            const x = data.questions[id];
            return (
              <li key={id}>
                <button
                  onClick={() => onSelect(id)}
                  className={cx(
                    'w-full rounded-lg border px-3 py-2 text-left transition',
                    id === current
                      ? 'border-ink-700 bg-white shadow-sm dark:border-ink-200 dark:bg-ink-900'
                      : 'border-transparent hover:border-ink-200 hover:bg-white dark:hover:border-ink-700 dark:hover:bg-ink-900',
                  )}
                >
                  <div className="text-xs text-ink-500 dark:text-ink-300">{i + 1}. {x.year} {x.qid} · guess: {topicName(data, x.topic)}</div>
                  <div className="truncate text-sm">{x.text}</div>
                </button>
              </li>
            );
          })}
        </ol>
      </aside>

      {}
      <section className="rounded-xl border border-ink-100 bg-white p-6 dark:border-ink-800 dark:bg-ink-900">
        {!q ? (
          <div className="py-10 text-center">
            <p className="font-serif text-xl">Every flagged question has been reviewed.</p>
            <p className="mt-2 text-sm text-ink-500 dark:text-ink-300">Priorities and the study plan now use your decisions.</p>
          </div>
        ) : (
          <div key={current} className="animate-rise">
            <div className="flex flex-wrap items-center gap-2 text-sm text-ink-500 dark:text-ink-300">
              <span>{q.year} · {q.qid}</span>
              <span className={cx('rounded border px-1.5 text-xs', q.marks === null ? 'border-pen-400 text-pen-500' : 'border-ink-200 dark:border-ink-700')}>
                {q.marks === null ? 'marks missing' : `${q.marks} marks`}
              </span>
            </div>
            <p className="mt-3 font-serif text-xl leading-relaxed">{q.text}</p>

            <div className="mt-5 rounded-lg border-l-4 border-pen-500 bg-pen-50 px-4 py-3 text-sm text-pen-600 dark:bg-pen-600/15 dark:text-pen-100">
              Best guess: <b>{topicName(data, q.topic)}</b> · confidence {q.confidence.toFixed(2)} · score {q.score.toFixed(1)}
            </div>

            <h3 className="mb-2 mt-6 text-sm font-semibold">Choose the correct topic</h3>
            <div className="flex flex-wrap gap-2">
              {data.topics.map((t) => (
                <button
                  key={t.id}
                  onClick={() => setChoice(t.id)}
                  className={cx(
                    'rounded-full border px-3 py-1 text-sm transition',
                    choice === t.id
                      ? 'border-ink-800 bg-ink-800 text-white dark:border-ink-100 dark:bg-ink-100 dark:text-ink-900'
                      : 'border-ink-200 hover:border-ink-500 dark:border-ink-700 dark:hover:border-ink-300',
                    t.id === q.topic && choice !== t.id && 'border-pen-400',
                  )}
                >
                  {t.name}
                </button>
              ))}
            </div>

            <div className="mt-6 flex flex-wrap items-center gap-2">
              <button
                disabled={choice === null}
                onClick={() => onDecide(q, choice)}
                className="rounded-lg bg-ink-800 px-4 py-2 text-sm font-medium text-white hover:bg-ink-700 disabled:opacity-40 dark:bg-ink-100 dark:text-ink-900"
              >
                Assign topic <span className="opacity-60">↵</span>
              </button>
              <button onClick={() => onDecide(q, q.topic)} className="rounded-lg border border-ink-200 px-4 py-2 text-sm hover:bg-ink-50 dark:border-ink-700 dark:hover:bg-ink-800">
                Accept guess <span className="opacity-60">G</span>
              </button>
              <button onClick={() => onDecide(q, null)} className="rounded-lg border border-ink-200 px-4 py-2 text-sm hover:bg-ink-50 dark:border-ink-700 dark:hover:bg-ink-800">
                No topic <span className="opacity-60">N</span>
              </button>
            </div>
            <p className="mt-4 text-xs text-ink-500 dark:text-ink-300">
              Shortcuts: <Key>↑</Key> <Key>↓</Key> move · <Key>Enter</Key> assign · <Key>G</Key> accept guess · <Key>N</Key> no topic
            </p>
          </div>
        )}
      </section>
    </div>
  );
}
