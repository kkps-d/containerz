# base

The base for all other images: Ubuntu 26.04, a non-root user, and debugging tools.

## What's inside

- **User:** `player` (UID/GID 1000), with passwordless `sudo`
- **Init:** `tini` as PID 1
- **Runtime dir:** `XDG_RUNTIME_DIR=/run/user/1000`, for Wayland and PipeWire sockets
- **Locale:** `C.UTF-8`, timezone UTC
- **Tools:** nano, less, curl, wget, ip, ping, dig, nc, htop, ps, lsof, strace, file, tree, unzip, xz, zstd
- **SSH server:** on by default, key-only, no root login

## Build

```powershell
docker build -t containerz/base:dev images/base
```

## Run

```powershell
docker run --rm -it containerz/base:dev
```

### With SSH

The image trusts a shared dev key, `ssh/containerz_dev.pub`. Its private half
lives at `~/.ssh/containerz_dev` on the host and is never committed.

```powershell
docker run -d --name cz-dev -p 2222:22 containerz/base:dev sleep infinity
```

```powershell
ssh cz
```

`cz` is a shortcut in `~/.ssh/config` for `player@localhost:2222` with the dev key.
To use another port, run `ssh -p <port> -i ~/.ssh/containerz_dev player@localhost`.

To trust an extra key, pass `-e SSH_PUBLIC_KEY="..."`.

## Settings

| Variable         | Default | Purpose                              |
| ---------------- | ------- | ------------------------------------ |
| `ENABLE_SSH`     | `1`     | Set to `0` to skip starting sshd     |
| `SSH_PUBLIC_KEY` | (none)  | Public key added to `authorized_keys` |

## Notes

- The entrypoint starts as root so it can start sshd, then switches to `player`.
- The container command keeps supplementary groups passed with Docker's `--group-add`, excluding group 0.
- `ssh` (the client) is present because Ubuntu's server package depends on it.
