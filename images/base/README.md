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

Pass your public key, then connect as `player`:

```powershell
docker run --rm -it -p 2222:22 -e SSH_PUBLIC_KEY="$(Get-Content ~/.ssh/id_ed25519.pub)" containerz/base:dev
```

```powershell
ssh -p 2222 player@localhost
```

You can also mount a key file at `/home/player/.ssh/authorized_keys`.

## Settings

| Variable         | Default | Purpose                              |
| ---------------- | ------- | ------------------------------------ |
| `ENABLE_SSH`     | `1`     | Set to `0` to skip starting sshd     |
| `SSH_PUBLIC_KEY` | (none)  | Public key added to `authorized_keys` |

## Notes

- The entrypoint starts as root so it can start sshd, then switches to `player`.
- `ssh` (the client) is present because Ubuntu's server package depends on it.
