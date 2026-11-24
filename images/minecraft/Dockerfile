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

RUN apt-get update && apt-get install -y --no-install-recommends openjdk-21-jdk-headless patch \
    && rm -rf /var/lib/apt/lists/*

# Patch the game entry point so instance-specific launch commands still run normally.
ARG PINECONE_ENTRYPOINT_SHA256=b3fef437ba982f14b73b584e51463c8f6856a3077d576dcfd6bc7f9c9149c0da
COPY VideoOptions.java video-options.patch /build/
RUN curl --proto '=https' --tlsv1.2 -fsSL --retry 3 \
        "https://raw.githubusercontent.com/ElyPrismLauncher/Launcher/${PINECONE_VERSION}/libraries/launcher/org/prismlauncher/EntryPoint.java" \
        -o EntryPoint.java \
    && printf '%s  EntryPoint.java\n' "$PINECONE_ENTRYPOINT_SHA256" | sha256sum -c - \
    && patch --fuzz=0 EntryPoint.java < video-options.patch \
    && launcher_jar=squashfs-root/share/ElyPrismLauncher/NewLaunch.jar \
    && javac --release 8 -Xlint:all,-options -Werror -cp "$launcher_jar" -d classes EntryPoint.java VideoOptions.java \
    && jar uf "$launcher_jar" -C classes org/prismlauncher

FROM containerz/base:dev AS seed

RUN apt-get update && apt-get install -y --no-install-recommends python3 \
    && rm -rf /var/lib/apt/lists/*

COPY prepare-seed.py /build/prepare-seed.py
RUN python3 /build/prepare-seed.py /seed

FROM containerz/streamer:dev

RUN apt-get update && apt-get install -y --no-install-recommends \
        openjdk-21-jre openjdk-25-jre fonts-dejavu-core libxcb-cursor0 python3 \
    && rm -rf /var/lib/apt/lists/* \
    && install -d -m 0700 -o player -g player \
        /home/player/.local /home/player/.local/share \
        /home/player/.local/share/PineconeMC \
        /home/player/.local/share/PineconeMC/instances

COPY --from=builder /build/squashfs-root /opt/pinecone
COPY --from=builder /build/start-pinecone /usr/local/bin/start-pinecone
COPY --from=seed /seed /opt/minecraft-seed
COPY --chmod=0755 start-minecraft.py /usr/local/bin/start-minecraft

CMD ["start-minecraft"]
