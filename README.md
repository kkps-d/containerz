# containerz

Ubuntu 26.04 container images for running games, streamed over WebRTC.

## Requirements

- Docker Engine on native Linux with a GPU for display and streamer runs
- Docker Desktop with WSL 2 for builds and CPU-only checks

## Usage

Each image lives in `images/<image>/` and has its own README.
For the browser smoke test, see the [streamer image](images/streamer/README.md).
For results by GPU and the checklist for adding a game, see [testing](docs/testing.md).

```powershell
docker build -t containerz/<image>:dev images/<image>
```

## For agents

See [AGENTS.md](AGENTS.md). Claude Code reads it through [CLAUDE.md](CLAUDE.md).
