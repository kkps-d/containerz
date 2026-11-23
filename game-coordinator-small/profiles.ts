import { randomUUID } from 'node:crypto';
import { mkdir, readFile, rename, rm, writeFile } from 'node:fs/promises';
import { join } from 'node:path';

export const games = {
  minecraft: {
    title: 'Minecraft',
    image: 'containerz/minecraft:dev',
    dataPath: '/home/player/.local/share/PineconeMC',
  },
  openarena: {
    title: 'OpenArena',
    image: 'containerz/openarena:dev',
    dataPath: '/home/player/.openarena',
  },
} as const;

export type GameId = keyof typeof games;
export const gameIds = Object.keys(games) as GameId[];

export const resolutions = {
  '720p': [1280, 720],
  '1080p': [1920, 1080],
  '1440p': [2560, 1440],
} as const;

export const frameRates = [30, 60, 120] as const;

export type Settings = {
  gpu: string;
  resolution: keyof typeof resolutions;
  fps: typeof frameRates[number];
};

type SavedGame = Settings & { port: number };

export type Profile = {
  id: string;
  name: string;
  games: Partial<Record<GameId, SavedGame>>;
};

export function profileStore(directory: string) {
  const path = join(directory, 'profiles.json');

  return {
    async read(): Promise<Profile[]> {
      try {
        return JSON.parse(await readFile(path, 'utf8'));
      } catch (error) {
        if ((error as NodeJS.ErrnoException).code === 'ENOENT') return [];
        throw error;
      }
    },

    async write(profiles: Profile[]) {
      await mkdir(directory, { recursive: true, mode: 0o700 });
      const temporary = join(directory, `profiles-${randomUUID()}.tmp`);
      try {
        await writeFile(temporary, JSON.stringify(profiles, null, 2) + '\n', { mode: 0o600, flag: 'wx' });
        await rename(temporary, path);
      } finally {
        await rm(temporary, { force: true });
      }
    },
  };
}
