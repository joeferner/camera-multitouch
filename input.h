
#ifndef INPUT_H
#define	INPUT_H

#ifdef	__cplusplus
extern "C" {
#endif

#define MOUSE_LBUTTON_UP   1
#define MOUSE_LBUTTON_DOWN 2

  typedef void InputContext;

  InputContext* inputBegin();
  void inputEnd(InputContext* ctx);
  void inputMouseMove(InputContext* ctx, int mouseLButtonState, int x, int y);
  void inputMouseDown(InputContext* ctx, int x, int y);
  void inputMouseUp(InputContext* ctx, int x, int y);


#ifdef	__cplusplus
}
#endif

#endif	/* INPUT_H */

