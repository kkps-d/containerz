# containerz

Ubuntu 26.04 container images for running games, streamed over WebRTC.

## Requirements

- Docker Desktop with the WSL 2 backend
- An NVIDIA GPU, for GPU images

## Usage

Each image lives in `images/<image>/` and has its own README.

```powershell
docker build -t containerz/<image>:dev images/<image>
```

## For agents

See [AGENTS.md](AGENTS.md). Claude Code reads it through [CLAUDE.md](CLAUDE.md).
