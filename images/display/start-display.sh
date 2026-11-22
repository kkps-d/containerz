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

if [[ ${DISPLAY_AUDIO:-0} == 1 ]]; then
    export PULSE_SERVER="unix:$XDG_RUNTIME_DIR/pulse/native"
    export PULSE_SINK=containerz-game-audio
    pipewire-pulse &
    pulse_pid=$!

    audio_ready() {
        [[ -S "$XDG_RUNTIME_DIR/pulse/native" ]] &&
            timeout 0.3s pw-cli ls Node 2>/dev/null | awk -v sink="$PULSE_SINK" '
                /^[[:space:]]*id / { named = audio = 0 }
                $1 == "node.name" && $3 == "\"" sink "\"" { named = 1 }
                /media\.class = "Audio\/Sink"$/ { audio = 1 }
                named && audio { found = 1 }
                END { exit !found }
            '
    }

    deadline=$((SECONDS + 3))
    until audio_ready; do
        if ! kill -0 "$pulse_pid" 2>/dev/null; then
            log "PipeWire PulseAudio service stopped before publishing $PULSE_SINK"
            break
        fi
        if (( SECONDS >= deadline )); then
            log "timed out waiting for the PulseAudio socket and $PULSE_SINK"
            break
        fi
        sleep 0.1
    done
fi

[ $# -eq 0 ] && set -- vkcube

exec gamescope --backend headless \
    -W "$DISPLAY_WIDTH" -H "$DISPLAY_HEIGHT" -r "$DISPLAY_FPS" \
    -- "$@"
