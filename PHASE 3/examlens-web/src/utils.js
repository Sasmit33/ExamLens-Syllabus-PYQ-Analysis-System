export const topicName = (data, t) => (t === null || t === undefined ? 'No topic' : data.topics[t].name);
export const pct = (x) => `${Math.round(x * 100)}%`;
export const cx = (...c) => c.filter(Boolean).join(' ');


export function mix(a, b, t) {
  const p = (h) => [1, 3, 5].map((i) => parseInt(h.slice(i, i + 2), 16));
  const [x, y] = [p(a), p(b)];
  const k = Math.max(0, Math.min(1, t));
  return `rgb(${x.map((v, i) => Math.round(v + (y[i] - v) * k)).join(',')})`;
}


export function layoutGraph(data, { nodeW = 168, nodeH = 46, gapX = 56, gapY = 14, pad = 16 } = {}) {
  const preds = new Map();
  for (const [u, v] of data.edges) preds.set(v, [...(preds.get(v) || []), u]);

  const layer = new Map();
  let maxLayer = 0;
  for (const t of data.study_order) {
    let l = 0;
    for (const u of preds.get(t) || []) l = Math.max(l, (layer.get(u) ?? 0) + 1);
    layer.set(t, l);
    maxLayer = Math.max(maxLayer, l);
  }
  for (const tp of data.topics) if (!layer.has(tp.id)) layer.set(tp.id, maxLayer + 1); 
  maxLayer = Math.max(...layer.values());

  const cols = Array.from({ length: maxLayer + 1 }, () => []);
  for (const tp of data.topics) cols[layer.get(tp.id)].push(tp.id);
  for (const c of cols) c.sort((a, b) => data.topics[b].priority - data.topics[a].priority);

  const rows = Math.max(...cols.map((c) => c.length));
  const width = pad * 2 + cols.length * nodeW + (cols.length - 1) * gapX;
  const height = pad * 2 + rows * (nodeH + gapY) - gapY;
  const pos = new Map();
  cols.forEach((c, ci) => {
    const colH = c.length * (nodeH + gapY) - gapY;
    const top = pad + (height - 2 * pad - colH) / 2;
    c.forEach((t, ri) => pos.set(t, { x: pad + ci * (nodeW + gapX), y: top + ri * (nodeH + gapY), w: nodeW, h: nodeH }));
  });
  return { pos, width, height };
}
