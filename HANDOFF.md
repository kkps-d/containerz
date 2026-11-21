# Handoff: moving to the Linux host

Summary of the first session (on Windows + WSL 2), for picking up on the Linux machine.
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
- **`containerz/display`** (work in progress): gamescope headless + XWayland + PipeWire.
  Builds, but never ran: WSL 2 has no GPU DRM device.

## Findings

- **gamescope needs a real GPU** with `VK_EXT_physical_device_drm`. lavapipe and WSL 2
  both fail with that error.
- **Weston 14** (`--backend=pipewire --renderer=pixman`) works on WSL as a fallback:
  Wayland and XWayland apps render, and `pipewiresrc target-object=weston.pipewire`
  captures it. But it has **no input path**, so it isn't a production option.
- **The agent needs a new capture mode.** Today it uses GNOME's ScreenCast portal. In a
  container it should read a named PipeWire video stream (gamescope's or Weston's)
  with `pipewiresrc` directly. Its PipeWire audio capture should mostly work as is.
- **Input:** the agent's uinput devices would show up on the host kernel, which is
  unsafe with several containers. gamescope links `libeis`, so **libei** is the likely
  route. Not verified.
- **Networking:** WebRTC through Docker's NAT has the same problems as VM NAT
  (see the agent's `docs/LINUX_DEV.md`). Try host networking or a fixed UDP range.

## Next steps

1. Set up the host: NVIDIA `-open` driver via `ubuntu-drivers`, Docker Engine, NVIDIA
   Container Toolkit (`NVIDIA_DRIVER_CAPABILITIES=all`). Monitor on the RTX 5080, not the
   AMD iGPU.
2. Run `containerz/display` with `--gpus all`. Confirm gamescope starts, `vkcube` renders,
   and gamescope publishes a PipeWire video stream (`pw-cli ls Node`).
3. Check whether gamescope accepts input through libei.
4. Then: add the PipeWire-node capture mode to the agent, package it, solve networking.
