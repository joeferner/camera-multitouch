
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

  printf("inputBegin complete\n");
  return ctx;
}

void inputEnd(InputContext* ctx) {
  OSXInputContext* osxctx = (OSXInputContext*) ctx;

  delete osxctx;
  printf("inputEnd complete\n");
}

void inputMouseMove(InputContext* ctx, int mouseLButtonState, int x, int y) {
  CGEventRef mouseMove = CGEventCreateMouseEvent(
      NULL, mouseLButtonState == MOUSE_LBUTTON_DOWN ? kCGEventLeftMouseDragged : kCGEventMouseMoved,
      CGPointMake(x, y),
      kCGMouseButtonLeft // ignored
  );

  CGEventPost(kCGHIDEventTap, mouseMove);
  CFRelease(mouseMove);
}

void inputMouseDown(InputContext* ctx, int x, int y) {
  CGDisplayHideCursor(kCGDirectMainDisplay);
  CGEventRef mouseDown = CGEventCreateMouseEvent(
      NULL, kCGEventLeftMouseDown,
      CGPointMake(x, y),
      kCGMouseButtonLeft
  );

  CGEventPost(kCGHIDEventTap, mouseDown);
  CFRelease(mouseDown);
}

void inputMouseUp(InputContext* ctx, int x, int y) {
  CGDisplayShowCursor(kCGDirectMainDisplay);
  CGEventRef mouseUp = CGEventCreateMouseEvent(
      NULL, kCGEventLeftMouseUp,
      CGPointMake(x, y),
      kCGMouseButtonLeft
  );

  CGEventPost(kCGHIDEventTap, mouseUp);
  CFRelease(mouseUp);
}
