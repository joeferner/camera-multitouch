
#include <stdlib.h>
#include <stdio.h>
#include "input.h"
#include <CoreGraphics/CoreGraphics.h>
#include <ApplicationServices/ApplicationServices.h>

typedef struct {
  CGDirectDisplayID displayID;
} OSXInputContext;

InputContext* inputBegin() {
  OSXInputContext* ctx = new OSXInputContext();

  ctx->displayID = CGMainDisplayID();

  return ctx;
}

void inputEnd(InputContext* ctx) {
  OSXInputContext* osxctx = (OSXInputContext*) ctx;

  delete osxctx;
}

void inputMouseMove(InputContext* ctx, int x, int y) {
  CGEventRef mouseMove = CGEventCreateMouseEvent(
      NULL, kCGEventMouseMoved,
      CGPointMake(x, y),
      kCGMouseButtonLeft // ignored
  );

  CGEventPost(kCGHIDEventTap, mouseMove);
  CFRelease(mouseMove);
}

void inputMouseDown(InputContext* ctx, int x, int y) {
  CGEventRef mouseDown = CGEventCreateMouseEvent(
      NULL, kCGEventLeftMouseDown,
      CGPointMake(x, y),
      kCGMouseButtonLeft
  );

  CGEventPost(kCGHIDEventTap, mouseDown);
  CFRelease(mouseDown);
}

void inputMouseUp(InputContext* ctx, int x, int y) {
  CGEventRef mouseUp = CGEventCreateMouseEvent(
      NULL, kCGEventLeftMouseUp,
      CGPointMake(x, y),
      kCGMouseButtonLeft
  );

  CGEventPost(kCGHIDEventTap, mouseUp);
  CFRelease(mouseUp);
}
