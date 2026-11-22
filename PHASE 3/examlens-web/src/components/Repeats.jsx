export default function Repeats({ data }) {
  if (!data.repeat_groups.length)
    return <p className="text-ink-500 dark:text-ink-300">No repeated questions found.</p>;

  return (
    <div className="space-y-4">
      <p className="text-sm text-ink-500 dark:text-ink-300">
        Questions asked again in different years. Wording like "explain" or "define" is ignored, so reworded questions still match
        (word-level edit distance, grouped with union-find).
      </p>
      <div className="grid gap-4 md:grid-cols-2">
        {data.repeat_groups.map((g, i) => (
          <article key={i} className="rounded-xl border border-ink-100 bg-white p-5 dark:border-ink-800 dark:bg-ink-900">
            <div className="mb-3 flex items-center justify-between">
              <h3 className="font-semibold">Repeated in {g.years} years</h3>
              <span className="flex gap-1">
                {Array.from({ length: g.years }, (_, k) => <span key={k} className="h-2 w-5 rounded-full bg-pen-500" />)}
              </span>
            </div>
            <ul className="space-y-2">
              {g.questions.map((id) => {
                const q = data.questions[id];
                return (
                  <li key={id} className="flex gap-3">
                    <span className="shrink-0 rounded bg-ink-50 px-2 py-0.5 text-xs tabular-nums dark:bg-ink-800">{q.year} {q.qid}</span>
                    <span className="font-serif text-sm">{q.text}</span>
                  </li>
                );
              })}
            </ul>
          </article>
        ))}
      </div>
    </div>
  );
}
