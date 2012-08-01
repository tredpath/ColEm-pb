/** ColEm: portable Coleco emulator **************************/
/**                                                         **/
/**                         Unix.c                          **/
/**                                                         **/
/** This file contains Unix-dependent subroutines and       **/
/** drivers. It includes screen drivers via Display.h.      **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2010                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "../Coleco.h"
#include "../../EMULib/Console.h"
#include "../../EMULib/EMULib.h"
#include "../../EMULib/NetPlay.h"
#include "../../EMULib/Sound.h"
#include "QNXMenu.h"

#include <sys/keycodes.h>
#include <SDL.h>

#include <string.h>
#include <stdio.h>

/* Combination of EFF_* bits */
int UseEffects  = EFF_SCALE|EFF_SAVECPU|EFF_VARBPP|EFF_SYNC;

extern int MasterVolume; /* Master volume                    */

int InMenu;                /* 1: In MenuColeco(),ignore keys */
int UseZoom     = 1;       /* Zoom factor (1=no zoom)        */
int UseSound    = 22050;   /* Audio sampling frequency (Hz)  */
int SyncFreq    = 60;      /* Sync frequency (0=sync off)    */
int FastForward;           /* Fast-forwarded UPeriod backup  */
int SndSwitch;             /* Mask of enabled sound channels */
int SndVolume;             /* Master volume for audio        */
int Keypad;                /* Keypad key being pressed       */
Image ScrBuf;              /* Display buffer                 */

char *Title = (char*)"ColEm-pb 1.0";

int SetScrDepth(int Depth);
void HandleKeys(unsigned int Key);

/** InitMachine() ********************************************/
/** Allocate resources needed by machine-dependent code.    **/
/*************************************************************/
int InitMachine(void)
{
  int J;

  /* Initialize variables */
  UseZoom        = UseZoom<1? 1:UseZoom>5? 5:UseZoom;
  InMenu         = 0;
  FastForward    = 0;
  ScrBuf.Data    = 0;

  /* Initialize system resources */
  InitQNX(Title,UseZoom*WIDTH,UseZoom*HEIGHT);

  /* Set visual effects */
  BPSSetEffects(UseEffects);

  /* Initialize video */
  if(!NewImage(&ScrBuf,WIDTH,HEIGHT)) return(0);
  SetVideo(&ScrBuf,0,0,WIDTH,HEIGHT);
  ScrWidth  = WIDTH;
  ScrHeight = HEIGHT;
  ScrBuffer = (byte*)ScrBuf.Data;

  /* Attach keyboard handler */
  SetKeyHandler(HandleKeys);

  /* Initialize sound */
  InitSound(UseSound,150);
  SndSwitch=(1<<SN76489_CHANNELS)-1;
  SndVolume=255/SN76489_CHANNELS;
  SetChannels(SndVolume,SndSwitch);

  /* Initialize sync timer if needed */
  if((SyncFreq>0)&&!SetSyncTimer(SyncFreq*UPeriod/100)) SyncFreq=0;

  /* Done */
  return(1);
}

/** TrashMachine() *******************************************/
/** Deallocate all resources taken by InitMachine().        **/
/*************************************************************/
void TrashMachine(void)
{
  FreeImage(&ScrBuf);
  TrashAudio();
  TrashQNX();
  SDL_Quit();
}

/** Mouse() **************************************************/
/** This function is called to poll mouse. It should return **/
/** the X coordinate (-512..+512) in the lower 16 bits, the **/
/** Y coordinate (-512..+512) in the next 14 bits, and the  **/
/** mouse buttons in the upper 2 bits. All values should be **/
/** counted from the screen center!                         **/
/*************************************************************/
unsigned int Mouse(void)
{
  unsigned int X,Y;

  X = GetMouse();
  Y = ((((X>>16)&0x0FFF)<<10)/200-512)&0x3FFF;
  Y = (Y<<16)|((((X&0x0FFF)<<10)/320-512)&0xFFFF);
  return(Y|(X&MSE_BUTTONS));
}

/** Joystick() ***********************************************/
/** This function is called to poll joysticks. It should    **/
/** return 2x16bit values in a following way:               **/
/**                                                         **/
/**      x.FIRE-B.x.x.L.D.R.U.x.FIRE-A.x.x.N3.N2.N1.N0      **/
/**                                                         **/
/** Least significant bit on the right. Active value is 1.  **/
/*************************************************************/
unsigned int Joystick(void)
{
  unsigned int J,I;

  /* Render audio (two frames to avoid skipping) */
  RenderAndPlayAudio(UseSound/30);

  /* Get joystick state */
  I=0;
  J=GetJoystick();
  if(J&BTN_SHIFT)       J|=J&BTN_ALT? (BTN_FIREA<<16):BTN_FIREA;
  if(J&BTN_CONTROL)     J|=J&BTN_ALT? (BTN_FIREB<<16):BTN_FIREB;
  if(J&BTN_LEFT)        I|=J&BTN_ALT? (JST_LEFT<<16):JST_LEFT;
  if(J&BTN_RIGHT)       I|=J&BTN_ALT? (JST_RIGHT<<16):JST_RIGHT;
  if(J&BTN_UP)          I|=J&BTN_ALT? (JST_UP<<16):JST_UP;
  if(J&BTN_DOWN)        I|=J&BTN_ALT? (JST_DOWN<<16):JST_DOWN;
  if(J&BTN_FIREA)       I|=J&BTN_ALT? (JST_FIRER<<16):JST_FIRER;
  if(J&BTN_FIREB)       I|=J&BTN_ALT? (JST_FIREL<<16):JST_FIREL;
  if(J&BTN_FIREL)       I|=J&BTN_ALT? (JST_PURPLE<<16):JST_PURPLE;
  if(J&BTN_FIRER)       I|=J&BTN_ALT? (JST_BLUE<<16):JST_BLUE;
  if(J&(BTN_LEFT<<16))  I|=JST_LEFT<<16;
  if(J&(BTN_RIGHT<<16)) I|=JST_RIGHT<<16;
  if(J&(BTN_UP<<16))    I|=JST_UP<<16;
  if(J&(BTN_DOWN<<16))  I|=JST_DOWN<<16;
  if(J&(BTN_FIREA<<16)) I|=JST_FIRER<<16;
  if(J&(BTN_FIREB<<16)) I|=JST_FIREL<<16;
  if(J&(BTN_FIREL<<16)) I|=JST_PURPLE<<16;
  if(J&(BTN_FIRER<<16)) I|=JST_BLUE<<16;

  /* Add keypad buttons */
  I|=J&BTN_ALT? (Keypad<<16):Keypad;

  /* If exchanged keymaps with remote... */
  if(NETExchange((char *)&J,(const char *)&I,sizeof(I)))
    /* Merge joysticks, server is player #1, client is player #2 */
    I = NETConnected()==NET_SERVER?
        ((I&0xFFFF)|((J&0xFFFF)<<16))
      : ((J&0xFFFF)|((I&0xFFFF)<<16));

  /* Done */
  return(I);
}

/** RefreshScreen() ******************************************/
/** Refresh screen. This function is called in the end of   **/
/** refresh cycle to show the entire screen.                **/
/*************************************************************/
void RefreshScreen(void *Buffer,int Width,int Height)
{
  /* Display the screen buffer */
  ShowVideo();
}

/** SetColor() ***********************************************/
/** Set color N to (R,G,B).                                 **/
/*************************************************************/
int SetColor(byte N,byte R,byte G,byte B)
{
  return(BPSGetColor(R,G,B));
}

/** HandleKeys() *********************************************/
/** Key handler.                                            **/
/*************************************************************/
void HandleKeys(unsigned int Key)
{
  if(InMenu||CPU.Trace) return;
  int M;

  if(Key&CON_RELEASE)
    switch(Key&CON_KEYCODE)
    {
      case KEYCODE_F9:
      case KEYCODE_PG_UP:
        if(FastForward)
        {
          BPSSetEffects(UseEffects);
          VDP.DrawFrames=UPeriod=FastForward;
          FastForward=0;
        }
        break;
      case SDLK_F13:
		  int J;
		  J = MasterVolume;
		  SetChannels(MasterVolume, 0);
		  PauseAudio(1);
    	  M = ShowOptionMenu();
		  PauseAudio(0);
		  SetChannels(MasterVolume, J);
    	  switch (M)
    	  {
    	  case 1:
    		  Mode ^= CV_AUTOFIRER;
    		  break;
    	  case 2:
    		  Mode ^= CV_AUTOFIREL;
    		  break;
    	  case 3:
    		  BPSSetEffects(BPSGetEffects() ^ EFF_TVLINES);
    		  break;
    	  case 4:
    		  BPSSetEffects(BPSGetEffects() ^ EFF_SOFTEN);
    		  break;
    	  case 6:
    		  ResetColeco(Mode);
    		  break;
    	  }
    	  break;
      case '0':     Keypad&=~JST_0;break;
      case '1':     Keypad&=~JST_1;break;
      case '2':     Keypad&=~JST_2;break;
      case '3':     Keypad&=~JST_3;break;
      case '4':     Keypad&=~JST_4;break;
      case '5':     Keypad&=~JST_5;break;
      case '6':     Keypad&=~JST_6;break;
      case '7':     Keypad&=~JST_7;break;
      case '8':     Keypad&=~JST_8;break;
      case '9':     Keypad&=~JST_9;break;
      case '-':     Keypad&=~JST_STAR;break;
      case '=':     Keypad&=~JST_POUND;break;
    }
  else
    switch(Key&CON_KEYCODE)
    {
#ifdef DEBUG
      case KEYCODE_F1:
        CPU.Trace=1;
        break;
#endif
      case KEYCODE_F2: MIDILogging(MIDI_TOGGLE);break;
      case KEYCODE_F3: Mode^=CV_AUTOFIRER;break;
      case KEYCODE_F4: Mode^=CV_AUTOFIREL;break;
      //case KEYCODE_F5:
      //  InMenu=1;
      //  SetChannels(0,0);
      //  MenuColeco();
      //  SetChannels(SndVolume,SndSwitch);
      //  InMenu=0;
      //  break;
      case SDLK_F6:
      case KEYCODE_F6:
        LoadSTA(StaName? StaName:"DEFAULT.STA");
        break;
      case SDLK_F7:
      case KEYCODE_F7:
        SaveSTA(StaName? StaName:"DEFAULT.STA");
        break;
      case KEYCODE_F8:
        UseEffects^=Key&CON_ALT? EFF_SOFTEN:EFF_TVLINES;
        BPSSetEffects(UseEffects);
        break;
      case KEYCODE_F9:
      case KEYCODE_PG_UP:
        if(!FastForward)
        {
          BPSSetEffects(UseEffects&~EFF_SYNC);
          FastForward=UPeriod;
          VDP.DrawFrames=UPeriod=10;
        }
        break;
      case KEYCODE_F11:
        ResetColeco(Mode);
        break;
      case KEYCODE_ESCAPE:
      case KEYCODE_F12:
        ExitNow=1;
        break;
      case '0':     Keypad|=JST_0;break;
      case '1':     Keypad|=JST_1;break;
      case '2':     Keypad|=JST_2;break;
      case '3':     Keypad|=JST_3;break;
      case '4':     Keypad|=JST_4;break;
      case '5':     Keypad|=JST_5;break;
      case '6':     Keypad|=JST_6;break;
      case '7':     Keypad|=JST_7;break;
      case '8':     Keypad|=JST_8;break;
      case '9':     Keypad|=JST_9;break;
      case '-':     Keypad|=JST_STAR;break;
      case '=':     Keypad|=JST_POUND;break;
    }
}

/** Display.h ************************************************/
/** Display drivers for all possible display depths.        **/
/*************************************************************/
//#include "Display.h"
