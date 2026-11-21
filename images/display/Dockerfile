# containerz/display: headless gamescope (Wayland + XWayland) with a PipeWire session.
FROM containerz/base:dev

USER root
RUN apt-get update && apt-get install -y --no-install-recommends \
        gamescope xwayland \
        libvulkan1 mesa-vulkan-drivers libegl1 libgl1 libgles2 \
        pipewire pipewire-bin wireplumber dbus-daemon dbus-user-session \
        vulkan-tools mesa-utils x11-utils \
    && rm -rf /var/lib/apt/lists/*

COPY --chmod=0755 start-display.sh /usr/local/bin/start-display

# Ubuntu installs gamescope in /usr/games.
ENV PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games \
    DISPLAY_WIDTH=1280 \
    DISPLAY_HEIGHT=720 \
    DISPLAY_FPS=60

CMD ["start-display"]
