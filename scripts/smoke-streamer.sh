#!/bin/bash
set -euo pipefail

fail() { echo "smoke-streamer: $*" >&2; exit 1; }

image=containerz/streamer:dev
command=()
mounts=()
env_args=()
reuse_client=0
case ${1:-} in
    --minecraft)
        image=containerz/minecraft:dev
        mounts=(--mount type=volume,src=containerz-pinecone-data,dst=/home/player/.local/share/PineconeMC)
        env_args=(--env MINECRAFT_OFFLINE_NAME)
        shift
        ;;
    --openarena)
        image=containerz/openarena:dev
        shift
        ;;
    --holocure)
        image=containerz/holocure:dev
        reuse_client=${HOLOCURE_REUSE_CLIENT:-0}
        shift
        ;;
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
    fail 'usage: bash scripts/smoke-streamer.sh [--minecraft|--openarena|--holocure|--audio-test|--input-test|--piano-test|--visual-input-test] nvidia|amd PCI_ADDRESS [WEB_REMOTE_DESKTOP_DIR]'
fi

gpu=$1
pci=$2
repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
client_dir=${3:-"$repo_dir/../web-remote-desktop"}/client

if [[ $image == containerz/holocure:dev ]]; then
    [[ -n ${HOLOCURE_GAME_DIR:-} ]] || fail 'set HOLOCURE_GAME_DIR to the extracted HoloCure directory'
    game_dir=$(realpath -e -- "$HOLOCURE_GAME_DIR") || fail "could not find $HOLOCURE_GAME_DIR"
    [[ -r $game_dir/HoloCure.exe && -r $game_dir/data.win ]] || fail 'extract the full HoloCure archive first'
    mounts=(--mount "type=bind,src=$game_dir,dst=/opt/holocure,readonly"
            --mount "type=volume,src=containerz-holocure-smoke-$gpu,dst=/home/player/.wine-holocure")
fi

if [[ $reuse_client == 1 ]]; then
    client_html=$(curl -fsS --max-time 2 http://127.0.0.1:5173/) ||
        fail 'the existing Remote Desktop viewer on port 5173 is unavailable'
    [[ $client_html == *'<title>Remote Desktop</title>'* ]] || fail 'port 5173 is not serving the Remote Desktop viewer'
else
    [[ -x $client_dir/node_modules/.bin/vite ]] || fail "run npm ci in $client_dir first"
fi

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

if [[ $reuse_client == 1 ]]; then
    [[ -z $(ss -H -ltn 'sport = :8080') ]] || fail 'port 8080 must be free'
else
    [[ -z $(ss -H -ltn '( sport = :8080 or sport = :5173 )') ]] || fail 'ports 8080 and 5173 must be free'
fi

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
    "${env_args[@]}" "${mounts[@]}" "$image" "${command[@]}")
docker logs --follow "$container" &
logs_pid=$!
if [[ $reuse_client != 1 ]]; then
    setsid npm --prefix "$client_dir" run dev -- --port 5173 --strictPort &
    client_pid=$!
fi

services_running() {
    [[ -z $client_pid ]] || kill -0 "$client_pid" 2>/dev/null || return 1
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
