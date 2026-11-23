FROM containerz/base:dev AS builder

RUN apt-get update && apt-get install -y --no-install-recommends build-essential libx11-dev \
    && rm -rf /var/lib/apt/lists/*

COPY start-prism.c /build/start-prism.c
RUN cc -O2 -Wall -Wextra -Werror /build/start-prism.c -lX11 -o /build/start-prism

FROM containerz/streamer:dev

RUN apt-get update && apt-get install -y --no-install-recommends \
        prismlauncher openjdk-21-jre openjdk-25-jre fonts-dejavu-core \
    && rm -rf /var/lib/apt/lists/* \
    && install -d -m 0700 -o player -g player \
        /home/player/.local /home/player/.local/share \
        /home/player/.local/share/PrismLauncher \
        /home/player/.local/share/PrismLauncher/instances

COPY --from=builder /build/start-prism /usr/local/bin/start-prism

CMD ["start-streamer", "env", "QT_QPA_PLATFORM=xcb", "start-prism"]
