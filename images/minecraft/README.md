# Minecraft Java Edition

Prism Launcher opens in headless gamescope. Use the browser viewer to create,
import, and launch Minecraft instances as `player`.

## Build

Build `containerz/streamer:dev` and install the browser client's dependencies
first. [Streamer setup](../streamer/README.md#build)

```bash
docker build -t containerz/minecraft:dev images/minecraft
```

## Play in a browser

Run one of these commands from the repo root. Use the GPU's PCI address from
`/dev/dri/by-path`.

```bash
bash scripts/smoke-streamer.sh --minecraft nvidia 0000:01:00.0
bash scripts/smoke-streamer.sh --minecraft amd 0000:0d:00.0
```

Open `http://localhost:5173` and connect to `localhost:8080`. Choose **Relative**
mouse mode and click the stream to capture the pointer. Add a Microsoft account
that owns Minecraft Java Edition, then follow the sign-in prompt in your own
browser. Create or import an instance and click **Launch** to download its game
files.

For a smoke test, create a vanilla **1.21.11** instance and select Java 21 in
Prism's instance settings. The image also includes Java 25 for releases that
require it. Unmute audio in the viewer when you enter a world.

Save and quit your world before pressing Ctrl+C in the smoke launcher to stop
the container and browser client.
The local Docker volume `containerz-minecraft-data` keeps Prism settings,
account state, downloads, instances, and worlds across runs. For remote play,
use the Linux host's LAN address for both ports. [LAN setup](../streamer/README.md#lan-testing)
