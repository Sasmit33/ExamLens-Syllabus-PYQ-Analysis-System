
async function call(url, body) {
  let res;
  try {
    res = await fetch(url, body === undefined ? undefined : {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
    });
  } catch {
    throw new Error('Cannot reach the local server. Is "npm run dev" still running?');
  }
  let data;
  try {
    data = await res.json();
  } catch {
    throw new Error('The server returned an invalid response.');
  }
  if (data.error) {
    const extra = [...(data.issues || []), data.details].filter(Boolean).join('\n');
    throw new Error(data.error + (extra ? '\n' + extra : ''));
  }
  return data;
}

export const api = {
  analysis: () => call('/api/analysis'),
  setWeights: (weights) => call('/api/weights', { weights }),
  review: (year, qid, topic) => call('/api/review', { year, qid, topic }),
  resetReviews: () => call('/api/reviews/reset', {}),
  upload: (files) => call('/api/upload', files),
  useSample: () => call('/api/use-sample', {}),
};
