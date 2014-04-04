
#include <stdlib.h>
#include <stdio.h>
#include "input.h"
#include <CoreGraphics/CoreGraphics.h>

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
  CGPoint point = (CGPoint) {x, y};
  CGWarpMouseCursorPosition(point);
}

