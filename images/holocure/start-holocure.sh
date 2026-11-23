#!/bin/bash
set -euo pipefail

log() { echo "start-holocure: $*" >&2; }
fail() { log "$*"; exit 1; }

[[ $(id -u) == 1000 ]] || fail 'expected to run as UID 1000'
[[ -d $WINEPREFIX && -w $WINEPREFIX ]] || fail "$WINEPREFIX must be a writable directory"
[[ -r /opt/holocure/HoloCure.exe && -r /opt/holocure/data.win ]] ||
    fail 'mount the full extracted game directory at /opt/holocure'

# This build writes its settings and saves beside the executable.
game_dir=$WINEPREFIX/game
install -d -m 0700 "$game_dir"
cp -R /opt/holocure/. "$game_dir/"

wine_server=/usr/lib/x86_64-linux-gnu/wine/wineserver
server_pid=
streamer_pid=
stopping=0

signal_streamer() {
    if [[ -n $streamer_pid ]]; then
        kill -TERM "$streamer_pid" 2>/dev/null || true
    fi
}

request_stop() {
    stopping=1
    signal_streamer
}

cleanup() {
    trap '' INT TERM
    if [[ -n $streamer_pid ]]; then
        signal_streamer
        wait "$streamer_pid" || true
    fi
    if [[ -n $server_pid ]]; then
        kill -TERM "$server_pid" 2>/dev/null || true
        wait "$server_pid" || true
        log 'Wine server stopped'
    fi
}

trap cleanup EXIT
trap request_stop INT TERM

# Keep the server alive outside gamescope's reaper until Wine's clients exit.
"$wine_server" --foreground --persistent &
server_pid=$!
deadline=$((SECONDS + 3))
until timeout 0.3s "$wine_server" --kill=0; do
    if (( stopping )); then exit 0; fi
    kill -0 "$server_pid" 2>/dev/null || fail 'Wine server exited before becoming ready'
    if (( SECONDS >= deadline )); then fail 'timed out waiting for the Wine server'; fi
    sleep 0.1
done
if (( stopping )); then exit 0; fi
kill -0 "$server_pid" 2>/dev/null || fail 'Wine server exited before launching the game'
log 'Wine server ready'

start-streamer bash -ec '
    [[ -n ${DISPLAY:-} ]] || { echo "start-holocure: gamescope did not provide an XWayland display" >&2; exit 1; }
    cd "$1"
    wine --version
    glxinfo -B
    exec wine ./HoloCure.exe
' bash "$game_dir" &
streamer_pid=$!

status=0
if (( ! stopping )); then wait "$streamer_pid" || status=$?; fi
if (( stopping )); then exit 0; fi

streamer_pid=
exit "$status"
