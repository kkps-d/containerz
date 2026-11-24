# Game coordinator

Run Minecraft, OpenArena, and HoloCure from a local browser page. Each profile
can run one instance of each game and keeps its own launch settings and saves.

## Setup

Use Node.js 24.2 or later in the Node 24 series, Docker, and a native Linux host
with an NVIDIA or AMD GPU. Build the base and display images first, then build
the streamer with the sibling viewer checkout and build the game images.
[Streamer build](../images/streamer/README.md#build)

From the repository root:

```bash
docker build -t containerz/minecraft:dev images/minecraft
docker build -t containerz/openarena:dev images/openarena
docker build -t containerz/holocure:dev images/holocure
mkdir -p game-coordinator-small/data/holocure
unzip -n HoloCure.zip -d game-coordinator-small/data/holocure
npm --prefix ../web-remote-desktop/client exec -- vite --host 127.0.0.1 --port 5173 --strictPort
```

If you already extracted HoloCure elsewhere, set `HOLOCURE_GAME_DIR` to that
directory before starting the coordinator.

Start the coordinator in another terminal from the repository root:

```bash
cd game-coordinator-small
npm start
```

### Test the coordinator on your LAN

Set `CONTAINERZ_COORDINATOR_HOST` to the game host's LAN IPv4 address when
starting the coordinator. For example, with `192.168.1.20`:

```bash
CONTAINERZ_COORDINATOR_HOST=192.168.1.20 npm start
```

Open `http://192.168.1.20:3000` from another device on your network. Use a
trusted LAN since anyone who can reach the page can manage profiles and games.
Open the game viewer on the game host, where the viewer and signaling listen
on loopback.

## Play

1. Open http://127.0.0.1:3000 and create or select a profile.
2. Pick a GPU, resolution, and frame rate on a game card, then click **Play**.
3. Click **Open Viewer** when the game is ready. It connects in a new tab.
4. Save and quit your Minecraft world before clicking **Stop Game**. Use
   Relative mouse mode for OpenArena. In HoloCure, use WASD and left-click
   during a stage to toggle mouse aiming.

Closing a viewer, switching profiles, or restarting the coordinator leaves
running games in Docker. Launch settings are recorded in `data/profiles.json`
and saves live in the game’s Docker volume. Stop a game before changing its
launch settings. The viewer and agent signaling use loopback, and the
coordinator assigns each profile and game a stable port in 8100–8999.

Select a profile and click **Rename Profile** to change its display name while
keeping its saves and launch settings. **Delete Profile** stops that profile’s
running games and permanently deletes all of its saves and launch settings.

## Quick check

Start two games under one profile, then start one under another. Open
each viewer and check its stream settings. Stop one game and check that the
other games are still running. Relaunch the stopped game to check its saved
settings and data. Rename a profile and verify that it still has its saves.
Delete a disposable profile and verify that its game volumes are gone.
