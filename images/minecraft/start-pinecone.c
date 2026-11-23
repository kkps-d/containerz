/* Request a frame from idle Pinecone windows so new viewers can start the stream. */
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

static int ignore_x_error(Display *display, XErrorEvent *event)
{
    /* A window can close between checking its focus and requesting a repaint. */
    (void)display;
    (void)event;
    return 0;
}

static void repaint_pinecone(Display *display)
{
    Window window;
    int revert_to;
    XGetInputFocus(display, &window, &revert_to);

    XClassHint hint;
    if (window == None || window == PointerRoot || !XGetClassHint(display, window, &hint))
        return;

    bool is_pinecone = hint.res_class && strcmp(hint.res_class, "ElyPrismLauncher") == 0;
    XFree(hint.res_name);
    XFree(hint.res_class);
    if (!is_pinecone)
        return;

    XWindowAttributes attrs;
    if (!XGetWindowAttributes(display, window, &attrs) || attrs.map_state != IsViewable)
        return;

    XEvent expose = { .xexpose = {
        .type = Expose,
        .display = display,
        .window = window,
        .width = attrs.width,
        .height = attrs.height,
    }};
    XSendEvent(display, window, False, ExposureMask, &expose);
    XFlush(display);
}

int main(void)
{
    pid_t launcher = getpid();
    pid_t repaint = fork();
    if (repaint == -1) {
        perror("start-pinecone: fork");
        return EXIT_FAILURE;
    }

    if (repaint == 0) {
        if (prctl(PR_SET_PDEATHSIG, SIGTERM) == -1 || getppid() != launcher)
            return EXIT_FAILURE;

        Display *display = XOpenDisplay(NULL);
        if (!display) {
            fputs("start-pinecone: could not open XWayland for launcher repaints\n", stderr);
            return EXIT_FAILURE;
        }
        XSetErrorHandler(ignore_x_error);

        for (;;) {
            repaint_pinecone(display);
            sleep(2);
        }
    }

    const char *app_run = "/opt/pinecone/AppRun";
    execl(app_run, app_run, "--dir", "/home/player/.local/share/PineconeMC", (char *)NULL);
    perror("start-pinecone: AppRun");
    return EXIT_FAILURE;
}
