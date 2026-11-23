# Minecraft Java Edition

PineconeMC opens in headless gamescope. Use the browser viewer to create,
import, and launch Minecraft instances as `player`.

## Build

Build `containerz/streamer:dev` and install the browser client's dependencies
first. [Streamer setup](../streamer/README.md#build)

The build downloads and checks the pinned Linux x86-64 PineconeMC release.
[PineconeMC 11.1.2](https://github.com/ElyPrismLauncher/Launcher/releases/tag/11.1.2)

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
mouse mode and click the stream to capture the pointer. For local single-player
testing, open **Settings → Accounts → Add Offline** and choose a username.
Create or import an instance and click **Launch** to download its game files.

For a smoke test, create a vanilla **1.21.11** instance and select Java 21 in
the instance settings. The image also includes Java 25 for releases that
require it. Unmute audio in the viewer when you enter a world.

Save and quit your world before pressing Ctrl+C in the smoke launcher to stop
the container and browser client.
The local Docker volume `containerz-pinecone-data` keeps launcher settings,
profiles, downloads, instances, and worlds across runs. The previous Prism data
is still in `containerz-minecraft-data` and can be exported from Prism for import
into PineconeMC.

For remote play, use the Linux host's LAN address for both ports.
[LAN setup](../streamer/README.md#lan-testing)
