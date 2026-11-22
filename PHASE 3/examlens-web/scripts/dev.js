
import { spawn } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const vite = path.join(root, 'node_modules', 'vite', 'bin', 'vite.js');

const procs = [
  spawn(process.execPath, [path.join(root, 'server', 'server.js')], { stdio: 'inherit', cwd: root }),
  spawn(process.execPath, [vite, '--open'], { stdio: 'inherit', cwd: root }),
];

const stop = () => { for (const p of procs) p.kill(); process.exit(); };
process.on('SIGINT', stop);
process.on('SIGTERM', stop);
for (const p of procs) p.on('exit', (code) => { if (code) stop(); });
