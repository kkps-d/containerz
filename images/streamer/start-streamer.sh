#!/bin/bash
set -euo pipefail

display_pid=
agent_pid=

log() { echo "start-streamer: $*" >&2; }

stop_child() {
    local name=$1 pid=$2 status=0
    if [[ -n $pid ]]; then
        kill -TERM "$pid" 2>/dev/null || true
        wait "$pid" || status=$?
        log "$name exited with status $status"
    fi
}

cleanup() {
    trap '' INT TERM
    stop_child agent "$agent_pid"
    stop_child gamescope "$display_pid"
}
trap cleanup EXIT
trap 'exit 0' INT TERM

cursor_socket=containerz-cursor.sock
case ${CONTAINERZ_STREAM_CURSOR:-auto} in
    auto) export GAMESCOPE_PIPEWIRE_CURSOR_SOCKET="$XDG_RUNTIME_DIR/$cursor_socket" ;;
    game-drawn) unset GAMESCOPE_PIPEWIRE_CURSOR_SOCKET ;;
    *) log 'CONTAINERZ_STREAM_CURSOR must be auto or game-drawn'; exit 1 ;;
esac

agent_config=$XDG_RUNTIME_DIR/containerz-agent.toml
if [[ -n ${CONTAINERZ_AGENT_PORT:-} ]]; then
    for value in "$CONTAINERZ_AGENT_PORT" "$DISPLAY_WIDTH" "$DISPLAY_HEIGHT" "$DISPLAY_FPS"; do
        if [[ ! $value =~ ^[1-9][0-9]*$ ]]; then
            log 'Agent port, display dimensions, and frame rate must be positive integers'
            exit 1
        fi
    done
    agent_bind=${CONTAINERZ_AGENT_BIND_ADDRESS:-127.0.0.1}
    if [[ ! $agent_bind =~ ^([0-9]{1,3}\.){3}[0-9]{1,3}$ ]]; then
        log 'CONTAINERZ_AGENT_BIND_ADDRESS must be an IPv4 address'
        exit 1
    fi
    sed -e "s/^port = .*/port = $CONTAINERZ_AGENT_PORT\nbind_address = \"$agent_bind\"/" \
        -e "s/^width_px = .*/width_px = $DISPLAY_WIDTH/" \
        -e "s/^height_px = .*/height_px = $DISPLAY_HEIGHT/" \
        /etc/containerz/agent.toml > "$agent_config"
else
    # The standalone image uses the native source dimensions in agent.toml.
    export DISPLAY_WIDTH=1280 DISPLAY_HEIGHT=720
    cp /etc/containerz/agent.toml "$agent_config"
fi
if [[ -n ${GAMESCOPE_PIPEWIRE_CURSOR_SOCKET:-} ]]; then
    # A restarted container can retain the socket from an unclean shutdown.
    if [[ -S $GAMESCOPE_PIPEWIRE_CURSOR_SOCKET ]]; then
        rm -f -- "$GAMESCOPE_PIPEWIRE_CURSOR_SOCKET" || log 'could not remove the stale cursor socket'
    fi
    printf '\npipewire_cursor_socket = "%s"\n' "$cursor_socket" >> "$agent_config"
fi
if [[ -n ${CONTAINERZ_AGENT_PORT:-} ]]; then
    printf '\n[stream]\ndefault_fps = %s\nmax_fps = %s\n' "$DISPLAY_FPS" "$DISPLAY_FPS" >> "$agent_config"
fi
start-display "$@" &
display_pid=$!

has_video_node() {
    timeout 0.3s pw-cli ls Node 2>/dev/null | awk '
        /^[[:space:]]*id / { named = video = 0 }
        /node\.name = "gamescope"$/ { named = 1 }
        /media\.class = "Video\/Source"$/ { video = 1 }
        named && video { found = 1 }
        END { exit !found }
    '
}

deadline=$((SECONDS + 10))
until has_video_node; do
    if ! kill -0 "$display_pid" 2>/dev/null; then
        log 'gamescope stopped before publishing its video node'
        exit 1
    fi
    if (( SECONDS >= deadline )); then
        log 'timed out waiting for the gamescope PipeWire video node'
        exit 1
    fi
    sleep 0.1
done

agent --config "$agent_config" &
agent_pid=$!

while kill -0 "$display_pid" 2>/dev/null && kill -0 "$agent_pid" 2>/dev/null; do
    sleep 0.2
done
log 'a required process stopped'
exit 1
