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

# These dimensions match the native source declared in agent.toml.
DISPLAY_WIDTH=1280 DISPLAY_HEIGHT=720 start-display "$@" &
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

agent --config /etc/containerz/agent.toml &
agent_pid=$!

while kill -0 "$display_pid" 2>/dev/null && kill -0 "$agent_pid" 2>/dev/null; do
    sleep 0.2
done
log 'a required process stopped'
exit 1
