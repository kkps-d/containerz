# Minecraft Java Edition

The container opens the Minecraft Java Edition 26.3 title screen in headless
gamescope as `player`. Choose **Singleplayer** to create your first world.

## Build

Build `containerz/streamer:dev` and install the browser client's dependencies
first. [Streamer setup](../streamer/README.md#build)

The build verifies the pinned Linux x86-64 PineconeMC release and downloads the
26.3 client files for offline startup with Java 25.
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
mouse mode and click the stream to capture the pointer. Gamescope streams the
cursor in menus and inventory. Unmute audio in the viewer when you enter a world.

A new volume starts in American English. Set `MINECRAFT_OFFLINE_NAME` before
the first launch to choose the offline username. It must be 3–16 ASCII letters,
digits, or underscores and defaults to `Player` for standalone launches.
The coordinator supplies a username based on the selected profile. Later boots
reuse the stored username.

Each Minecraft launch sets **Max Framerate** to the requested `DISPLAY_FPS`
and turns **VSync** on. The coordinator passes the FPS selected on the game
card. Standalone launches use 60 fps by default. These two settings are applied
again when you relaunch an instance from PineconeMC.

Save and quit your world before pressing Ctrl+C in the smoke launcher to stop
the container and browser client. The local Docker volume
`containerz-pinecone-data` keeps launcher settings, accounts, downloads,
instances, and worlds across runs. On first boot after the update, an existing
volume gets the prepared 26.3 instance alongside its current instances.
The previous Prism data is still in `containerz-minecraft-data` and can be
exported from Prism for import into PineconeMC.

For remote play, use the Linux host's LAN address for both ports.
[LAN setup](../streamer/README.md#lan-testing)

## Open the launcher

To manage instances in PineconeMC, stop the running container and override the
command by appending `start-streamer env QT_QPA_PLATFORM=xcb start-pinecone`
after the image name in `docker run`. Reuse the same GPU and volume options.
To relaunch the prepared instance manually, choose **Launch Offline** in its
launch menu. The image also includes Java 21 for older instances.
