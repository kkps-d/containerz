FROM containerz/base:dev AS agent-builder

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

FROM containerz/display:dev

USER root
RUN apt-get update && apt-get install -y --no-install-recommends \
        gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad \
        gstreamer1.0-plugins-ugly gstreamer1.0-nice gstreamer1.0-pipewire \
    && rm -rf /var/lib/apt/lists/*

COPY --from=agent-builder /build/agent/target/release/agent /usr/local/bin/agent
COPY agent.toml /etc/containerz/agent.toml
COPY --chmod=0755 start-streamer.sh /usr/local/bin/start-streamer

ENV ENABLE_SSH=0
CMD ["start-streamer"]
