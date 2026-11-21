# display

> [!WARNING]
> Work in progress. Builds, but gamescope fails on WSL 2. Test on a Linux host with a GPU.

Headless gamescope (Wayland + XWayland) with a PipeWire session, built on `containerz/base`.

## What's inside

- **gamescope** (headless backend) and **XWayland**
- **PipeWire** and **WirePlumber**, plus a session D-Bus
- **Mesa** Vulkan/GL drivers (lavapipe for software)
- **Test apps:** `vkcube`, `glxgears`

## Build

```powershell
docker build -t containerz/display:dev images/display
```

## Run

Starts gamescope with `vkcube`. Pass another command to run that instead.

```powershell
docker run --rm -it containerz/display:dev
```

| Variable         | Default |
| ---------------- | ------- |
| `DISPLAY_WIDTH`  | `1280`  |
| `DISPLAY_HEIGHT` | `720`   |
| `DISPLAY_FPS`    | `60`    |

## Known issues

- **gamescope needs a GPU with a DRM device.** Without one it exits with
  `physical device doesn't support VK_EXT_physical_device_drm`. lavapipe and WSL 2 lack it.
- Ubuntu installs gamescope in `/usr/games`, so the image adds it to `PATH`.
