# Game coordinator

Run Minecraft and OpenArena from a local browser page. Each profile can run one
instance of each game. Launch settings and game data are kept per profile and
game.

## Setup

Use Node.js 24.2 or later in the Node 24 series, Docker, and a native Linux host
with an NVIDIA or AMD GPU. Build the base and display images first, then build
the streamer with the sibling viewer checkout and rebuild both game images.
[Streamer build](../images/streamer/README.md#build)

From the repository root:

```bash
docker build -t containerz/minecraft:dev images/minecraft
docker build -t containerz/openarena:dev images/openarena
npm --prefix ../web-remote-desktop/client exec -- vite --host 127.0.0.1 --port 5173 --strictPort
```

Start the coordinator in another terminal from the repository root:

```bash
cd game-coordinator-small
npm start
```

## Play

1. Open http://127.0.0.1:3000 and create or select a profile.
2. Pick a GPU, resolution, and frame rate on a game card, then click **Play**.
3. Click **Open Viewer** when the game is ready. It connects in a new tab.
4. Save and quit your Minecraft world before clicking **Stop Game**. Use
   Relative mouse mode for OpenArena.

Closing a viewer, switching profiles, or restarting the coordinator leaves
running games in Docker. Launch settings are recorded in `data/profiles.json`
and saves live in the game’s Docker volume. Stop a game before changing its
launch settings. The viewer and agent signaling use loopback, and the
coordinator assigns each profile and game a stable port in 8100–8999.

## Quick check

Start both games under one profile, then start Minecraft under another. Open
each viewer and check its stream settings. Stop one game and check that the
other games are still running. Relaunch the stopped game to check its saved
settings and data.
