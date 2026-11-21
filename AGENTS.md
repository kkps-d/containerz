# AGENTS.md

Guidance for AI coding agents (Claude Code, Codex) working in this repo.

## Project

Container images for running games, streamed over WebRTC. The streamer already
exists elsewhere; this repo only builds and runs the game containers.

We build in small steps: base image, then display, audio, GPU, and finally games.
Only build what the current step needs.

## Environment

- **Development:** Ubuntu 26.04 on a native Linux host with an NVIDIA GPU
  (RTX 5080), Docker Engine and the NVIDIA Container Toolkit.
- **WSL 2 (Docker Desktop):** builds and CPU-only tests only. It has no
  `/dev/dri`, so gamescope can't run, and Vulkan falls back to software.
- **Architecture:** `linux/amd64` only.
- **Registry:** none. Images stay local. Never push.

## Image rules

- **Base:** every image starts from `ubuntu:26.04`, directly or through another image here.
- **Naming:** `containerz/<image>:<tag>`, where `<image>` is the folder name.
  - `:dev` for local work.
  - `:YYYY.MM.DD` for snapshots worth keeping.
- **User:** run as `player` (UID 1000), inherited from `containerz/base`. Root-only
  setup goes in the entrypoint, before it drops to `player`.
- **SSH:** images trust the shared dev key `images/base/ssh/containerz_dev.pub`.
  Never commit a private key.
- **Line endings:** LF only (enforced by `.gitattributes`). CRLF breaks scripts.
- **Size:** use `--no-install-recommends` and clean apt lists in the same layer.
- **Versions:** pin package versions when stability matters, and say why in a comment.

## Target stack

| Layer   | Choice                                       |
| ------- | -------------------------------------------- |
| Display | Wayland, with gamescope as the compositor    |
| X11     | XWayland, for games that need it             |
| Audio   | PipeWire                                     |
| GPU     | NVIDIA now; AMD later (vendor-agnostic)      |
| Capture | gamescope's PipeWire video stream            |
| Input   | libei into gamescope (not yet verified)      |

The streamer is [web-remote-desktop](https://github.com/kkps-d/web-remote-desktop)'s
agent. It needs a capture mode that reads a named PipeWire stream instead of the
GNOME ScreenCast portal.

### GPU: stay vendor-agnostic

Production runs on both NVIDIA and AMD. Only NVIDIA is tested for now.

- Don't hard-code NVIDIA in shared images. Put vendor-specific setup in its own
  layer or at runtime (for example `--gpus all` vs `--device /dev/dri`).
- Prefer standard APIs (Vulkan, EGL, GBM) over vendor-only ones.
- Don't add AMD validation yet.

## Layout

```
images/<image>/
  Dockerfile
  README.md      # what it is, how to build and run it
```

## Commands

```powershell
docker build -t containerz/<image>:dev images/<image>
docker run --rm -it containerz/<image>:dev
```

## Checks before finishing

- The image builds.
- A quick smoke test runs (for example, start the container and run its main command).
- The image's `README.md` is up to date.

## Documentation style

Keep docs short and easy for people to read. Use clear sections. Write down only
what someone needs to build, run, or change the image. Leave out anything else.
