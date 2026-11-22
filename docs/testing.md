# Testing

Build and launch the browser smoke tests using the [streamer README](../images/streamer/README.md#test-in-a-browser).

## Verified on native Linux

| Check | RTX 5080 | AMD Raphael iGPU |
| --- | --- | --- |
| Gamescope PipeWire capture | 16 distinct 1280×720 frames | 16 distinct 1280×720 frames |
| Browser video | Playback, reconnect, and resolution changes passed | Playback, reconnect, and resolution changes passed |
| Browser input | Keyboard, clicks, scroll, and both mouse modes passed | No result recorded |
| Game audio | Tone decoded after reconnect and resolution changes | No result recorded |

On the RTX 5080, absolute mouse input stayed aligned after resizing. The agent
released held input on channel close, viewer replacement, and shutdown. A missing
EIS socket left video running. A missing or removed audio sink left video and
input running.

The browser smoke test also passed from a second device using the Linux host's
LAN address.

## When adding a game

1. Launch the game on each GPU and check live video, game audio, and the keyboard
   and mouse controls the game uses.
2. Reconnect and change the stream resolution during gameplay. Check that video,
   audio, and input still work.
3. Connect from a second LAN device using the host address.
4. Stop the launcher and confirm that the container and client exit. Record any
   gamescope shutdown status separately from the playback result.
