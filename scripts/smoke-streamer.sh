#!/bin/bash
set -euo pipefail

fail() { echo "smoke-streamer: $*" >&2; exit 1; }

command=()
case ${1:-} in
    --input-test)
        command=(start-streamer stdbuf -oL xev -geometry 1152x648+0+0 -event keyboard -event mouse)
        shift
        ;;
    --visual-input-test)
        command=(start-streamer input-visualizer)
        shift
        ;;
    --audio-test)
        command=(start-streamer audio-test)
        shift
        ;;
    --piano-test)
        command=(start-streamer piano-tester)
        shift
        ;;
esac

if (( $# < 2 || $# > 3 )); then
    fail 'usage: bash scripts/smoke-streamer.sh [--audio-test|--input-test|--piano-test|--visual-input-test] nvidia|amd PCI_ADDRESS [WEB_REMOTE_DESKTOP_DIR]'
fi

gpu=$1
pci=$2
repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
client_dir=${3:-"$repo_dir/../web-remote-desktop"}/client
[[ -x $client_dir/node_modules/.bin/vite ]] || fail "run npm ci in $client_dir first"

drm=/dev/dri/by-path/pci-$pci
card=$(readlink -e -- "$drm-card") || fail "no DRM card for $pci"
render=$(readlink -e -- "$drm-render") || fail "no DRM render node for $pci"
vendor=$(cat -- "/sys/bus/pci/devices/$pci/vendor") || fail "could not read GPU vendor for $pci"
case $gpu:$vendor in
    nvidia:0x10de)
        uuid=$(nvidia-smi --id="$pci" --query-gpu=uuid --format=csv,noheader) \
            || fail "could not find the NVIDIA GPU at $pci"
        [[ $uuid == GPU-* ]] || fail "no NVIDIA GPU UUID for $pci"
        devices=(--device "nvidia.com/gpu=$uuid")
        ;;
    amd:0x1002) devices=(--device "$card" --device "$render") ;;
    *) fail "GPU at $pci does not match '$gpu' (use nvidia or amd)" ;;
esac

listeners=$(ss -H -ltn '( sport = :8080 or sport = :5173 )')
[[ -z $listeners ]] || fail 'ports 8080 and 5173 must be free'

container=
client_pid=
logs_pid=
cleanup() {
    trap '' INT TERM
    if [[ -n $client_pid ]]; then
        kill -TERM -- "-$client_pid" 2>/dev/null || true
        wait "$client_pid" 2>/dev/null || true
    fi
    if [[ -n $container ]]; then
        docker stop "$container" >/dev/null || true
        docker inspect --format 'Streamer container exit status: {{.State.ExitCode}}' "$container" || true
        if [[ -n $logs_pid ]]; then wait "$logs_pid" || true; fi
        docker rm "$container" >/dev/null || true
    fi
}
trap cleanup EXIT
trap 'exit 0' INT TERM

container=$(docker run -d --pull never --network host --stop-timeout 15 --ulimit core=0 \
    "${devices[@]}" --group-add "$(stat -c %g "$card")" --group-add "$(stat -c %g "$render")" \
    containerz/streamer:dev "${command[@]}")
docker logs --follow "$container" &
logs_pid=$!
setsid npm --prefix "$client_dir" run dev -- --port 5173 --strictPort &
client_pid=$!

services_running() {
    kill -0 "$client_pid" 2>/dev/null &&
        [[ $(docker inspect --format '{{.State.Running}}' "$container") == true ]]
}

deadline=$((SECONDS + 20))
while services_running; do
    offer_status=$(curl -s --max-time 1 -o /dev/null -w '%{http_code}' http://127.0.0.1:8080/offer) || true
    if [[ $offer_status == 405 ]] && curl -fs --max-time 1 -o /dev/null http://127.0.0.1:5173/; then
        printf '\nBrowser: http://localhost:5173\nAgent address: localhost:8080\n\n'
        while services_running; do sleep 0.3; done
        break
    fi
    if (( SECONDS >= deadline )); then fail 'timed out waiting for the browser client and agent'; fi
    sleep 0.2
done
fail 'the browser client or agent stopped'
