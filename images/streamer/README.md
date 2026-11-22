# streamer

Headless gamescope and `vkcube` streamed by the `web-remote-desktop` agent. The
agent runs as `player`, reads the `gamescope` PipeWire node, and sends keyboard
and mouse input to gamescope's EIS socket in the container. Game audio comes
from the monitor of the container-local `containerz-game-audio` stereo sink.

## Build

Use a native Linux host with Docker, Node.js 24, and the gamescope input changes
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
On the RTX 5080, gamescope can report status 139 during shutdown after playback.
[Display troubleshooting](../display/README.md#known-issues)

Use `--visual-input-test` to open the C/Xlib input tester. Click the streamed
window to focus it. The tester shows a crosshair, pointer coordinates, click
and scroll counts, and highlights every held key. Use the viewer's Input menu
to switch between Absolute and Relative mouse modes.

```bash
bash scripts/smoke-streamer.sh --visual-input-test nvidia 0000:01:00.0
```

Use `--input-test` to run `xev` and print the raw X11 events to the terminal.
The static `xev` window can need a repaint before gamescope sends a new first
frame after reconnecting.

### Audio

```bash
bash scripts/smoke-streamer.sh --audio-test nvidia 0000:01:00.0
```

Connect and choose **Unmute audio** in the browser toolbar. You should hear a
quiet 440 Hz tone about every second and a half while the visual tester responds
to input. Try mute and reconnect, then press Ctrl+C to stop the test.

The streamer starts PipeWire PulseAudio inside the container and directs game
clients to its private socket and sink. The agent reads only the sink named by
`[capture].pipewire_audio_sink`. If audio is unavailable, stream info reports
the reason and video and input continue.

### Piano

```bash
bash scripts/smoke-streamer.sh --piano-test nvidia 0000:01:00.0
```

Unmute audio, then click the stream and play the C3–C5 keyboard. Use Absolute
mouse mode for clicks and drags, or play chords with the computer keys marked
on the piano. Keys light up while held. **Touch contacts** counts independent
touches that reach the app. The current gamescope streamer sends touchscreen
gestures as mouse input, so that counter stays at zero over this connection.

To test native multitouch directly on a Linux touchscreen desktop, build the
same app locally with `libsdl3-dev` installed.

```bash
cc -O2 images/streamer/piano-tester.c $(pkg-config --cflags --libs sdl3) -lm -o /tmp/piano-tester
/tmp/piano-tester
```

Hold two notes with two fingers, then lift one. The other key should stay lit
and keep sounding while the contact count drops from two to one.

For a second device on the LAN, use the Linux host's LAN address for both
ports. The launcher uses host networking so browsers can reach the agent's
WebRTC UDP ports directly, which avoids Docker bridge NAT. Run this smoke test
on a trusted LAN because signaling uses unauthenticated HTTP. Allow TCP ports
5173 and 8080 and WebRTC UDP traffic from the test device if the host firewall
filters them.
