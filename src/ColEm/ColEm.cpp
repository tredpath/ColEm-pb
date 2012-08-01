/** ColEm: portable Coleco emulator **************************/
/**                                                         **/
/**                          ColEm.c                        **/
/**                                                         **/
/** This file contains generic main() procedure statrting   **/
/** the emulation.                                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2010                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "Coleco.h"
#include "../EMULib/EMULib.h"
#include "Help.h"
#include "QNX/QNXMenu.h"

#include <bps/bps.h>
#include <bps/navigator.h>
#include <bps/dialog.h>

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

char *Options[]=
{ 
  (char*)"verbose",(char*)"pal",(char*)"ntsc",(char*)"skip",(char*)"help",
  (char*)"adam",(char*)"cv",(char*)"allspr",(char*)"autoa",(char*)"noautoa",(char*)"autob",(char*)"noautob",
  (char*)"spin1x",(char*)"spin1y",(char*)"spin2x",(char*)"spin2y",
  (char*)"drums",(char*)"nodrums",(char*)"logsnd",(char*)"palette",
  (char*)"home",(char*)"sound",(char*)"nosound",(char*)"trap",
  (char*)"sync",(char*)"nosync",(char*)"tv",(char*)"notv",(char*)"soft",(char*)"nosoft",
  (char*)"saver",(char*)"nosaver",(char*)"shm",(char*)"noshm",(char*)"scale",(char*)"vsync",
  0
};

extern const char *Title;/* Program title                     */
extern int   UseSound;   /* Sound mode                        */
extern int   UseZoom;    /* Scaling factor (UNIX)             */
extern int   UseEffects; /* EFF_* bits (UNIX/MSDOS)           */
extern int   SyncFreq;   /* Sync screen updates (UNIX/MSDOS)  */

/** main() ***************************************************/
/** This is a main() function used in Unix and MSDOS ports. **/
/** It parses command line arguments, sets emulation        **/
/** parameters, and passes control to the emulation itself. **/
/*************************************************************/
int main(int argc,char *argv[])
{
	bps_initialize();

	mkdir("shared/misc/ColEm", 0777);
	mkdir("shared/misc/ColEm/ROM", 0777);
	mkdir("shared/misc/ColEm/BIOS", 0777);
	mkdir("shared/misc/ColEm/save", 0777);

	char CartName[256];

	dialog_request_events(0);
	UpdateRomList();
	if (!AutoLoadRom(CartName))
	{
		bps_shutdown();
		return 0;
	}

#if defined(DEBUG)
  CPU.Trap=0xFFFF;
  CPU.Trace=0;
#endif

#if defined(MSDOS)
  Verbose=0;
#else
  Verbose=5;
#endif

  HomeDir = (char*)"shared/misc/ColEm/BIOS/";

#ifndef USE_SDL
  if (navigator_request_events(0) != BPS_SUCCESS)
    {
  	  bps_shutdown();
  	  return 0;
    }
#endif

  if(!InitMachine()) return(1);
  StartColeco(CartName);
  TrashColeco();
  TrashMachine();

  bps_shutdown();

  return(0);
}
