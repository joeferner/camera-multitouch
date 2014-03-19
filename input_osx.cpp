
#include <stdlib.h>
#include <stdio.h>
#include "input.h"

typedef struct {
  int todo;
} OSXInputContext;

InputContext* inputBegin() {
  OSXInputContext* ctx = new OSXInputContext();

  return ctx;
}

void inputEnd(InputContext* ctx) {
}

void inputMouseMove(InputContext* ctx, int x, int y) {
}

