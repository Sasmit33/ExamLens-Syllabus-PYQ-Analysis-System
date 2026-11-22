import { useMemo, useState } from 'react';
import { cx, layoutGraph, mix } from '../utils.js';

const SELECTED = '#c0392b';
const BEFORE = '#d9822b';
const AFTER = '#2e8b7a';

function Graph({ data, selected, onSelect }) {
  const [hover, setHover] = useState(null);
  const { pos, width, height } = useMemo(() => layoutGraph(data), [data]);

  const sel = selected === null ? null : data.topics[selected];
  const before = new Set(sel ? sel.before.map(([t]) => t) : []);
  const after = new Set(sel ? sel.after.map(([t]) => t) : []);
  const up = new Set([...before, selected]);
  const down = new Set([...after, selected]);

  return (
    <div className="overflow-auto rounded-xl border border-ink-100 bg-white dark:border-ink-800 dark:bg-ink-900">
      <svg viewBox={`0 0 ${width} ${height}`} className="mx-auto block h-auto" role="img"
           style={{ width: '100%', maxWidth: width, minWidth: Math.min(width, 640) }}
           aria-label="Prerequisite graph of syllabus topics">
        <defs>
          {[['arrow', '#9aa7bd'], ['arrow-before', BEFORE], ['arrow-after', AFTER]].map(([id, c]) => (
            <marker key={id} id={id} viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto">
              <path d="M0 1 L9 5 L0 9 z" fill={c} />
            </marker>
          ))}
        </defs>

        {data.edges.map(([u, v]) => {
          const a = pos.get(u), b = pos.get(v);
          if (!a || !b) return null;
          const x1 = a.x + a.w, y1 = a.y + a.h / 2, x2 = b.x - 2, y2 = b.y + b.h / 2, dx = (x2 - x1) / 2;
          const kind = sel === null ? 'none' : up.has(u) && up.has(v) ? 'before' : down.has(u) && down.has(v) ? 'after' : 'dim';
          const stroke = kind === 'before' ? BEFORE : kind === 'after' ? AFTER : '#9aa7bd';
          return (
            <path key={`${u}-${v}`} d={`M${x1},${y1} C${x1 + dx},${y1} ${x2 - dx},${y2} ${x2},${y2}`}
                  fill="none" stroke={stroke} strokeWidth={kind === 'before' || kind === 'after' ? 2.4 : 1.3}
                  opacity={kind === 'dim' ? 0.2 : 1}
                  markerEnd={`url(#${kind === 'before' ? 'arrow-before' : kind === 'after' ? 'arrow-after' : 'arrow'})`}
                  className="transition-all duration-300" />
          );
        })}

        {data.topics.map((t) => {
          const p = pos.get(t.id);
          const related = sel === null || t.id === selected || before.has(t.id) || after.has(t.id);
          const stroke = t.id === selected ? SELECTED : before.has(t.id) ? BEFORE : after.has(t.id) ? AFTER : hover === t.id ? '#5e81bb' : '#b7c8e4';
          const strong = t.id === selected || before.has(t.id) || after.has(t.id);
          const light = t.priority <= 0.5;
          return (
            <g key={t.id} transform={`translate(${p.x},${p.y})`} onClick={() => onSelect(t.id)}
               onMouseEnter={() => setHover(t.id)} onMouseLeave={() => setHover(null)}
               className="cursor-pointer transition-opacity duration-300" opacity={related ? 1 : 0.25}>
              <title>{`${t.name}\nPriority ${t.priority.toFixed(3)}`}</title>
              <rect width={p.w} height={p.h} rx="9" fill={mix('#eef3fa', '#18294b', t.priority)}
                    stroke={stroke} strokeWidth={strong ? 3 : hover === t.id ? 2 : 1} />
              <text x={p.w / 2} y={19} textAnchor="middle" fontSize="12.5" fontWeight="600"
                    fill={light ? '#18294b' : '#ffffff'}>
                {t.name.length > 22 ? t.name.slice(0, 21) + '…' : t.name}
              </text>
              <text x={p.w / 2} y={35} textAnchor="middle" fontSize="11" fill={light ? '#3e5c99' : '#dbe5f3'}>
                {t.priority.toFixed(2)}
              </text>
            </g>
          );
        })}
      </svg>
    </div>
  );
}

function Levels({ title, pairs, word, empty, color }) {
  const groups = {};
  for (const [t, lvl] of pairs) (groups[lvl] ||= []).push(t);
  return (
    <div>
      <h3 className="mb-2 flex items-center gap-2 text-sm font-semibold">
        <span className="h-2.5 w-2.5 rounded-full" style={{ background: color }} />{title}
      </h3>
      {!pairs.length ? <p className="text-sm text-ink-500 dark:text-ink-300">{empty}</p> : (
        <div className="space-y-2">
          {Object.entries(groups).map(([lvl, ts]) => (
            <div key={lvl}>
              <div className="text-xs text-ink-500 dark:text-ink-300">{lvl} {lvl === '1' ? 'step' : 'steps'} {word}</div>
              <div className="mt-1 flex flex-wrap gap-1.5">{ts.map((t) => <span key={t} className="rounded-full bg-ink-50 px-2.5 py-0.5 text-sm dark:bg-ink-800">{t}</span>)}</div>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}

export default function StudyPlan({ data, selected, onSelect }) {
  const current = selected ?? data.study_order[0] ?? null;
  const tp = current === null ? null : data.topics[current];
  const name = (t) => data.topics[t].name;

  return (
    <div className="space-y-5">
      {!data.study_complete && (
        <div className="rounded-lg border border-pen-400 bg-pen-50 px-4 py-3 text-sm text-pen-600 dark:bg-pen-600/15 dark:text-pen-100">
          Only {data.study_order.length} of {data.topics.length} topics could be ordered: the prerequisites contain a cycle. See Data issues.
        </div>
      )}

      <div className="flex flex-wrap items-center gap-4 text-xs text-ink-500 dark:text-ink-300">
        <span>Click a topic.</span>
        {[['Selected', SELECTED], ['Study first', BEFORE], ['Leads to', AFTER]].map(([l, c]) => (
          <span key={l} className="flex items-center gap-1.5"><span className="h-2.5 w-2.5 rounded-full" style={{ background: c }} />{l}</span>
        ))}
        <span className="flex items-center gap-1.5">
          <span className="h-2.5 w-10 rounded-full" style={{ background: 'linear-gradient(90deg,#eef3fa,#18294b)' }} />darker = higher priority
        </span>
      </div>

      <Graph data={data} selected={current} onSelect={onSelect} />

      <div className="grid gap-6 lg:grid-cols-[minmax(0,24rem)_1fr]">
        <section>
          <h2 className="mb-3 font-serif text-lg font-semibold">Study order</h2>
          <ol className="space-y-1">
            {data.study_order.map((t, i) => (
              <li key={t}>
                <button
                  onClick={() => onSelect(t)}
                  className={cx(
                    'flex w-full items-center gap-3 rounded-lg px-3 py-1.5 text-left text-sm transition',
                    t === current ? 'bg-ink-800 text-white dark:bg-ink-100 dark:text-ink-900' : 'hover:bg-ink-50 dark:hover:bg-ink-800',
                  )}
                >
                  <span className="w-5 text-right tabular-nums opacity-60">{i + 1}</span>
                  <span className="flex-1">{name(t)}</span>
                  <span className="tabular-nums opacity-70">{data.topics[t].priority.toFixed(2)}</span>
                </button>
              </li>
            ))}
          </ol>
        </section>

        {tp && (
          <section key={current} className="animate-rise space-y-5 rounded-xl border border-ink-100 bg-white p-5 dark:border-ink-800 dark:bg-ink-900">
            <div>
              <h2 className="font-serif text-xl font-semibold">{tp.name}</h2>
              <p className="mt-1 text-sm text-ink-500 dark:text-ink-300">
                Priority {tp.priority.toFixed(3)} · {tp.marks} marks · asked in {tp.years} year(s) · last asked {tp.last_year ?? 'never'}
              </p>
            </div>
            <Levels title="Study these first" color={BEFORE} word="back"
                    pairs={tp.before.map(([t, l]) => [name(t), l])} empty="No prerequisites: you can start here." />
            <Levels title="This topic leads to" color={AFTER} word="ahead"
                    pairs={tp.after.map(([t, l]) => [name(t), l])} empty="No later topics depend on this one." />
          </section>
        )}
      </div>
    </div>
  );
}
