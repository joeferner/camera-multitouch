
#ifndef INPUT_H
#define	INPUT_H

#ifdef	__cplusplus
extern "C" {
#endif

  typedef void InputContext;

  InputContext* inputBegin();
  void inputEnd(InputContext* ctx);
  void inputMouseMove(InputContext* ctx, int x, int y);


#ifdef	__cplusplus
}
#endif

#endif	/* INPUT_H */

