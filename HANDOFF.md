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
  1280×720 `vkcube` frames on each GPU (RTX 5080 and AMD Raphael iGPU). The RTX 5080
  still exited with status 139 on shutdown.
- **`containerz/streamer`** streams `vkcube` to the existing browser client using
  the agent's named PipeWire capture. Playback, reconnecting, and resizing passed
  on both GPUs. The host LAN address also worked from the Linux host.
  [Streamer smoke test](images/streamer/README.md)

## Findings

- **gamescope needs a real GPU** with `VK_EXT_physical_device_drm`. lavapipe and WSL 2
  both fail with that error.
- **Weston 14** (`--backend=pipewire --renderer=pixman`) works on WSL as a fallback:
  Wayland and XWayland apps render, and `pipewiresrc target-object=weston.pipewire`
  captures it. But it has **no input path**, so it isn't a production option.
- **Input:** the agent's uinput devices would show up on the host kernel, which is
  unsafe with several containers. gamescope links `libeis`, so **libei** is the likely
  route. Not verified.
- **Networking:** WebRTC through Docker's NAT has the same problems as VM NAT
  (see the agent's `docs/LINUX_DEV.md`). The smoke test uses host networking.

## Next steps

1. Check whether gamescope accepts input through libei.
2. Verify audio capture from a container-local PipeWire sink.
3. Test from a second LAN device.
