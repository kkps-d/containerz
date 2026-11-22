#!/bin/bash
set -euo pipefail

gst-launch-1.0 -q audiotestsrc is-live=true wave=ticks freq=440 volume=0.12 \
    sine-periods-per-tick=132 tick-interval=1500000000 ! audio/x-raw,rate=48000,channels=2 \
    ! pulsesink &
tone_pid=$!
input-visualizer &
visualizer_pid=$!

cleanup() {
    kill -TERM "$tone_pid" "$visualizer_pid" 2>/dev/null || true
    wait "$tone_pid" "$visualizer_pid" || true
}
trap cleanup EXIT
trap 'exit 0' INT TERM
wait -n "$tone_pid" "$visualizer_pid"
