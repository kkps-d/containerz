# display

Headless gamescope (Wayland + XWayland) with a PipeWire session, built on `containerz/base`.

## What's inside

- **gamescope** (headless backend) and **XWayland**
- **PipeWire** and **WirePlumber**, plus a session D-Bus
- **Mesa** Vulkan/GL drivers (lavapipe for software)
- **Test apps:** `vkcube`, `glxgears`

## Build

```bash
docker build -t containerz/base:dev images/base
docker build -t containerz/display:dev images/display
```

The builder stage patches Ubuntu's gamescope package with the PipeWire loop
fix and optional cursor capture. The streamer image enables cursor control
when the game's policy allows it. [Streamer cursor policy](../streamer/README.md#cursor-policy)

## Run on NVIDIA

Use a native Linux host with the NVIDIA Container Toolkit and Docker CDI enabled.
Set `gpu_pci` to the NVIDIA GPU's PCI address from `/dev/dri/by-path`.

```bash
gpu_pci=0000:01:00.0
video_gid=$(stat -Lc %g "/dev/dri/by-path/pci-${gpu_pci}-card")
render_gid=$(stat -Lc %g "/dev/dri/by-path/pci-${gpu_pci}-render")

docker run --rm -it --name containerz-display \
    --device nvidia.com/gpu=all \
    --group-add "$video_gid" --group-add "$render_gid" \
    containerz/display:dev
```

The default app is `vkcube`. To launch another app, append `start-display <command>`.
While the container is running, check for `node.name = "gamescope"` and `media.class = "Video/Source"`:

```bash
docker exec --user player containerz-display pw-cli ls Node
```

| Variable         | Default |
| ---------------- | ------- |
| `DISPLAY_WIDTH`  | `1280`  |
| `DISPLAY_HEIGHT` | `720`   |
| `DISPLAY_FPS`    | `60`    |

## Known issues

- **gamescope needs a GPU with a DRM device.** Without one it exits with
  `physical device doesn't support VK_EXT_physical_device_drm`. lavapipe and WSL 2 lack it.
- On the RTX 5080, gamescope 3.16.20 can exit with status 139 (SIGSEGV) after
  playback. GDB caught the shader compilation thread inside the NVIDIA Vulkan
  driver during teardown. Record the shutdown status separately from playback
  and launcher cleanup. [Upstream issue](https://github.com/ValveSoftware/gamescope/issues/2516) ·
  [Proposed teardown fix](https://github.com/ValveSoftware/gamescope/pull/2542)
