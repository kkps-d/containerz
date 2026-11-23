FROM containerz/base:dev AS builder

RUN apt-get update && apt-get install -y --no-install-recommends build-essential libx11-dev \
    && rm -rf /var/lib/apt/lists/*

# Pin the release and checksum so local builds use the tested launcher.
ARG PINECONE_VERSION=11.1.2
ARG PINECONE_SHA256=068129b472da7658dd77919250cfb88f38e2b43a65ab92239e0b5a726ba9444d
WORKDIR /build
RUN curl --proto '=https' --tlsv1.2 -fsSL --retry 3 \
        "https://github.com/ElyPrismLauncher/Launcher/releases/download/${PINECONE_VERSION}/PineconeMC-Linux-x86_64.AppImage" \
        -o pinecone.AppImage \
    && printf '%s  pinecone.AppImage\n' "$PINECONE_SHA256" | sha256sum -c - \
    && chmod +x pinecone.AppImage \
    && ./pinecone.AppImage --appimage-extract >/dev/null \
    && rm pinecone.AppImage

COPY start-pinecone.c /build/start-pinecone.c
RUN cc -O2 -Wall -Wextra -Werror start-pinecone.c -lX11 -o start-pinecone

FROM containerz/streamer:dev

RUN apt-get update && apt-get install -y --no-install-recommends \
        openjdk-21-jre openjdk-25-jre fonts-dejavu-core libxcb-cursor0 \
    && rm -rf /var/lib/apt/lists/* \
    && install -d -m 0700 -o player -g player \
        /home/player/.local /home/player/.local/share \
        /home/player/.local/share/PineconeMC \
        /home/player/.local/share/PineconeMC/instances

COPY --from=builder /build/squashfs-root /opt/pinecone
COPY --from=builder /build/start-pinecone /usr/local/bin/start-pinecone

CMD ["start-streamer", "env", "QT_QPA_PLATFORM=xcb", "start-pinecone"]
