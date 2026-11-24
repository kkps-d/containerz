# containerz/display: headless gamescope (Wayland + XWayland) with a PipeWire session.
FROM containerz/base:dev AS gamescope-builder

USER root
WORKDIR /build

# Pin the Ubuntu source these patches apply to.
ARG GAMESCOPE_SOURCE_VERSION=3.16.20+ds-1
RUN printf '%s\n' 'deb-src http://archive.ubuntu.com/ubuntu/ resolute multiverse' \
        > /etc/apt/sources.list.d/gamescope-source.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends build-essential dpkg-dev \
    && apt-get build-dep -y --no-install-recommends "gamescope=${GAMESCOPE_SOURCE_VERSION}" \
    && apt-get source "gamescope=${GAMESCOPE_SOURCE_VERSION}" \
    && rm -rf /var/lib/apt/lists/*

COPY patches/gamescope-pipewire-loop-locking.patch patches/gamescope-pipewire-cursor.patch \
     patches/gamescope-xwayland-fullscreen.patch /build/
RUN cd gamescope-*/ \
    && patch --batch --fuzz=0 -p1 < ../gamescope-pipewire-loop-locking.patch \
    && patch --batch --fuzz=0 -p1 < ../gamescope-pipewire-cursor.patch \
    && patch --batch --fuzz=0 -p1 < ../gamescope-xwayland-fullscreen.patch \
    && sed -i '1s/) /+containerz3) /' debian/changelog \
    && DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -b -uc -us -j8 \
    && install -D -m 0644 ../gamescope_*_amd64.deb /packages/gamescope.deb

FROM containerz/base:dev

USER root
RUN --mount=type=bind,from=gamescope-builder,source=/packages,target=/packages \
    apt-get update && apt-get install -y --no-install-recommends \
        /packages/gamescope.deb xwayland \
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
