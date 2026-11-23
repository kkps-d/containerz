FROM containerz/streamer:dev

RUN apt-get update && apt-get install -y --no-install-recommends openarena libpulse0 \
    && rm -rf /var/lib/apt/lists/* \
    && install -d -m 0700 -o player -g player /home/player/.openarena

# SDL's warp mode keeps relative aiming moving across XWayland cursor boundaries.
CMD ["start-streamer", "env", "SDL_VIDEODRIVER=x11", "SDL_AUDIODRIVER=pulseaudio", "SDL_MOUSE_RELATIVE_MODE_WARP=1", \
     "/usr/games/openarena", "+set", "fs_homepath", "/home/player/.openarena", "+set", "s_useOpenAL", "0", \
     "+set", "r_mode", "-2", "+set", "r_fullscreen", "1"]
