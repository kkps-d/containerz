#!/bin/bash
# Runs as root: optional setup, then hands off to the non-root user.
set -euo pipefail

user="${CONTAINERZ_USER:-player}"
home="/home/${user}"

# Runtime dir may be missing if /run is a fresh tmpfs.
install -d -m 0700 -o "$user" -g "$user" "$XDG_RUNTIME_DIR"

if [ "${ENABLE_SSH:-0}" = "1" ]; then
    # Optional: key passed as an env var instead of a mounted file.
    if [ -n "${SSH_PUBLIC_KEY:-}" ]; then
        echo "$SSH_PUBLIC_KEY" >> "${home}/.ssh/authorized_keys"
    fi
    if [ -f "${home}/.ssh/authorized_keys" ]; then
        chown "$user:$user" "${home}/.ssh/authorized_keys" 2>/dev/null || true
        chmod 600 "${home}/.ssh/authorized_keys" 2>/dev/null || true
    else
        echo "entrypoint: ENABLE_SSH=1 but no authorized_keys; SSH login will fail" >&2
    fi
    ssh-keygen -A >/dev/null
    mkdir -p /run/sshd
    /usr/sbin/sshd
fi

exec setpriv --reuid="$user" --regid="$user" --init-groups \
    env HOME="$home" USER="$user" "$@"
