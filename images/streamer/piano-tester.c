#include <SDL3/SDL.h>
#include <math.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { WIDTH = 1280, HEIGHT = 720, RATE = 48000, NOTES = 25, NO_NOTE = -1 };
enum { FIRST_MIDI_NOTE = 48, MIN_NOTE_FRAMES = RATE / 20 };

static const float full_turn = 6.28318530718f;
static const SDL_FRect keyboard_area = { 36, 193, 1208, 430 };

static const int white_notes[] = { 0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24 };
static const char *const pitches[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
static const char typing_keys[] = "ZSXDCVGBHNJMQ2W3ER5T6Y7UI";
static const SDL_Scancode typing_codes[NOTES] = {
    SDL_SCANCODE_Z, SDL_SCANCODE_S, SDL_SCANCODE_X, SDL_SCANCODE_D, SDL_SCANCODE_C,
    SDL_SCANCODE_V, SDL_SCANCODE_G, SDL_SCANCODE_B, SDL_SCANCODE_H, SDL_SCANCODE_N,
    SDL_SCANCODE_J, SDL_SCANCODE_M, SDL_SCANCODE_Q, SDL_SCANCODE_2, SDL_SCANCODE_W,
    SDL_SCANCODE_3, SDL_SCANCODE_E, SDL_SCANCODE_R, SDL_SCANCODE_5, SDL_SCANCODE_T,
    SDL_SCANCODE_6, SDL_SCANCODE_Y, SDL_SCANCODE_7, SDL_SCANCODE_U, SDL_SCANCODE_I,
};

struct Key {
    SDL_FRect area;
    bool sharp;
};

struct Finger {
    SDL_TouchID device;
    SDL_FingerID id;
    bool active;
    int note;
};

struct Piano {
    struct Key keys[NOTES];
    struct Finger fingers[32];
    uint32_t keyboard;
    uint32_t active;
    bool mouse_held;
    int mouse_note;
};

struct Voice {
    float step;
    float phase;
    float level;
    int minimum_frames;
};

struct Synth {
    _Atomic uint32_t held;
    _Atomic uint32_t attacks;
    struct Voice voices[NOTES];
};

static uint32_t note_bit(int note) {
    return note == NO_NOTE ? 0 : UINT32_C(1) << note;
}

static void build_keyboard(struct Piano *piano) {
    const size_t count = sizeof white_notes / sizeof *white_notes;
    const float width = keyboard_area.w / count;
    for (size_t i = 0; i < count; i++) {
        const int note = white_notes[i];
        const float x = keyboard_area.x + i * width;
        piano->keys[note] = (struct Key){{x, keyboard_area.y, width, keyboard_area.h}, false};
        if (i + 1 < count && white_notes[i + 1] == note + 2) {
            piano->keys[note + 1] = (struct Key){
                {x + width * .69f, keyboard_area.y, width * .62f, keyboard_area.h * .60f},
                true,
            };
        }
    }
}

static int note_at(const struct Piano *piano, float x, float y) {
    const SDL_FPoint point = {x, y};
    for (int note = 0; note < NOTES; note++) {
        if (piano->keys[note].sharp && SDL_PointInRectFloat(&point, &piano->keys[note].area)) {
            return note;
        }
    }
    for (int note = 0; note < NOTES; note++) {
        if (!piano->keys[note].sharp && SDL_PointInRectFloat(&point, &piano->keys[note].area)) {
            return note;
        }
    }
    return NO_NOTE;
}

static void finger_event(struct Piano *piano, const SDL_TouchFingerEvent *event) {
    struct Finger *vacant = NULL;
    for (size_t i = 0; i < sizeof piano->fingers / sizeof *piano->fingers; i++) {
        struct Finger *finger = &piano->fingers[i];
        if (!finger->active) {
            if (!vacant) vacant = finger;
        } else if (finger->id == event->fingerID && finger->device == event->touchID) {
            if (event->type == SDL_EVENT_FINGER_UP || event->type == SDL_EVENT_FINGER_CANCELED) {
                finger->active = false;
            } else {
                finger->note = note_at(piano, event->x, event->y);
            }
            return;
        }
    }
    if (vacant && event->type == SDL_EVENT_FINGER_DOWN) {
        *vacant = (struct Finger){
            event->touchID, event->fingerID, true, note_at(piano, event->x, event->y),
        };
    }
}

static void update_notes(struct Piano *piano, struct Synth *synth) {
    uint32_t held = piano->keyboard | note_bit(piano->mouse_note);
    for (size_t i = 0; i < sizeof piano->fingers / sizeof *piano->fingers; i++) {
        if (piano->fingers[i].active) held |= note_bit(piano->fingers[i].note);
    }
    atomic_store(&synth->held, held);
    atomic_fetch_or(&synth->attacks, held & ~piano->active);
    piano->active = held;
}

static void input_event(struct Piano *piano, const SDL_Event *event) {
    switch (event->type) {
        case SDL_EVENT_FINGER_DOWN:
        case SDL_EVENT_FINGER_MOTION:
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_CANCELED:
            finger_event(piano, &event->tfinger);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event->button.which != SDL_TOUCH_MOUSEID && event->button.button == SDL_BUTTON_LEFT) {
                piano->mouse_held = event->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                piano->mouse_note = piano->mouse_held ? note_at(piano, event->button.x, event->button.y) : NO_NOTE;
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
            if (event->motion.which != SDL_TOUCH_MOUSEID && piano->mouse_held) {
                piano->mouse_held = (event->motion.state & SDL_BUTTON_LMASK) != 0;
                piano->mouse_note = piano->mouse_held ? note_at(piano, event->motion.x, event->motion.y) : NO_NOTE;
            }
            break;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            if (event->key.repeat) break;
            for (int note = 0; note < NOTES; note++) {
                if (typing_codes[note] != event->key.scancode) continue;
                if (event->type == SDL_EVENT_KEY_DOWN) piano->keyboard |= note_bit(note);
                else piano->keyboard &= ~note_bit(note);
                break;
            }
            break;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            piano->keyboard = 0;
            piano->mouse_held = false;
            piano->mouse_note = NO_NOTE;
            memset(piano->fingers, 0, sizeof piano->fingers);
            break;
    }
}

static void init_synth(struct Synth *synth) {
    for (int note = 0; note < NOTES; note++) {
        float frequency = 440.0f * exp2f((FIRST_MIDI_NOTE + note - 69) / 12.0f);
        synth->voices[note].step = full_turn * frequency / RATE;
    }
}

static void render_audio(struct Synth *synth, float *samples, int frames) {
    const uint32_t attacks = atomic_exchange(&synth->attacks, 0);
    const uint32_t held = atomic_load(&synth->held);
    for (int note = 0; note < NOTES; note++) {
        if (attacks & note_bit(note)) synth->voices[note].minimum_frames = MIN_NOTE_FRAMES;
    }
    for (int frame = 0; frame < frames; frame++) {
        float mix = 0;
        for (int note = 0; note < NOTES; note++) {
            struct Voice *voice = &synth->voices[note];
            bool sounding = (held & note_bit(note)) || voice->minimum_frames > 0;
            if (voice->minimum_frames > 0) voice->minimum_frames--;
            if (sounding) voice->level = fminf(1.0f, voice->level + 1.0f / 256);
            else voice->level = fmaxf(0, voice->level - 1.0f / 2400);
            if (voice->level == 0) {
                voice->phase = 0;
                continue;
            }
            mix += voice->level * (sinf(voice->phase) + .23f * sinf(2 * voice->phase)
                + .08f * sinf(3 * voice->phase)) * .12f;
            voice->phase += voice->step;
            if (voice->phase >= full_turn) voice->phase -= full_turn;
        }
        const float sample = mix / (1.0f + fabsf(mix));
        samples[frame * 2] = samples[frame * 2 + 1] = sample;
    }
}

static void SDLCALL supply_audio(void *user, SDL_AudioStream *stream, int additional, int total) {
    (void)total;
    struct Synth *synth = user;
    float samples[512 * 2];
    const int frame_bytes = 2 * sizeof(float);
    while (additional > 0) {
        int frames = (additional + frame_bytes - 1) / frame_bytes;
        if (frames > 512) frames = 512;
        render_audio(synth, samples, frames);
        if (!SDL_PutAudioStreamData(stream, samples, frames * frame_bytes)) return;
        additional -= frames * frame_bytes;
    }
}

static void color(SDL_Renderer *renderer, uint32_t rgb) {
    SDL_SetRenderDrawColor(renderer, (rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255, 255);
}

static void draw_key(SDL_Renderer *renderer, const struct Piano *piano, int note) {
    const struct Key *key = &piano->keys[note];
    const bool down = (piano->active & note_bit(note)) != 0;
    SDL_FRect rect = key->area;
    rect.x += 1.5f;
    rect.w -= 3.0f;
    color(renderer, down ? 0x50ded1 : (key->sharp ? 0x1a2933 : 0xf3eee0));
    SDL_RenderFillRect(renderer, &rect);
    color(renderer, down || !key->sharp ? 0x172730 : 0xf3eee0);
    int midi = FIRST_MIDI_NOTE + note;
    char label[8];
    snprintf(label, sizeof label, "%s%d", pitches[midi % 12], midi / 12 - 1);
    const float center = rect.x + rect.w / 2;
    const float bottom = rect.y + rect.h;
    const float scale = key->sharp ? 1.5f : 2.0f;
    SDL_SetRenderScale(renderer, scale, scale);
    SDL_RenderDebugText(renderer, center / scale - strlen(label) * 4, (bottom - 50) / scale, label);
    char button[] = {typing_keys[note], '\0'};
    SDL_RenderDebugText(renderer, center / scale - 4, (bottom - 25) / scale, button);
    SDL_SetRenderScale(renderer, 1, 1);
}

static void draw(SDL_Renderer *renderer, const struct Piano *piano, bool audio_available, unsigned int frame) {
    color(renderer, 0x0d1c24);
    SDL_RenderClear(renderer);
    color(renderer, 0xf3eee0);
    SDL_SetRenderScale(renderer, 3, 3);
    SDL_RenderDebugText(renderer, 12, 13, "Piano tester");
    SDL_SetRenderScale(renderer, 2, 2);
    int fingers = 0;
    for (size_t i = 0; i < sizeof piano->fingers / sizeof *piano->fingers; i++) {
        fingers += piano->fingers[i].active;
    }
    char status[80];
    snprintf(status, sizeof status, "C3 to C5     Touch contacts %d", fingers);
    color(renderer, fingers ? 0x50ded1 : 0xadc0c7);
    SDL_RenderDebugText(renderer, 18, 42, status);
    color(renderer, audio_available ? 0xadc0c7 : 0xffbd80);
    SDL_RenderDebugText(renderer, 18, 68, audio_available
        ? "Tap, click, or use the letter and number keys shown below."
        : "Audio output unavailable. The keys still show input.");
    SDL_SetRenderScale(renderer, 1, 1);
    // A repaint keeps the first video frame available when a viewer reconnects.
    color(renderer, 0x50ded1);
    SDL_FRect activity = {1186 + ((frame / 6) % 7) * 7, 46, 5, 5};
    SDL_RenderFillRect(renderer, &activity);
    for (int note = 0; note < NOTES; note++) if (!piano->keys[note].sharp) draw_key(renderer, piano, note);
    for (int note = 0; note < NOTES; note++) if (piano->keys[note].sharp) draw_key(renderer, piano, note);
    SDL_RenderPresent(renderer);
}

int main(void) {
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    if (SDL_getenv("PULSE_SERVER")) SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "pulseaudio");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Could not open piano display: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Piano tester", WIDTH, HEIGHT, SDL_WINDOW_BORDERLESS);
    SDL_Renderer *renderer = window ? SDL_CreateRenderer(window, "software") : NULL;
    if (!renderer || !SDL_SetRenderLogicalPresentation(renderer, WIDTH, HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX)) {
        SDL_Log("Could not create piano window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    struct Piano piano = {.mouse_note = NO_NOTE};
    struct Synth synth = {0};
    build_keyboard(&piano);
    init_synth(&synth);

    const SDL_AudioSpec spec = {.format = SDL_AUDIO_F32, .channels = 2, .freq = RATE};
    SDL_AudioStream *audio = NULL;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, supply_audio, &synth);
        if (audio && !SDL_ResumeAudioStreamDevice(audio)) {
            SDL_DestroyAudioStream(audio);
            audio = NULL;
        }
    }
    if (!audio) SDL_Log("Piano audio unavailable: %s", SDL_GetError());

    unsigned int frame = 0;
    bool running = true;
    while (running) {
        bool dirty = frame % 6 == 0;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                running = false;
                break;
            }
            if (!SDL_ConvertEventToRenderCoordinates(renderer, &event)) continue;
            input_event(&piano, &event);
            update_notes(&piano, &synth);
            dirty = true;
        }
        if (dirty && running) draw(renderer, &piano, audio != NULL, frame);
        frame++;
        SDL_Delay(16);
    }
    SDL_DestroyAudioStream(audio);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
