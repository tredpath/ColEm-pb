/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        SndUnix.c                        **/
/**                                                         **/
/** This file contains Unix-dependent sound implementation  **/
/** for the emulation library.                              **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2010                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "../EMULib.h"
#include "../Sound.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#if defined(PULSE_AUDIO)

#include <pulse/simple.h>

#elif defined(ESD_AUDIO)

#include "esd.h"

#elif defined(SUN_AUDIO)

#include <sys/audioio.h>
#include <sys/conf.h>
#include <stropts.h>

static const unsigned char ULAW[256] =
{
    31,   31,   31,   32,   32,   32,   32,   33,
    33,   33,   33,   34,   34,   34,   34,   35,
    35,   35,   35,   36,   36,   36,   36,   37,
    37,   37,   37,   38,   38,   38,   38,   39,
    39,   39,   39,   40,   40,   40,   40,   41,
    41,   41,   41,   42,   42,   42,   42,   43,
    43,   43,   43,   44,   44,   44,   44,   45,
    45,   45,   45,   46,   46,   46,   46,   47,
    47,   47,   47,   48,   48,   49,   49,   50,
    50,   51,   51,   52,   52,   53,   53,   54,
    54,   55,   55,   56,   56,   57,   57,   58,
    58,   59,   59,   60,   60,   61,   61,   62,
    62,   63,   63,   64,   65,   66,   67,   68,
    69,   70,   71,   72,   73,   74,   75,   76,
    77,   78,   79,   81,   83,   85,   87,   89,
    91,   93,   95,   99,  103,  107,  111,  119,
   255,  247,  239,  235,  231,  227,  223,  221,
   219,  217,  215,  213,  211,  209,  207,  206,
   205,  204,  203,  202,  201,  200,  199,  198,
   219,  217,  215,  213,  211,  209,  207,  206,
   205,  204,  203,  202,  201,  200,  199,  198,
   197,  196,  195,  194,  193,  192,  191,  191,
   190,  190,  189,  189,  188,  188,  187,  187,
   186,  186,  185,  185,  184,  184,  183,  183,
   182,  182,  181,  181,  180,  180,  179,  179,
   178,  178,  177,  177,  176,  176,  175,  175,
   175,  175,  174,  174,  174,  174,  173,  173,
   173,  173,  172,  172,  172,  172,  171,  171,
   171,  171,  170,  170,  170,  170,  169,  169,
   169,  169,  168,  168,  168,  168,  167,  167,
   167,  167,  166,  166,  166,  166,  165,  165,
   165,  165,  164,  164,  164,  164,  163,  163
};

#elif defined(__PLAYBOOK__)

#include <SDL.h>

#else /* !SUN_AUDIO && !ESD_AUDIO && !PULSE_AUDIO */

#ifdef __FreeBSD__
#include <sys/soundcard.h>
#endif

#ifdef __NetBSD__
#include <soundcard.h>
#endif

#ifdef __linux__
#include <sys/soundcard.h>
#endif

#endif /* !SUN_AUDIO && !ESD_AUDIO && !PULSE_AUDIO */

#if defined(SUN_AUDIO)
#define AUDIO_CONV(A) (ULAW[0xFF&(128+(A))])
#elif defined(BPS16)
#define AUDIO_CONV(A) (A)
#else
#define AUDIO_CONV(A) (128+(A))
#endif

#ifndef __PLAYBOOK__
static int SoundFD     = -1; /* Audio device descriptor      */
static pthread_t Thr   = 0;  /* Audio thread                 */
#endif
static int SndRate     = 0;  /* Audio sampling rate          */
static int SndSize     = 0;  /* SndData[] size               */
static sample *SndData = 0;  /* Audio buffers                */
static int RPtr        = 0;  /* Read pointer into Bufs       */
static int WPtr        = 0;  /* Write pointer into Bufs      */
static volatile int AudioPaused = 0; /* 1: Audio paused      */

/** ThrHandler() *********************************************/
/** This is the thread function responsible for sending     **/
/** buffers to the audio device.                            **/
/*************************************************************/
#if !defined(__PLAYBOOK__) && !defined(NO_SOUND)
static void *ThrHandler(void *Arg)
{
#ifndef NO_SOUND
  int J;

  // Spin until audiSo has been trashed
  for(RPtr=WPtr=0;SndRate&&SndData&&(SoundFD>=0);)
  {
#if defined(PULSE_AUDIO)
    if(SoundFD)
      pa_simple_write(SoundFD,SndData+RPtr,SND_BUFSIZE*sizeof(sample),0);
#elif defined(SUN_AUDIO)
    // Flush output first, don't care about return status. After this
    // write next buffer of audio data. This method produces a horrible
    // click on each buffer :( Any ideas, how to fix this?
    ioctl(SoundFD,AUDIO_DRAIN);
    write(SoundFD,SndData+RPtr,SND_BUFSIZE*sizeof(sample));
#elif defined(__PLAYBOOK__)

#else
    // We'll block here until next DMA buffer becomes free. It happens
    // once per SND_BUFSIZE/SndRate seconds.

    write(SoundFD,SndData+RPtr,SND_BUFSIZE*sizeof(sample));
#endif

    // Advance buffer pointer, clearing the buffer
    for(J=0;J<SND_BUFSIZE;++J) SndData[RPtr++]=AUDIO_CONV(0);
    if(RPtr>=SndSize) RPtr=0;
  }
#endif

  return(0);
}

#else

static void
sdl_audio_callback (void *userdata, Uint8 *stream, int len)
{
	/* Need to have valid playback rate */
	if(!SndRate) return;

	if (AudioPaused)
	{
		for (int J = 0; J < len; ++J)
			stream[J] = 0;
		return;
	}

	/* Recompute length in samples */
	len /= sizeof(sample);

	/* Copy audio data */
	for(int J = 0; J < len; ++J)
	{
		stream[J] = SndData[RPtr];
		RPtr = RPtr < SndSize - 1 ? RPtr + 1 : 0;
	}
}
#endif

/** InitAudio() **********************************************/
/** Initialize sound. Returns rate (Hz) on success, else 0. **/
/** Rate=0 to skip initialization (will be silent).         **/
/*************************************************************/
unsigned int InitAudio(unsigned int Rate,unsigned int Latency)
{
#ifndef NO_SOUND
  int I,J,K;

  // Shut down audio, just to be sure
  TrashAudio();
#ifndef __PLAYBOOK__
  SoundFD     = -1;
  Thr         = 0;
#else
  SDL_AudioSpec AudioFormat;
#endif
  SndRate     = 0;
  SndSize     = 0;
  SndData     = 0;
  RPtr        = 0;
  WPtr        = 0;
  AudioPaused = 0;

  // Have to have at least 8kHz sampling rate and 1ms buffer
  if((Rate<8000)||!Latency) return(0);

  // Compute number of sound buffers
  SndSize=Rate*Latency/1000;

  // Allocate audio buffers
  SndData=(sample *)malloc(SndSize*sizeof(sample));
  if(!SndData) { TrashSound();return(0); }

#if defined(__PLAYBOOK__)
	SDL_InitSubSystem (SDL_INIT_AUDIO);

	/* Set SDL audio settings */
	AudioFormat.freq     = Rate;
	AudioFormat.format   = sizeof(sample)>1? AUDIO_S16:AUDIO_S8;
	AudioFormat.channels = 1;
	AudioFormat.samples  = SndSize/2;
	AudioFormat.callback = sdl_audio_callback;
	AudioFormat.userdata = 0;

	printf ("SDL sound driver initializing...\n");
	printf ("    --> (Frequency: %dhz, Latency: %dms)...",
			AudioFormat.freq,
		(AudioFormat.samples * 1000 / AudioFormat.freq) << 1);

	if (SDL_OpenAudio (&AudioFormat, NULL) < 0)
	{
		printf ("Failed\n");

		free(SndData);
		SndData = NULL;

		return 0;
	}

	printf ("OK\n");

	// Clear audio buffers
	for(J=0;J<SndSize;++J) SndData[J]=AUDIO_CONV(0);

	// Thread expects valid SndRate!=0 at the start
	SndRate=Rate;

	SDL_PauseAudio (0);

#elif defined(PULSE_AUDIO)

  {
    // Configure PulseAudio sound
    pa_sample_spec PASpec;
    PASpec.format   = sizeof(sample)>1? PA_SAMPLE_S16LE:PA_SAMPLE_U8;
    PASpec.rate     = Rate;
    PASpec.channels = 1;
    // Try opening PulseAudio
    if(!(SoundFD=pa_simple_new(0,"EMULib",PA_STREAM_PLAYBACK,0,"playback",&PASpec,0,0,0)))
    { SoundFD=-1;return(0); }
  }

#elif defined(ESD_AUDIO)

  // ESD options for playing wave audio
  J=ESD_MONO|ESD_STREAM|ESD_PLAY|(sizeof(sample)>1? ESD_BITS16:ESD_BITS8);
  // Open ESD socket, fall back to /dev/dsp is no ESD
  if((SoundFD=esd_play_stream_fallback(J,Rate,0,0))<0) return(0);

#elif defined(SUN_AUDIO)

  // Open Sun's audio device
  if((SoundFD=open("/dev/audio",O_WRONLY|O_NONBLOCK))<0) return(0);

  // Sun's specific initialization should be here...
  // We assume, that it's set to 8000Hz u-law mono right now.

#else // !SUN_AUDIO

  // Open /dev/dsp audio device
  if((SoundFD=open("/dev/dsp",O_WRONLY))<0) return(0);
  // Set sound format
  J=sizeof(sample)>1? AFMT_S16_NE:AFMT_U8;
  I=ioctl(SoundFD,SNDCTL_DSP_SETFMT,&J)<0;
  // Set mono sound
  J=0;
  I|=ioctl(SoundFD,SNDCTL_DSP_STEREO,&J)<0;
  // Set sampling rate
  I|=ioctl(SoundFD,SNDCTL_DSP_SPEED,&Rate)<0;

  // Set buffer length and number of buffers
  J=K=SND_BITS|(SndSize<<16);
  I|=ioctl(SoundFD,SNDCTL_DSP_SETFRAGMENT,&J)<0;

  // Buffer length as n, not 2^n!
  if((J&0xFFFF)<=16) J=(J&0xFFFF0000)|(1<<(J&0xFFFF));
  K=SND_BUFSIZE|(SndSize<<16);

  // Check audio parameters
  I|=(J!=K)&&(((J>>16)<SndSize)||((J&0xFFFF)!=SND_BUFSIZE));

  // If something went wrong, drop out
  if(I) { TrashSound();return(0); }

#endif // !SUN_AUDIO

  // Create audio thread
#ifndef __PLAYBOOK__
  // SndSize now means the total buffer size
  SndSize*=SND_BUFSIZE;

	// Clear audio buffers
	for(J=0;J<SndSize;++J) SndData[J]=AUDIO_CONV(0);

	// Thread expects valid SndRate!=0 at the start
	SndRate=Rate;

  if(pthread_create(&Thr,0,ThrHandler,0)) { TrashSound();SndRate=0;return(0); }
#endif

  // Done, return effective audio rate
  return(SndRate);
#else
  return 0;
#endif
}

/** TrashAudio() *********************************************/
/** Free resources allocated by InitAudio().                **/
/*************************************************************/
void TrashAudio(void)
{
#ifndef NO_SOUND
  int Error;

  // Sound off, pause off
  SndRate     = 0;
  AudioPaused = 0;

#ifdef __PLAYBOOK__
  SDL_CloseAudio();
#else
  // Wait for audio thread to finish
  if(Thr) pthread_join(Thr,0);
#endif

  // Sound trashed
#ifndef __PLAYBOOK__
  // If audio was initialized...
  if(SoundFD>=0)
  {
#if defined(PULSE_AUDIO)
    if(SoundFD) pa_simple_free(SoundFD);
#elif defined(ESD_AUDIO)
    esd_close(SoundFD);
#elif defined(SUN_AUDIO)
    close(SoundFD);
#else
    ioctl(SoundFD,SNDCTL_DSP_RESET);
    close(SoundFD);
#endif
  }  

  SoundFD = -1;
  Thr     = 0;
#endif

  // If buffers were allocated...
  if(SndData) free(SndData);
  SndData = 0;
  SndSize = 0;
  RPtr    = 0;
  WPtr    = 0;
#endif
}

/** PauseAudio() *********************************************/
/** Pause/resume audio playback. Returns current playback   **/
/** state.                                                  **/
/*************************************************************/
int PauseAudio(int Switch)
{
  static int Rate,Latency;

  // If audio not initialized, return "off"
  if(!SndRate&&!AudioPaused) return(0);

  // Toggle audio status if requested
  if(Switch==2) Switch=AudioPaused? 0:1;

  // When switching audio state...
  if((Switch>=0)&&(Switch<=1)&&(Switch!=AudioPaused))
  {
#ifdef __PLAYBOOK__
	  SDL_PauseAudio(Switch);
#else
    if(Switch)
    {
      // Memorize audio parameters and kill audio
      Rate    = SndRate;
      Latency = 1000*SndSize/SndRate;
      TrashAudio();
    }
    else
    {
      // Start audio using memorized parameters
      if(!InitAudio(Rate,Latency)) Switch=0;
    }
#endif

    // Audio switched
    AudioPaused=Switch;
  }

  // Return current status
  return(AudioPaused);
}

/** GetFreeAudio() *******************************************/
/** Get the amount of free samples in the audio buffer.     **/
/*************************************************************/
unsigned int GetFreeAudio(void)
{
  return(!SndRate? 0:RPtr>=WPtr? RPtr-WPtr:RPtr-WPtr+SndSize);
}

/** WriteAudio() *********************************************/
/** Write up to a given number of samples to audio buffer.  **/
/** Returns the number of samples written.                  **/
/*************************************************************/
unsigned int WriteAudio(sample *Data,unsigned int Length)
{
  unsigned int J;

  // Require audio to be initialized
  if(!SndRate) return(0);

  // Copy audio samples
  for(J=0;(J<Length)&&(RPtr!=WPtr);++J)
  {
    SndData[WPtr++]=AUDIO_CONV(Data[J]);
    if(WPtr>=SndSize) WPtr=0;
  }

  // Return number of samples copied
  return(J);
}

