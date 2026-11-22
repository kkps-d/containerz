FROM containerz/base:dev AS builder

USER root
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential pkg-config libgstreamer1.0-dev \
        libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev \
    && rm -rf /var/lib/apt/lists/*

# The agent's Cargo.lock includes crates that require Rust newer than Ubuntu's 1.93.
RUN curl --proto '=https' --tlsv1.2 -fsSL --retry 3 https://sh.rustup.rs -o /tmp/rustup.sh \
    && sh /tmp/rustup.sh -y --profile minimal --default-toolchain 1.100.0 --no-modify-path \
    && rm /tmp/rustup.sh
ENV PATH=/root/.cargo/bin:$PATH

WORKDIR /build/agent
COPY --from=streamer Cargo.toml Cargo.lock ./
COPY --from=streamer src ./src
RUN cargo build --locked --release

COPY input-visualizer.c piano-tester.c /build/
RUN apt-get update && apt-get install -y --no-install-recommends libx11-dev libsdl3-dev \
    && rm -rf /var/lib/apt/lists/* \
    && cc -O2 -Wall -Wextra -Werror /build/input-visualizer.c -lX11 -o /build/input-visualizer \
    && cc -O2 -Wall -Wextra -Werror /build/piano-tester.c $(pkg-config --cflags --libs sdl3) -lm -o /build/piano-tester

FROM containerz/display:dev

USER root
RUN apt-get update && apt-get install -y --no-install-recommends \
        gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad \
        gstreamer1.0-plugins-ugly gstreamer1.0-nice gstreamer1.0-pipewire \
        gstreamer1.0-tools libei1 libsdl3-0 pipewire-pulse \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /build/agent/target/release/agent /usr/local/bin/agent
COPY --from=builder /build/input-visualizer /usr/local/bin/input-visualizer
COPY --from=builder /build/piano-tester /usr/local/bin/piano-tester
COPY agent.toml /etc/containerz/agent.toml
COPY game-audio.conf /etc/pipewire/pipewire-pulse.conf.d/90-containerz.conf
COPY --chmod=0755 start-streamer.sh /usr/local/bin/start-streamer
COPY --chmod=0755 audio-test.sh /usr/local/bin/audio-test

ENV ENABLE_SSH=0 DISPLAY_AUDIO=1
CMD ["start-streamer"]
