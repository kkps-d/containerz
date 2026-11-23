# OpenArena

OpenArena runs fullscreen in headless gamescope. The streamer sends video and
game audio to the browser and forwards keyboard and mouse input.

## Build

Build `containerz/streamer:dev` and install the browser client's dependencies
first. [Streamer setup](../streamer/README.md#build)

```bash
docker build -t containerz/openarena:dev images/openarena
```

## Play in a browser

Run one of these commands from the repo root. Replace the PCI address with the
one for your GPU in `/dev/dri/by-path`.

```bash
bash scripts/smoke-streamer.sh --openarena nvidia 0000:01:00.0
bash scripts/smoke-streamer.sh --openarena amd 0000:0d:00.0
```

Open `http://localhost:5173` and connect to `localhost:8080`. Unmute audio in
the viewer toolbar. Choose **Input → Mouse → Relative** and click the stream
to capture the mouse. Open **Single Player → Skirmish** and start a match with
bots. OpenArena uses relative mouse movement for both menus and aiming. Press
Escape to release the browser's mouse lock.

For a LAN test, use the Linux host's LAN address for both ports. Press Ctrl+C
in the launcher to stop the game and browser client. [LAN setup](../streamer/README.md#lan-testing)

The [local game coordinator](../../game-coordinator-small/README.md) mounts a
profile's OpenArena volume at `/home/player/.openarena` to keep game settings.
