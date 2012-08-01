/** ColEm: portable Coleco emulator **************************/
/**                                                         **/
/**                          Help.h                         **/
/**                                                         **/
/** This file contains help information printed out by the  **/
/** main() routine when started with option "-help".        **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2010                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

char *HelpText[] =
{
  (char*)"\nUsage: colem [-option1 [-option2...]] [filename]",
  (char*)"\n[filename] = Name of the file to load as a cartridge [CART.ROM]",
#if defined(ZLIB)
  (char*)"  This program will transparently uncompress singular GZIPped",
  (char*)"  and PKZIPped files.",
#endif
  (char*)"\n[-option]  =",
  (char*)"  -verbose <level>    - Select debugging messages [5]",
  (char*)"                        0 - Silent       1 - Startup messages",
  (char*)"                        2 - VDP          4 - Illegal Z80 ops",
  (char*)"                                        16 - Sound",
  (char*)"  -pal/-ntsc          - Video system to use [-ntsc]",
  (char*)"  -skip <percent>     - Percentage of frames to skip [25]",
  (char*)"  -help               - Print this help page",
  (char*)"  -home <dirname>     - Set directory with system ROM files [off]",
  (char*)"  -adam/-cv           - Run in Adam/ColecoVision mode [-cv]",
  (char*)"  -allspr             - Show all sprites [off]",
  (char*)"  -autoa/-noautoa     - Autofire/No autofire for FIRE-R button [-noautoa]",
  (char*)"  -autob/-noautob     - Autofire/No autofire for FIRE-L button [-noautob]",

  (char*)"  -spin1x/-spin1y     - Mouse X/Y position as SuperAction spinner 1 [off]",
  (char*)"  -spin2x/-spin2y     - Mouse X/Y position as SuperAction spinner 2 [off]",
  (char*)"  -drums/-nodrums     - Hit/Don't hit MIDI drums on noise [-nodrums]",
  (char*)"  -logsnd <filename>  - Write soundtrack to a MIDI file [LOG.MID]",
  (char*)"  -palette <number>   - Use given color palette [0]",
  (char*)"                        0 - Scaled VDP colors   1 - Original VDP colors",
  (char*)"                        2 - Faded NTSC colors",
  (char*)"  -sound [<quality>]  - Sound emulation quality [22050]",
  (char*)"  -nosound            - Don't emulate sound [-nosound]",

#if defined(DEBUG)
  (char*)"  -trap <address>     - Trap execution when PC reaches address [FFFFh]",
#endif /* DEBUG */

#if defined(MSDOS) || defined(UNIX) || defined(MAEMO)
  (char*)"  -sync <frequency>   - Sync screen updates to <frequency> [60]",
  (char*)"  -nosync             - Do not sync screen updates [-nosync]",
  (char*)"  -tv/-notv           - Simulate/Don't simulate scanlines [-notv]",
  (char*)"  -soft/-nosoft       - Scale screen up with softening [-nosoft]",
#endif /* MSDOS || UNIX || MAEMO */

#if defined(UNIX) || defined(MAEMO)
  (char*)"  -saver/-nosaver     - Save/don't save CPU when inactive [-saver]",
#endif /* UNIX || MAEMO */

#if defined(UNIX)
#if defined(MITSHM)
  (char*)"  -shm/-noshm         - Use/don't use MIT SHM extensions for X [-shm]",
#endif
  (char*)"  -scale <factor>     - Scale window by <factor> [2]",
#endif /* UNIX */

#if defined(MSDOS)
  (char*)"  -vsync              - Sync screen updates to VGA VBlanks [-vsync]",
#endif /* MSDOS */

  (char*)"\nKeyboard bindings:",
  (char*)"  [ALT]          - Hold to switch to the second controller",
  (char*)"  [SPACE]        - FIRE-R button (also: [SHIFT],A,S,D,F,G,H,J,K,L)",
  (char*)"  [CONTROL]      - FIRE-L button (also: Z,X,C,V,B,N,M)",
  (char*)"  [Q]            - SuperAction PURPLE button (also: E,T,U,O)",
  (char*)"  [W]            - SuperAction BLUE button (also: R,Y,I,P)",
  (char*)"  [0]-[9]        - Digit buttons",
  (char*)"  [-]            - [*] button",
  (char*)"  [=]            - [#] button",
  (char*)"  [PGUP]         - Fast-forward emulation (also: [F9])",
  (char*)"  [ESC]          - Quit emulation (also: [F12])",
#if defined(DEBUG)
  (char*)"  [F1]           - Go into the built-in debugger",
#endif
  (char*)"  [F2]           - Turn soundtrack log on/off",
  (char*)"  [F3]           - Toggle FIRE-R autofire on/off",
  (char*)"  [F4]           - Toggle FIRE-L autofire on/off",
  (char*)"  [F5]           - Invoke configuration menu",
  (char*)"  [F6]           - Load emulation state",
  (char*)"  [F7]           - Save emulation state",
  (char*)"  [F8]           - Turn scanline simulation on/off",
  (char*)"  [ALT]+[F8]     - Toggle screen softening on/off",
  (char*)"  [F9]           - Fast-forward emulation (also: [PGUP])",
#if defined(GIFLIB)
  (char*)"  [F10]          - Make a screen snapshot (SNAPxxxx.GIF)",
#endif
  (char*)"  [F11]          - Reset hardware",
  (char*)"  [F12]          - Quit emulation (also: [ESC])",
  (char*)"  [ALT]+[PGUP]   - Increase audio volume",
  (char*)"  [ALT]+[PGDOWN] - Decrease audio volume",
#if defined(WINDOWS)
  (char*)"  [ALT]+[ENTER]  - Switch between full scren and window modes",
#endif
  0
};
