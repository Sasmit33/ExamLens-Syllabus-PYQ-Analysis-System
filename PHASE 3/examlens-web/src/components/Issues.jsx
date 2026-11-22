export default function Issues({ data }) {
  if (!data.issues.length)
    return <p className="text-ink-500 dark:text-ink-300">No problems found in the input files.</p>;

  const kind = (text) =>
    /duplicate/.test(text) ? 'Duplicate' : /missing marks|invalid marks/.test(text) ? 'Marks'
      : /missing year|invalid year/.test(text) ? 'Year' : /cycle/.test(text) ? 'Cycle'
      : /expected|empty/.test(text) ? 'Format' : 'Other';

  return (
    <div className="space-y-3">
      <p className="text-sm text-ink-500 dark:text-ink-300">Fix these lines in the .txt files and load them again.</p>
      <ul className="divide-y divide-ink-100 rounded-xl border border-ink-100 bg-white dark:divide-ink-800 dark:border-ink-800 dark:bg-ink-900">
        {data.issues.map((text, i) => (
          <li key={i} className="flex items-start gap-3 px-4 py-3 text-sm">
            <span className="mt-0.5 w-20 shrink-0 rounded bg-pen-50 px-2 py-0.5 text-center text-xs font-semibold text-pen-600 dark:bg-pen-600/20 dark:text-pen-100">
              {kind(text)}
            </span>
            <span>{text}</span>
          </li>
        ))}
      </ul>
    </div>
  );
}
