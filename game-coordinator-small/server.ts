import { execFile } from 'node:child_process';
import { randomUUID } from 'node:crypto';
import { constants } from 'node:fs';
import { access, readdir, readFile, realpath, stat } from 'node:fs/promises';
import { createServer, type IncomingMessage } from 'node:http';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { promisify } from 'node:util';

import {
  frameRates, gameIds, games, profileStore, resolutions,
  type GameId, type Profile, type Settings,
} from './profiles.ts';

const host = '127.0.0.1';
const port = 3000;
const ownerLabel = 'io.containerz.game-coordinator';
const profileLabel = 'io.containerz.profile';
const gameLabel = 'io.containerz.game';
const portLabel = 'io.containerz.agent-port';
const owner = 'small';
const exec = promisify(execFile);

type Gpu = { pci: string; vendor: 'nvidia' | 'amd'; card: string; render: string };

type Container = {
  Id: string;
  Config: { Labels: Record<string, string> | null };
  State: { Running: boolean; ExitCode: number; StartedAt: string };
};

class RequestError extends Error {
  status: number;

  constructor(status: number, message: string) {
    super(message);
    this.status = status;
  }
}

async function command(file: string, args: string[]) {
  try {
    const { stdout } = await exec(file, args, { timeout: 30_000 });
    return stdout.trim();
  } catch (error) {
    const failure = error as NodeJS.ErrnoException & { stderr?: string; killed?: boolean };
    const details = failure.stderr?.trim() || (failure.killed ? 'command timed out' : failure.code);
    throw new RequestError(503, `${file} ${args[0]} failed${details ? `: ${details}` : ''}`);
  }
}

function containerName(profile: Profile, game: GameId) {
  return `containerz-small-${profile.id}-${game}`;
}

async function inspectGame(profile: Profile, game: GameId): Promise<Container | undefined> {
  const name = containerName(profile, game);
  let metadata: string;
  try {
    metadata = await command('docker', ['container', 'inspect', '--format', '{{json .}}', name]);
  } catch (error) {
    // A successful list distinguishes an absent container from a Docker failure.
    const names = await command('docker', [
      'container', 'ls', '--all', '--filter', `name=${name}`, '--format', '{{.Names}}',
    ]);
    if (!names.split('\n').includes(name)) return;
    throw error;
  }
  const container: Container = JSON.parse(metadata);
  const labels = container.Config.Labels;
  if (labels?.[ownerLabel] !== owner || labels[profileLabel] !== profile.id || labels[gameLabel] !== game) {
    throw new RequestError(409, `The container name ${name} is already in use by another container.`);
  }
  return container;
}

async function removeGameContainer(container: Container | undefined) {
  if (!container) return;
  if (container.State.Running) await command('docker', ['stop', container.Id]);
  await command('docker', ['rm', container.Id]);
}

async function deleteProfileSaves(profile: Profile) {
  const containers = await Promise.all(gameIds.map((game) => inspectGame(profile, game)));
  const names = await command('docker', [
    'volume', 'ls', '--filter', `name=containerz-small-${profile.id}-`, '--format', '{{.Name}}',
  ]);
  const volumes = new Set(names.split('\n'));
  for (const container of containers) await removeGameContainer(container);
  for (const game of gameIds) {
    const name = containerName(profile, game);
    if (volumes.has(name)) await command('docker', ['volume', 'rm', name]);
  }
}

async function listGpus() {
  const directory = '/dev/dri/by-path';
  const entries = await readdir(directory).catch((error: NodeJS.ErrnoException): string[] => {
    if (error.code === 'ENOENT') return [];
    throw error;
  });
  const devices = await Promise.all(entries.sort().map(async (entry): Promise<Gpu | undefined> => {
    const pci = /^pci-([\da-f]{4}:[\da-f]{2}:[\da-f]{2}\.[0-7])-card$/.exec(entry)?.[1];
    if (!pci || !entries.includes(`pci-${pci}-render`)) return;
    try {
      const [card, render, vendorId] = await Promise.all([
        realpath(`${directory}/${entry}`),
        realpath(`${directory}/pci-${pci}-render`),
        readFile(`/sys/bus/pci/devices/${pci}/vendor`, 'utf8'),
      ]);
      const vendor = { '0x10de': 'nvidia', '0x1002': 'amd' }[vendorId.trim()] as Gpu['vendor'] | undefined;
      if (vendor) return { pci, vendor, card, render };
    } catch (error) {
      if ((error as NodeJS.ErrnoException).code !== 'ENOENT') throw error;
    }
  }));
  return devices.filter((device) => device !== undefined);
}

async function gpuLaunchArguments(gpu: Gpu) {
  const [card, render] = await Promise.all([stat(gpu.card), stat(gpu.render)]);
  const groups = ['--group-add', String(card.gid), '--group-add', String(render.gid)];
  if (gpu.vendor === 'amd') return ['--device', gpu.card, '--device', gpu.render, ...groups];

  const uuid = await command('nvidia-smi', [`--id=${gpu.pci}`, '--query-gpu=uuid', '--format=csv,noheader']);
  if (!/^GPU-[\da-f-]+$/i.test(uuid)) {
    throw new RequestError(503, `Could not resolve the NVIDIA GPU UUID for ${gpu.pci}.`);
  }
  return ['--device', `nvidia.com/gpu=${uuid}`, ...groups];
}

async function holocureMount(dataDir: string) {
  const directory = process.env.HOLOCURE_GAME_DIR || join(dataDir, 'holocure');
  try {
    const source = await realpath(directory);
    await Promise.all(['HoloCure.exe', 'data.win'].map((file) => access(join(source, file), constants.R_OK)));
    // Docker parses the mount fields as CSV.
    return ['--mount', `type=bind,"src=${source.replaceAll('"', '""')}",dst=/opt/holocure,readonly`];
  } catch (error) {
    const { code } = error as NodeJS.ErrnoException;
    if (code !== 'ENOENT' && code !== 'ENOTDIR' && code !== 'EACCES') throw error;
    throw new RequestError(503, `Extract HoloCure.zip into ${directory} or set HOLOCURE_GAME_DIR to the extracted game folder.`);
  }
}

async function streamPort(profiles: Profile[], assigned?: number) {
  const listeners = await command('ss', ['-H', '-ltn']);
  const occupied = new Set(listeners.split('\n').map((line) => {
    return Number(line.trim().split(/\s+/)[3]?.match(/:(\d+)$/)?.[1]);
  }));
  if (assigned !== undefined) {
    if (occupied.has(assigned)) throw new RequestError(409, `Stream port ${assigned} is already in use.`);
    return assigned;
  }

  const reserved = new Set(profiles.flatMap(({ games }) => Object.values(games).map(({ port }) => port)));
  for (let candidate = 8100; candidate <= 8999; candidate++) {
    if (!occupied.has(candidate) && !reserved.has(candidate)) return candidate;
  }
  throw new RequestError(503, 'No stream ports are available.');
}

async function agentIsListening(port: number) {
  try {
    const response = await fetch(`http://${host}:${port}/offer`, {
      redirect: 'error',
      signal: AbortSignal.timeout(1_000),
    });
    await response.body?.cancel();
    return response.status === 405;
  } catch {
    return false;
  }
}

async function gameStatus(profile: Profile, game: GameId) {
  const saved = profile.games[game];
  const settings = saved
    ? { gpu: saved.gpu, resolution: saved.resolution, fps: saved.fps }
    : { gpu: null, resolution: '720p', fps: 60 };
  const details = { id: game, title: games[game].title, settings };
  const container = await inspectGame(profile, game).catch((error: unknown) => {
    if (error instanceof RequestError && error.status === 409) return error;
    throw error;
  });
  if (!container) return { ...details, state: 'idle' };
  if (container instanceof RequestError) return { ...details, state: 'blocked', message: container.message };
  if (!container.State.Running) {
    return { ...details, state: 'failed', message: `Container exited with status ${container.State.ExitCode}.` };
  }
  if (!saved || container.Config.Labels?.[portLabel] !== String(saved.port)) {
    return { ...details, state: 'failed', message: 'The container has no matching stream port.' };
  }
  if (await agentIsListening(saved.port)) {
    const address = `${host}:${saved.port}`;
    const viewer = new URL(`http://${host}:5173`);
    viewer.searchParams.set('localAgent', address);
    return { ...details, state: 'ready', address, viewerUrl: viewer.href };
  }
  if (Date.now() - Date.parse(container.State.StartedAt) >= 20_000) {
    return { ...details, state: 'failed', message: 'The streaming agent did not respond within 20 seconds.' };
  }
  return { ...details, state: 'starting' };
}

async function jsonBody(request: IncomingMessage): Promise<Record<string, unknown>> {
  if (request.headers['content-type']?.split(';')[0].trim() !== 'application/json') {
    throw new RequestError(415, 'Send an application/json body.');
  }
  let body = '';
  request.setEncoding('utf8');
  for await (const chunk of request.iterator({ destroyOnReturn: false })) {
    body += chunk;
    if (body.length > 4096) throw new RequestError(413, 'Request body is too large.');
  }
  let data: unknown;
  try {
    data = JSON.parse(body);
  } catch {
    throw new RequestError(400, 'Send valid JSON.');
  }
  if (!data || typeof data !== 'object' || Array.isArray(data)) {
    throw new RequestError(400, 'Send a JSON object.');
  }
  return data as Record<string, unknown>;
}

function validateSettings(data: Record<string, unknown>): Settings {
  const { gpu, resolution, fps } = data;
  if (typeof gpu !== 'string' || typeof resolution !== 'string' || !Object.hasOwn(resolutions, resolution) ||
      (fps !== 30 && fps !== 60 && fps !== 120)) {
    throw new RequestError(400, 'Select a GPU, a supported resolution, and 30, 60, or 120 fps.');
  }
  return { gpu, resolution: resolution as Settings['resolution'], fps };
}

function findProfile(profiles: Profile[], id: string) {
  const profile = profiles.find((entry) => entry.id === id);
  if (!profile) throw new RequestError(404, 'Profile not found.');
  return profile;
}

function validateProfileName(value: unknown) {
  const name = typeof value === 'string' ? value.trim() : '';
  if (!name || name.length > 48 || /[\u0000-\u001f\u007f]/.test(name)) {
    throw new RequestError(400, 'Enter a profile name of 1 to 48 characters.');
  }
  return name;
}

function requireUniqueProfileName(profiles: Profile[], name: string, id?: string) {
  if (profiles.some((profile) => profile.id !== id && profile.name.toLowerCase() === name.toLowerCase())) {
    throw new RequestError(409, 'A profile with that name already exists.');
  }
}

export function createCoordinatorServer(dataDir = fileURLToPath(new URL('./data/', import.meta.url))) {
  const store = profileStore(dataDir);
  let lastWrite = Promise.resolve();
  function mutate<T>(operation: () => Promise<T>) {
    const task = lastWrite.then(operation);
    lastWrite = task.then(() => {}, () => {});
    return task;
  }

  const server = createServer(async (request, response) => {
    response.setHeader('Cache-Control', 'no-store');
    function json(status: number, data: object) {
      response.writeHead(status, { 'Content-Type': 'application/json; charset=utf-8' });
      response.end(JSON.stringify(data));
    }

    try {
      const path = request.url ?? '';
      if (request.method === 'POST' || request.method === 'PATCH' || request.method === 'DELETE') {
        const bound = server.address();
        const origin = bound && typeof bound === 'object' ? `http://${host}:${bound.port}` : undefined;
        if (!origin || request.headers.origin !== origin) {
          throw new RequestError(403, 'Use the game coordinator page to manage profiles and games.');
        }
      }

      if (request.method === 'GET' && path === '/') {
        const page = await readFile(new URL('./index.html', import.meta.url));
        response.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
        response.end(page);
        return;
      }
      if (path === '/api/profiles') {
        if (request.method === 'GET') {
          const profiles = await store.read();
          return json(200, { profiles: profiles.map(({ id, name }) => ({ id, name })) });
        }
        if (request.method === 'POST') {
          const name = validateProfileName((await jsonBody(request)).name);
          const id = await mutate(async () => {
            const profiles = await store.read();
            requireUniqueProfileName(profiles, name);
            const id = randomUUID();
            await store.write([...profiles, { id, name, games: {} }]);
            return id;
          });
          return json(201, { id, name });
        }
      }
      const profilePath = /^\/api\/profiles\/([a-f\d-]+)$/.exec(path);
      if (profilePath && request.method === 'PATCH') {
        const [, id] = profilePath;
        const name = validateProfileName((await jsonBody(request)).name);
        await mutate(async () => {
          const profiles = await store.read();
          const profile = findProfile(profiles, id);
          requireUniqueProfileName(profiles, name, id);
          profile.name = name;
          await store.write(profiles);
        });
        return json(200, { id, name });
      }
      if (profilePath && request.method === 'DELETE') {
        const [, id] = profilePath;
        await mutate(async () => {
          const profiles = await store.read();
          const profile = findProfile(profiles, id);
          await deleteProfileSaves(profile);
          await store.write(profiles.filter((entry) => entry.id !== id));
        });
        return response.writeHead(204).end();
      }
      if (request.method === 'GET' && path === '/api/gpus') {
        const gpus = await listGpus();
        return json(200, { gpus: gpus.map(({ pci, vendor }) => ({ pci, vendor })) });
      }
      const list = /^\/api\/profiles\/([a-f\d-]+)\/games$/.exec(path);
      if (request.method === 'GET' && list) {
        const profile = findProfile(await store.read(), list[1]);
        const statuses = await Promise.all(gameIds.map((game) => gameStatus(profile, game)));
        return json(200, { games: statuses, resolutions, frameRates });
      }

      const action = /^\/api\/profiles\/([a-f\d-]+)\/games\/([a-z\d-]+)\/(start|stop)$/.exec(path);
      if (request.method !== 'POST' || !action || !Object.hasOwn(games, action[2])) {
        return json(404, { message: 'Route not found.' });
      }

      const [, id, gameId, operation] = action;
      const game = gameId as GameId;
      const settings = operation === 'start' ? validateSettings(await jsonBody(request)) : undefined;
      await mutate(async () => {
        const profiles = await store.read();
        const profile = findProfile(profiles, id);
        const container = await inspectGame(profile, game);
        if (!settings) {
          await removeGameContainer(container);
          return;
        }
        if (container) throw new RequestError(409, 'Stop this profile’s existing game before starting it again.');
        const gameMount = game === 'holocure' ? await holocureMount(dataDir) : [];
        const gpu = (await listGpus()).find(({ pci }) => pci === settings.gpu);
        if (!gpu) throw new RequestError(400, 'The selected GPU is unavailable. Choose another GPU.');
        const gpuArgs = await gpuLaunchArguments(gpu);
        const port = await streamPort(profiles, profile.games[game]?.port);
        profile.games[game] = { ...settings, port };
        // Reserve the address on disk before starting Docker so a restart can recover it.
        await store.write(profiles);

        const [width, height] = resolutions[settings.resolution];
        const name = containerName(profile, game);
        const { image, dataPath } = games[game];
        await command('docker', [
          'run', '-d', '--name', name, '--label', `${ownerLabel}=${owner}`,
          '--label', `${profileLabel}=${id}`, '--label', `${gameLabel}=${game}`,
          '--label', `${portLabel}=${port}`, '--pull', 'never', '--network', 'host',
          '--stop-timeout', '15', '--ulimit', 'core=0', ...gpuArgs,
          '--env', `DISPLAY_WIDTH=${width}`, '--env', `DISPLAY_HEIGHT=${height}`,
          '--env', `DISPLAY_FPS=${settings.fps}`, '--env', `CONTAINERZ_AGENT_PORT=${port}`,
          '--mount', `type=volume,src=${name},dst=${dataPath}`, ...gameMount, image,
        ]);
      });

      if (settings) return json(202, { state: 'starting' });
      response.writeHead(204).end();
    } catch (error) {
      if (error instanceof RequestError) return json(error.status, { message: error.message });
      console.error(error);
      json(500, { message: 'The game coordinator encountered an error. Check its terminal output.' });
    }
  });
  return server;
}

if (import.meta.main) {
  const server = createCoordinatorServer();
  const origin = `http://${host}:${port}`;
  server.on('error', (error) => {
    console.error(`Could not listen at ${origin}. ${error.message}`);
    process.exitCode = 1;
  });
  server.listen(port, host, () => console.log(`Game coordinator: ${origin}`));
}
