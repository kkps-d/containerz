#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

struct KeyCap {
    const char *label;
    KeySym symbol;
    int units;
    KeyCode code;
};

#define KEY(label, symbol) {label, XK_##symbol, 4, 0}
#define WIDE(label, symbol, units) {label, XK_##symbol, units, 0}
static struct KeyCap function_row[] = {
    KEY("Esc", Escape), KEY("F1", F1), KEY("F2", F2), KEY("F3", F3),
    KEY("F4", F4), KEY("F5", F5), KEY("F6", F6), KEY("F7", F7), KEY("F8", F8),
    KEY("F9", F9), KEY("F10", F10), KEY("F11", F11), KEY("F12", F12),
    WIDE("Delete", Delete, 8),
};
static struct KeyCap number_row[] = {
    KEY("`", grave), KEY("1", 1), KEY("2", 2), KEY("3", 3), KEY("4", 4),
    KEY("5", 5), KEY("6", 6), KEY("7", 7), KEY("8", 8), KEY("9", 9),
    KEY("0", 0), KEY("-", minus), KEY("=", equal), WIDE("Backspace", BackSpace, 8),
};
static struct KeyCap upper_row[] = {
    WIDE("Tab", Tab, 6), KEY("Q", q), KEY("W", w), KEY("E", e), KEY("R", r),
    KEY("T", t), KEY("Y", y), KEY("U", u), KEY("I", i), KEY("O", o), KEY("P", p),
    KEY("[", bracketleft), KEY("]", bracketright), WIDE("\\", backslash, 6),
};
static struct KeyCap home_row[] = {
    WIDE("Caps Lock", Caps_Lock, 7), KEY("A", a), KEY("S", s), KEY("D", d),
    KEY("F", f), KEY("G", g), KEY("H", h), KEY("J", j), KEY("K", k), KEY("L", l),
    KEY(";", semicolon), KEY("'", apostrophe), WIDE("Enter", Return, 9),
};
static struct KeyCap lower_row[] = {
    WIDE("Shift L", Shift_L, 9), KEY("Z", z), KEY("X", x), KEY("C", c), KEY("V", v),
    KEY("B", b), KEY("N", n), KEY("M", m), KEY(",", comma), KEY(".", period),
    KEY("/", slash), WIDE("Shift R", Shift_R, 7), KEY("Up", Up),
};
static struct KeyCap modifier_row[] = {
    WIDE("Ctrl L", Control_L, 5), WIDE("Super", Super_L, 5), WIDE("Alt L", Alt_L, 5),
    WIDE("Space", space, 20), WIDE("Alt R", Alt_R, 5), WIDE("Ctrl R", Control_R, 5),
    WIDE("Left", Left, 5), WIDE("Down", Down, 5), WIDE("Right", Right, 5),
};
#undef KEY
#undef WIDE

static const struct {
    struct KeyCap *caps;
    size_t size;
} keyboard_rows[] = {
#define ROW(caps) { caps, sizeof caps / sizeof *caps }
    ROW(function_row), ROW(number_row), ROW(upper_row),
    ROW(home_row), ROW(lower_row), ROW(modifier_row),
#undef ROW
};
static const size_t row_count = sizeof keyboard_rows / sizeof *keyboard_rows;

static void load_keycodes(Display *display) {
    for (size_t row = 0; row < row_count; row++) {
        for (size_t index = 0; index < keyboard_rows[row].size; index++) {
            struct KeyCap *key = &keyboard_rows[row].caps[index];
            key->code = XKeysymToKeycode(display, key->symbol);
        }
    }
}

static void draw_keyboard(Display *display, Drawable frame, GC gc, XFontStruct *font,
    int width, int height, const bool pressed[256]) {
    const int pitch = 43;
    const int top = height - 30 - (int)row_count * pitch;
    const int usable = width - 72;
    for (size_t row = 0; row < row_count; row++) {
        int offset = 0;
        for (size_t index = 0; index < keyboard_rows[row].size; index++) {
            const struct KeyCap *key = &keyboard_rows[row].caps[index];
            const bool held = key->code && pressed[key->code];
            const int x = 36 + offset * usable / 60;
            const int y = top + (int)row * pitch;
            const int key_width = (offset + key->units) * usable / 60 - offset * usable / 60 - 5;
            const int label_length = strlen(key->label);
            const int label_width = font ? XTextWidth(font, key->label, label_length) : label_length * 6;
            XSetForeground(display, gc, held ? 0x70d9ee : 0x2a394c);
            XFillRectangle(display, frame, gc, x, y, key_width, pitch - 5);
            XSetForeground(display, gc, held ? 0x111821 : 0xeaf4fc);
            XDrawString(display, frame, gc, x + (key_width - label_width) / 2, y + 24,
                key->label, label_length);
            offset += key->units;
        }
    }
}

static void describe_held_keys(Display *display, const bool pressed[256], char *text, size_t size) {
    const char prefix[] = "Keys held: ";
    const size_t prefix_length = sizeof prefix - 1;
    size_t used = prefix_length;
    snprintf(text, size, "%s", prefix);
    for (unsigned int code = 0; code < 256 && used + 20 < size; code++) {
        if (!pressed[code]) continue;
        const char *name = XKeysymToString(XkbKeycodeToKeysym(display, code, 0, 0));
        int written = snprintf(text + used, size - used, "%s%s",
            used == prefix_length ? "" : "  +  ", name ? name : "Unknown");
        if (written < 0 || (size_t)written >= size - used) return;
        used += written;
    }
    if (used == prefix_length) snprintf(text + used, size - used, "None");
}

int main(void) {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fputs("Could not open gamescope's XWayland display\n", stderr);
        return 1;
    }

    const int screen = DefaultScreen(display);
    const int width = DisplayWidth(display, screen);
    const int height = DisplayHeight(display, screen);
    Window window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0,
        width, height, 0, 0x1c2635, 0x111821);
    XStoreName(display, window, "Gamescope Input Test");
    XSelectInput(display, window, ExposureMask | KeyPressMask | KeyReleaseMask |
        PointerMotionMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | FocusChangeMask);
    XMapRaised(display, window);

    GC gc = XCreateGC(display, window, 0, NULL);
    // CopyArea completion events can keep the event loop awake continuously.
    XSetGraphicsExposures(display, gc, False);
    Pixmap frame = XCreatePixmap(display, window, width, height, DefaultDepth(display, screen));
    XFontStruct *font = XLoadQueryFont(display, "9x15");
    if (font) XSetFont(display, gc, font->fid);

    XkbSetDetectableAutoRepeat(display, True, NULL);
    load_keycodes(display);
    bool pressed[256] = {false};
    int pointer_x = -1, pointer_y = -1;
    int clicks = 0, wheel_x = 0, wheel_y = 0;
    unsigned int frames = 0;
    char last_key[128] = "Key: click the window, then type";
    char last_button[96] = "Mouse: move, click or scroll";

    const struct timespec frame_interval = { .tv_nsec = 33000000 };
    for (;;) {
        int dirty = frames % 6 == 0;
        for (unsigned int events = 0; events < 128 && XPending(display); events++) {
            XEvent event;
            XNextEvent(display, &event);
            switch (event.type) {
                case DestroyNotify:
                    return 0;
                case Expose:
                    dirty = 1;
                    break;
                case MotionNotify:
                    dirty = 1;
                    pointer_x = event.xmotion.x_root;
                    pointer_y = event.xmotion.y_root;
                    break;
                case FocusOut:
                    memset(pressed, 0, sizeof pressed);
                    dirty = 1;
                    break;
                case KeyPress:
                case KeyRelease: {
                    dirty = 1;
                    if (event.xkey.keycode < sizeof pressed / sizeof *pressed) {
                        pressed[event.xkey.keycode] = event.type == KeyPress;
                    }
                    KeySym symbol = XLookupKeysym(&event.xkey, 0);
                    const char *name = XKeysymToString(symbol);
                    snprintf(last_key, sizeof last_key, "Key: %s %s", name ? name : "(unknown)",
                        event.type == KeyPress ? "pressed" : "released");
                    break;
                }
                case ButtonPress:
                case ButtonRelease: {
                    unsigned int button = event.xbutton.button;
                    dirty = 1;
                    pointer_x = event.xbutton.x_root;
                    pointer_y = event.xbutton.y_root;
                    if (button >= 4 && button <= 7) {
                        if (event.type == ButtonPress) {
                            wheel_y += (button == 5) - (button == 4);
                            wheel_x += (button == 7) - (button == 6);
                            snprintf(last_button, sizeof last_button, "Wheel: horizontal %d, vertical %d", wheel_x, wheel_y);
                        }
                    } else {
                        if (event.type == ButtonPress) clicks++;
                        snprintf(last_button, sizeof last_button, "Mouse: button %u %s (%d clicks)",
                            button, event.type == ButtonPress ? "pressed" : "released", clicks);
                    }
                    break;
                }
            }
        }

        frames++;
        if (!dirty) {
            nanosleep(&frame_interval, NULL);
            continue;
        }
        XSetForeground(display, gc, 0x111821);
        XFillRectangle(display, frame, gc, 0, 0, width, height);
        XSetForeground(display, gc, 0x1c2635);
        XFillRectangle(display, frame, gc, 18, 134, width - 36, height - 154);
        XSetForeground(display, gc, 0x111821);
        XFillRectangle(display, frame, gc, 0, 0, width, 133);
        XSetForeground(display, gc, 0xeaf4fc);
        char pointer[96] = "Pointer: move into the window";
        if (pointer_x >= 0) {
            snprintf(pointer, sizeof pointer, "Pointer: %d, %d / %d x %d",
                pointer_x, pointer_y, width, height);
        }
        char held[128];
        describe_held_keys(display, pressed, held, sizeof held);
        const char *lines[] = { "Gamescope input test", last_key, held, last_button, pointer };
        for (size_t i = 0; i < sizeof lines / sizeof *lines; i++) {
            XDrawString(display, frame, gc, 20, 26 + 22 * i, lines[i], strlen(lines[i]));
        }
        XSetForeground(display, gc, 0x70d9ee);
        XFillRectangle(display, frame, gc, width - 165 + (frames / 6 % 12) * 10, 22, 6, 6);
        draw_keyboard(display, frame, gc, font, width, height, pressed);
        if (pointer_x >= 0) {
            XSetForeground(display, gc, 0x70d9ee);
            XDrawArc(display, frame, gc, pointer_x - 11, pointer_y - 11, 22, 22, 0, 360 * 64);
            XDrawLine(display, frame, gc, pointer_x - 16, pointer_y, pointer_x + 16, pointer_y);
            XDrawLine(display, frame, gc, pointer_x, pointer_y - 16, pointer_x, pointer_y + 16);
        }
        XCopyArea(display, frame, window, gc, 0, 0, width, height, 0, 0);
        XFlush(display);

        nanosleep(&frame_interval, NULL);
    }
}
