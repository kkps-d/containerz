# HoloCure

The standalone Windows build runs in Wine under gamescope and streams to the
browser. WineD3D handles Direct3D 11. Use a native Linux host with Docker and
an NVIDIA or AMD GPU.

## Build

Build `containerz/streamer:dev` and install the browser client's dependencies
first. [Streamer setup](../streamer/README.md#build)

```bash
docker build --platform linux/amd64 -t containerz/holocure:dev images/holocure
```

## Play in a browser

Run from the repo root with your local copy of `HoloCure.zip`. Use the PCI
address of the GPU from `/dev/dri/by-path`.

```bash
HOLOCURE_GAME_DIR=$(mktemp -d)
export HOLOCURE_GAME_DIR
unzip -q HoloCure.zip -d "$HOLOCURE_GAME_DIR"
bash scripts/smoke-streamer.sh --holocure nvidia 0000:01:00.0
# Or, after stopping the NVIDIA test:
bash scripts/smoke-streamer.sh --holocure amd 0000:0d:00.0
```

If the Remote Desktop viewer is already serving port 5173, set
`HOLOCURE_REUSE_CLIENT=1` when running the launcher. Open
`http://localhost:5173`, connect to `localhost:8080`, and unmute audio.
Click into the stream and use WASD to move. Left-click toggles mouse aiming
during a stage. Press Ctrl+C in the launcher when finished.

The launcher mounts the extracted game at `/opt/holocure` read-only.
The game runs from a writable copy in `/home/player/.wine-holocure/game`,
where this build puts its settings and saves. The volumes
`containerz-holocure-smoke-nvidia` and `containerz-holocure-smoke-amd`
keep their respective saves across container recreation.

The [game coordinator](../../game-coordinator-small/README.md) also launches
HoloCure and keeps a separate Wine volume for each profile.

The launcher starts the Wine server outside gamescope's process tree and
stops it after gamescope exits so Wine's device helpers can finish shutting down.
When you quit from the game menu, the streamer reports status 1 because
its game session ended first.
