/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         Touch.h                         **/
/**                                                         **/
/** This file declares functions that simulate joystick and **/
/** dialpad with the touch screen.                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2008-2010                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef TOUCH_H
#define TOUCH_H

#include "EMULib.h"

#ifdef __cplusplus
extern "C" {
#endif

/** GenericPenJoystick() *************************************/
/** Get simulated joystick buttons from touch screen UI.    **/
/** Result compatible with GetJoystick().                   **/
/*************************************************************/
unsigned int GenericPenJoystick(int X,int Y,int W,int H);

/** GenericPenDialpad() **************************************/
/** Get simulated dialpad buttons from touch screen UI.     **/
/*************************************************************/
unsigned char GenericPenDialpad(int X,int Y,int W,int H);

/** GenericPenKeyboard() *************************************/
/** Get virtual on-screen keyboard buttons.                 **/
/*************************************************************/
unsigned char GenericPenKeyboard(int X,int Y,int W,int H);

/** GenericDialKeyboard() ************************************/
/** Process dialpad input to the virtual keyboard. Returns  **/
/** virtual keyboard key if selected, or 0 if not.          **/
/*************************************************************/
unsigned char GenericDialKeyboard(unsigned char Key);

/** SetPenCues() *********************************************/
/** Set pen cues for given buttons to a given string.       **/
/*************************************************************/
void SetPenCues(unsigned int Buttons,const char *CueText);

/** DrawPenCues() ********************************************/
/** Overlay dotted cue lines for using PenJoystick() onto a **/
/** given image. Show dialpad cues if requested.            **/
/*************************************************************/
void DrawPenCues(Image *Img,int ShowDialpad,pixel Color);

/** DrawKeyboard() *******************************************/
/** Draw virtual keyboard in a given image. Key modifiers   **/
/** and the key code passed in CurKey are highlighted.      **/
/*************************************************************/
void DrawKeyboard(Image *Img,unsigned int CurKey);

#ifdef __cplusplus
}
#endif
#endif /* TOUCH_H */
