# streamer

Headless gamescope and `vkcube` streamed by the `web-remote-desktop` agent. The
agent runs as `player` and reads the `gamescope` PipeWire node. This image streams video only.

## Build

Use a native Linux host with Docker, Node.js 24, and the named-capture changes
in the sibling `web-remote-desktop` checkout. Run these from the `containerz` root.

```bash
docker build -t containerz/base:dev images/base
docker build -t containerz/display:dev images/display
docker build --build-context streamer=../web-remote-desktop/agent \
    -t containerz/streamer:dev images/streamer
npm --prefix ../web-remote-desktop/client ci
```

## Test in a browser

Choose the GPU's PCI address from `/dev/dri/by-path`. This example uses NVIDIA
CDI. For AMD, pass `amd` and the AMD GPU's PCI address. The optional third
argument is the path to the `web-remote-desktop` checkout.

```bash
bash scripts/smoke-streamer.sh nvidia 0000:01:00.0
```

Open `http://localhost:5173`, enter `localhost:8080`, and confirm the cube
rotates. Disconnect and reconnect, then change the resolution and check that
video continues. Press Ctrl+C in the launcher to stop both services.
On the RTX 5080, gamescope can report status 139 during shutdown after playback
has completed.

For a second device on the LAN, use the Linux host's LAN address for both
ports. Run this smoke test on a trusted LAN because it uses unauthenticated
HTTP signaling and host networking. Allow TCP ports 5173 and 8080 and WebRTC
UDP traffic from the test device if the host firewall filters them.
