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

The coordinator is at `http://127.0.0.1:3000` by default.

### Play from another device on your LAN

Build the streamer and game images with the current `web-remote-desktop`
checkout. Start the viewer and coordinator on the same LAN IPv4 address. For
example, run these from the repository root in separate terminals:

```bash
npm --prefix ../web-remote-desktop/client exec -- vite --host 192.168.1.20 --port 5173 --strictPort
CONTAINERZ_COORDINATOR_HOST=192.168.1.20 npm --prefix game-coordinator-small start
```

Open `http://192.168.1.20:3000` from the other device. **Open Viewer** uses the
same IP for the viewer and game agent. Relaunch games started before you
changed the bind address. Use a trusted LAN since devices that can reach the
coordinator and agents can manage games and connect to streams.
If the host filters traffic, allow TCP ports 3000, 5173, and the assigned game
ports in 8100–8999, plus WebRTC UDP traffic from the test device.

## Play

1. Open the coordinator and create or select a profile.
2. Pick a GPU, resolution, and frame rate on a game card, then click **Play**.
3. Click **Open Viewer** when the game is ready. It connects in a new tab.
4. Minecraft opens at the 26.3 title screen. Choose **Singleplayer** to create a
   world. The offline username follows the profile name when it fits Minecraft's
   3–16 character limit and uses only ASCII letters, digits, or underscores. Other names
   are converted to a stable Minecraft username.
5. Save and quit your Minecraft world before clicking **Stop Game**. Use
   Relative mouse mode for OpenArena. In HoloCure, use WASD and left-click
   during a stage to toggle mouse aiming.

Closing a viewer, switching profiles, or restarting the coordinator leaves
running games in Docker. Launch settings are recorded in `data/profiles.json`
and saves live in the game’s Docker volume. Stop a game before changing its
launch settings. By default the viewer and agent signaling use loopback. The
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
