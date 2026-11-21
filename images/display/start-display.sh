#!/bin/bash
# Starts the session bus, PipeWire and headless gamescope, then runs the app.
# Usage: start-display [app args...]   (default app: vkcube)
set -euo pipefail

log() { echo "start-display: $*" >&2; }

# Session bus, for PipeWire and WirePlumber.
export DBUS_SESSION_BUS_ADDRESS="unix:path=$XDG_RUNTIME_DIR/bus"
dbus-daemon --session --address="$DBUS_SESSION_BUS_ADDRESS" --fork --nopidfile

pipewire &
until [ -S "$XDG_RUNTIME_DIR/pipewire-0" ]; do sleep 0.1; done
wireplumber &
log "PipeWire ready"

[ $# -eq 0 ] && set -- vkcube

exec gamescope --backend headless \
    -W "$DISPLAY_WIDTH" -H "$DISPLAY_HEIGHT" -r "$DISPLAY_FPS" \
    -- "$@"
