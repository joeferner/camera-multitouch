
#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "input.h"

typedef struct {
  Display *display;
  Window rootWindow;
} X11InputContext;

InputContext* inputBegin() {
  X11InputContext* ctx = new X11InputContext();
  ctx->display = XOpenDisplay(0);
  if (!ctx->display) {
    return NULL;
  }

  int screen = DefaultScreen(ctx->display);
  ctx->rootWindow = RootWindow(ctx->display, screen);
  if (!ctx->rootWindow) {
    return NULL;
  }

  return ctx;
}

void inputEnd(InputContext* ctx) {
  X11InputContext* x11ctx = (X11InputContext*) ctx;

  XCloseDisplay(x11ctx->display);

  delete x11ctx;
}

void inputMouseMove(InputContext* ctx, int x, int y) {
  X11InputContext* x11ctx = (X11InputContext*) ctx;
  int r = XWarpPointer(x11ctx->display, None, x11ctx->rootWindow, 0, 0, 0, 0, x, y);
  XFlush(x11ctx->display);
}

