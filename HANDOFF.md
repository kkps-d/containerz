# Handoff: moving to the Linux host

Summary of the WSL 2 work and the first native Linux display test.
Rules and conventions are in [AGENTS.md](AGENTS.md); this file is only the story so far.
Delete it once it's no longer useful.

## Goal

Containerized games streamed over WebRTC by the
[web-remote-desktop](https://github.com/kkps-d/web-remote-desktop) agent.
Built in small steps: base, display, audio, GPU, games.

## Done

- **`containerz/base`** works: Ubuntu 26.04, user `player` (UID 1000) with passwordless
  sudo, `tini`, debug tools, SSH on by default (key-only).
- **Dev SSH key:** the image trusts `images/base/ssh/containerz_dev.pub`. The private
  key is `~/.ssh/containerz_dev` on the Windows machine. Copy it over, or make a new
  pair and replace the `.pub`.
- **`containerz/display`** builds and runs on the RTX 5080. With NVIDIA CDI and the
  GPU's DRM groups, `vkcube` renders through XWayland and gamescope publishes a
  PipeWire video node. Capture and shutdown issues are in the [display README](images/display/README.md).
- **PipeWire capture:** the display image now backports the upstream loop-locking
  fix to Ubuntu's gamescope 3.16.20. The rebuilt image delivered 16 distinct
  1280×720 `vkcube` frames on each GPU (RTX 5080 and AMD Raphael iGPU).
- **`containerz/streamer`** streams `vkcube` to the existing browser client using
  the agent's named PipeWire capture. Playback, reconnecting, and resizing passed
  on both GPUs. The host LAN address also worked from the Linux host.
  [Streamer smoke test](images/streamer/README.md)
- **Browser input** works through gamescope's EIS socket on the RTX 5080. Firefox
  delivered keyboard input, clicks, scrolling, and both pointer modes to `xev`.
  Absolute motion still matched the native display after resizing the stream.
  Held input was released on channel close, viewer replacement, and shutdown.
  With a missing EIS socket, the viewer showed the reason and the cube kept rotating.
  The visual smoke tester runs inside XWayland and shows mouse movement and held keys.
- **Game audio** streams the monitor of the container-local PipeWire sink as
  Opus. On the RTX 5080, Firefox decoded the test tone after reconnecting and
  resizing, and video and input stayed live when the sink was missing or removed.
  The piano tester exercises audio and browser input. The browser smoke test
  also passed from a second device using the Linux host's LAN address.
  [Audio smoke test](images/streamer/README.md#audio)

## Findings

- **RTX 5080 shutdown:** gamescope can exit with status 139 after playback. GDB
  caught the shader compilation thread inside the NVIDIA Vulkan driver during
  teardown. Playback, reconnect, and launcher cleanup passed. This can make a
  normal session stop look like a crash in monitoring. Track the proposed Vulkan
  teardown fix separately from input and audio work. [Upstream patch](https://github.com/ValveSoftware/gamescope/pull/2542)
- **gamescope needs a real GPU** with `VK_EXT_physical_device_drm`. lavapipe and WSL 2
  both fail with that error.
- **Weston 14** (`--backend=pipewire --renderer=pixman`) works on WSL as a fallback:
  Wayland and XWayland apps render, and `pipewiresrc target-object=weston.pipewire`
  captures it. But it has **no input path**, so it isn't a production option.
- **Input:** headless gamescope publishes `gamescope-0-ei` in `player`'s private
  runtime directory. Browser input passed with `/dev/uinput` absent. The static
  `xev` window needed a repaint for gamescope to send a new first frame after reconnect.
- **Networking:** WebRTC through Docker's NAT has the same problems as VM NAT
  (see the agent's `docs/LINUX_DEV.md`). The smoke test uses host networking.

## Next steps

1. Stream a game and verify video, audio, and controls on both GPUs.
