/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        TMS9918.c                        **/
/**                                                         **/
/** This file contains emulation for the TMS9918 video chip **/
/** produced by Texas Instruments. See TMS9918.h for        **/
/** declarations.                                           **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2010                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "TMS9918.h"
#include "DRV9918.h"
#include <stdlib.h>
#include <string.h>

/** Palette9918[] ********************************************/
/** 16 standard colors used by TMS9918/TMS9928 VDP chips.   **/
/*************************************************************/
RGB Palette9918[16*3] =
{
  /* Palette taken from the datasheet and scaled by 255/224 */
  {0x00,0x00,0x00},{0x00,0x00,0x00},{0x24,0xDA,0x24},{0x6D,0xFF,0x6D},
  {0x24,0x24,0xFF},{0x48,0x6D,0xFF},{0xB6,0x24,0x24},{0x48,0xDA,0xFF},
  {0xFF,0x24,0x24},{0xFF,0x6D,0x6D},{0xDA,0xDA,0x24},{0xDA,0xDA,0x91},
  {0x24,0x91,0x24},{0xDA,0x48,0xB6},{0xB6,0xB6,0xB6},{0xFF,0xFF,0xFF},
  /* TMS9918 Palette from the datasheet */
  {0x00,0x00,0x00},{0x00,0x00,0x00},{0x20,0xC0,0x20},{0x60,0xE0,0x60},
  {0x20,0x20,0xE0},{0x40,0x60,0xE0},{0xA0,0x20,0x20},{0x40,0xC0,0xE0},
  {0xE0,0x20,0x20},{0xE0,0x60,0x60},{0xC0,0xC0,0x20},{0xC0,0xC0,0x80},
  {0x20,0x80,0x20},{0xC0,0x40,0xA0},{0xA0,0xA0,0xA0},{0xE0,0xE0,0xE0},
  /* TMS9918 NTSC Palette by Richard F. Drushel */
  {0x00,0x00,0x00},{0x00,0x00,0x00},{0x47,0xB7,0x3B},{0x7C,0xCF,0x6F},
  {0x5D,0x4E,0xFF},{0x80,0x72,0xFF},{0xB6,0x62,0x47},{0x5D,0xC8,0xED},
  {0xD7,0x6B,0x48},{0xFB,0x8F,0x6C},{0xC3,0xCD,0x41},{0xD3,0xDA,0x76},
  {0x3E,0x9F,0x2F},{0xB6,0x64,0xC7},{0xCC,0xCC,0xCC},{0xFF,0xFF,0xFF}
};

/** Screen9918[] *********************************************/
/** Pointer to the scanline refresh handlers and VDP table  **/
/** address masks for the standard TMS9918 screen modes.    **/
/*************************************************************/
struct
{
  void (*LineHandler)(TMS9918 *VDP,byte Y);
  byte R2,R3,R4,R5,R6,M2,M3,M4,M5;
} Screen9918[4] =
{
  { RefreshLine0,0x7F,0x00,0x3F,0x00,0x3F,0x00,0x00,0x00,0x00 },/* SCREEN 0:TEXT 40x24    */
  { RefreshLine1,0x7F,0xFF,0x3F,0xFF,0x3F,0x00,0x00,0x00,0x00 },/* SCREEN 1:TEXT 32x24    */
  { RefreshLine2,0x7F,0x80,0x3C,0xFF,0x3F,0x00,0x7F,0x03,0x00 },/* SCREEN 2:BLOCK 256x192 */
  { RefreshLine3,0x7F,0x00,0x3F,0xFF,0x3F,0x00,0x00,0x00,0x00 },/* SCREEN 3:GFX 64x48x16  */
};

/** Static Functions *****************************************/
/** Functions used internally by the TMS9918 emulation.     **/
/*************************************************************/
static byte CheckSprites(TMS9918 *VDP);

/** New9918() ************************************************/
/** Create a new VDP context. The user can either provide   **/
/** his own screen buffer by pointing Buffer to it, or ask  **/
/** New9918() to allocate a buffer by setting Buffer to 0.  **/
/** Width and Height must always specify screen buffer      **/
/** dimensions. New9918() returns pointer to the screen     **/
/** buffer on success, 0 otherwise.                         **/
/*************************************************************/
byte *New9918(TMS9918 *VDP,byte *Buffer,int Width,int Height)
{
  /* Allocate memory for VRAM */
  VDP->VRAM=(byte *)malloc(0x4000);
  if(!VDP->VRAM) return(0);

  /* Reset VDP */
  VDP->DrawFrames = TMS9918_DRAWFRAMES;
  VDP->MaxSprites = TMS9918_MAXSPRITES;
  VDP->Scanlines  = TMS9918_LINES;
  VDP->OwnXBuf    = 0;
  Reset9918(VDP,Buffer,Width,Height);

  /* If needed, allocate screen buffer */
  if(!Buffer)
  {
    Buffer=(byte*)malloc(Width*Height*sizeof(pixel));
    if(Buffer) { VDP->XBuf=Buffer;VDP->OwnXBuf=1; }
    else       { free(VDP->VRAM);return(0); }
  }

  /* Done */
  return(VDP->XBuf);
}

/** Reset9918() **********************************************/
/** Reset the VDP. The user can provide a new screen buffer **/
/** by pointing Buffer to it and setting Width and Height.  **/
/** Set Buffer to 0 to use the existing screen buffer.      **/
/*************************************************************/
void Reset9918(TMS9918 *VDP,byte *Buffer,int Width,int Height)
{
  /* If new screen buffer is passed, set it */
  if(Buffer)
  {
    if(VDP->OwnXBuf&&VDP->XBuf) free(VDP->XBuf);
    VDP->XBuf    = Buffer;
    VDP->Width   = Width;
    VDP->Height  = Height;
    VDP->OwnXBuf = 0;
  }

  memset(VDP->VRAM,0x00,0x4000);
  memset(VDP->R,0x00,sizeof(VDP->R));

  VDP->UCount  = 0;
  VDP->VAddr   = 0x0000;
  VDP->Status  = 0x00;
  VDP->VKey    = 1;
  VDP->WKey    = 1;
  VDP->Mode    = 0;
  VDP->Line    = 0;
  VDP->CLatch  = 0;
  VDP->DLatch  = 0;
  VDP->FGColor = 0;
  VDP->BGColor = 0;

  VDP->ChrTab  = VDP->VRAM;
  VDP->ChrGen  = VDP->VRAM;
  VDP->ColTab  = VDP->VRAM;
  VDP->SprTab  = VDP->VRAM;
  VDP->SprGen  = VDP->VRAM;

  VDP->ChrTabM = ~0;
  VDP->ColTabM = ~0;
  VDP->ChrGenM = ~0;
  VDP->SprTabM = ~0;
}

/** Trash9918() **********************************************/
/** Free all buffers associated with VDP and invalidate VDP **/
/** context. Use this to shut down VDP.                     **/
/*************************************************************/
void Trash9918(TMS9918 *VDP)
{
  /* Free all allocated memory */
  if(VDP->VRAM)               free(VDP->VRAM);
  if(VDP->XBuf&&VDP->OwnXBuf) free(VDP->XBuf);

  VDP->VRAM    = 0;
  VDP->XBuf    = 0;
  VDP->OwnXBuf = 0;
}

/** Write9918() **********************************************/
/** This is a convinience function provided for the user to **/
/** write into VDP control registers. This can also be done **/
/** by two consequent WrCtrl9918() calls. Enabling IRQs in  **/
/** this function may cause an IRQ to be generated. In this **/
/** case, Write9918() returns 1. Returns 0 otherwise.       **/
/*************************************************************/
byte Write9918(TMS9918 *VDP,byte R,byte V)
{
  int VRAMMask;
  byte IRQ;

  /* Enabling IRQs may cause an IRQ here */
  IRQ  = (R==1)
      && ((VDP->R[1]^V)&V&TMS9918_REG1_IRQ)
      && (VDP->Status&TMS9918_STAT_VBLANK);

  /* VRAM can either be 4kB or 16kB */
  VRAMMask = (R==1) && ((VDP->R[1]^V)&TMS9918_REG1_RAM16K)? 0
           : TMS9918_VRAMMask(VDP);

  /* Store value into the register */
  VDP->R[R]=V;

  /* Depending on the register, do... */  
  switch(R)  
  {
    case 0: /* Mode register 0 */
    case 1: /* Mode register 1 */
      /* Figure out new screen mode number */
      switch(TMS9918_Mode(VDP))
      {
        case 0x00: V=1;break;
        case 0x01: V=2;break;
        case 0x02: V=0;break;
        case 0x04: V=3;break;
        default:   V=VDP->Mode;break;
      }

      /* If mode was changed, recompute table addresses */
      if((V!=VDP->Mode)||!VRAMMask)
      {
        VRAMMask     = TMS9918_VRAMMask(VDP);
        VDP->ChrTab  = VDP->VRAM+(((int)(VDP->R[2]&Screen9918[V].R2)<<10)&VRAMMask);
        VDP->ColTab  = VDP->VRAM+(((int)(VDP->R[3]&Screen9918[V].R3)<<6)&VRAMMask);
        VDP->ChrGen  = VDP->VRAM+(((int)(VDP->R[4]&Screen9918[V].R4)<<11)&VRAMMask);
        VDP->SprTab  = VDP->VRAM+(((int)(VDP->R[5]&Screen9918[V].R5)<<7)&VRAMMask);
        VDP->SprGen  = VDP->VRAM+(((int)(VDP->R[6]&Screen9918[V].R6)<<11)&VRAMMask);
        VDP->ChrTabM = ((int)(VDP->R[2]|~Screen9918[V].M2)<<10)|0x03FF;
        VDP->ColTabM = ((int)(VDP->R[3]|~Screen9918[V].M3)<<6)|0x1C03F;
        VDP->ChrGenM = ((int)(VDP->R[4]|~Screen9918[V].M4)<<11)|0x007FF;
        VDP->SprTabM = ((int)(VDP->R[5]|~Screen9918[V].M5)<<7)|0x1807F;
        VDP->Mode    = V;
      }
      break;

    case 2: /* Name Table */
      VDP->ChrTab  = VDP->VRAM+(((int)(V&Screen9918[VDP->Mode].R2)<<10)&VRAMMask);
      VDP->ChrTabM = ((int)(V|~Screen9918[VDP->Mode].M2)<<10)|0x03FF;
      break;
    case 3: /* Color Table */
      VDP->ColTab  = VDP->VRAM+(((int)(V&Screen9918[VDP->Mode].R3)<<6)&VRAMMask);
      VDP->ColTabM = ((int)(V|~Screen9918[VDP->Mode].M3)<<6)|0x1C03F;
      break;
    case 4: /* Pattern Table */
      VDP->ChrGen  = VDP->VRAM+(((int)(V&Screen9918[VDP->Mode].R4)<<11)&VRAMMask);
      VDP->ChrGenM = ((int)(V|~Screen9918[VDP->Mode].M4)<<11)|0x007FF;
      break;
    case 5: /* Sprite Attributes */
      VDP->SprTab  = VDP->VRAM+(((int)(V&Screen9918[VDP->Mode].R5)<<7)&VRAMMask);
      VDP->SprTabM = ((int)(V|~Screen9918[VDP->Mode].M5)<<7)|0x1807F;
      break;
    case 6: /* Sprite Patterns */
      VDP->SprGen  = VDP->VRAM+(((int)(V&Screen9918[VDP->Mode].R6)<<11)&VRAMMask);
      break;

    case 7: /* Foreground and background colors */
      VDP->FGColor=VDP->XPal[V>>4];
      V&=0x0F;
      VDP->XPal[0]=VDP->XPal[V? V:1];
      VDP->BGColor=VDP->XPal[V];
      break;
  }

  /* Return IRQ, if generated */
  return(IRQ);
}

/** Loop9918() ***********************************************/
/** Call this routine on every scanline to update the       **/
/** screen buffer. Loop9918() returns 1 if an interrupt is  **/
/** to be generated, 0 otherwise.                           **/
/*************************************************************/
byte Loop9918(TMS9918 *VDP)
{
  register byte IRQ;

  /* No IRQ yet */
  IRQ=0;

  /* Increment scanline */
  if(++VDP->Line>=VDP->Scanlines) VDP->Line=0;

  /* If refreshing display area, call scanline handler */
  if((VDP->Line>=TMS9918_START_LINE)&&(VDP->Line<TMS9918_END_LINE))
    if(VDP->UCount>=100)
      Screen9918[VDP->Mode].LineHandler(VDP,VDP->Line-TMS9918_START_LINE);

  /* If time for VBlank... */
  if(VDP->Line==TMS9918_END_LINE)
  {
    /* If drawing screen... */
    if(VDP->UCount>=100)
    {
      /* Refresh screen */
      RefreshScreen(VDP->XBuf,VDP->Width,VDP->Height);
      /* Reset update counter */
      VDP->UCount-=100;
    }

    /* Decrement/reset update counter */
    VDP->UCount+=VDP->DrawFrames;

    /* Generate IRQ when enabled and when VBlank flag goes up */
    IRQ=TMS9918_VBlankON(VDP)&&!(VDP->Status&TMS9918_STAT_VBLANK);

    /* Set VBlank status flag */
    VDP->Status|=TMS9918_STAT_VBLANK;

    /* Set Sprite Collision status flag */
    if(!(VDP->Status&TMS9918_STAT_OVRLAP))
      if(CheckSprites(VDP)) VDP->Status|=TMS9918_STAT_OVRLAP;
  }

  /* Done */
  return(IRQ);
}

/** WrCtrl9918() *********************************************/
/** Write a value V to the VDP Control Port. Enabling IRQs  **/
/** in this function may cause an IRQ to be generated. In   **/
/** this case, WrCtrl9918() returns 1. Returns 0 otherwise. **/
/*************************************************************/
byte WrCtrl9918(TMS9918 *VDP,byte V)
{
  if(VDP->VKey) { VDP->VKey=0;VDP->CLatch=V; }
  else
  {
    VDP->VKey=1;
    switch(V&0xC0)
    {
      case 0x00:
      case 0x40:
        VDP->VAddr=(VDP->CLatch|((int)V<<8))&0x3FFF;
        VDP->WKey=V&0x40;
/*
        if(!VDP->WKey)
        {
          VDP->DLatch=VDP->VRAM[VDP->VAddr];
          VDP->VAddr=(VDP->VAddr+1)&0x3FFF;
        }
*/
        break;
      case 0x80:
        /* Enabling IRQs may cause an IRQ here */ 
        return(Write9918(VDP,V&0x0F,VDP->CLatch));
    }
  }

  /* No interrupts */
  return(0);
}

/** RdCtrl9918() *********************************************/
/** Read a value from the VDP Control Port.                 **/
/*************************************************************/
byte RdCtrl9918(TMS9918 *VDP)
{
  register byte J;

  J           = VDP->Status;
  VDP->Status&= TMS9918_STAT_5THNUM;
  VDP->VKey   = 1;
  return(J);
}

/** WrData9918() *********************************************/
/** Write a value V to the VDP Data Port.                   **/
/*************************************************************/
void WrData9918(TMS9918 *VDP,byte V)
{
  if(VDP->WKey)
  {
    /* VDP in the proper WRITE mode */
    VDP->DLatch = VDP->VRAM[VDP->VAddr]=V;
    VDP->VAddr  = (VDP->VAddr+1)&0x3FFF;
  }
  else
  {
    /* VDP in the READ mode */
    VDP->DLatch = VDP->VRAM[VDP->VAddr];
    VDP->VAddr  = (VDP->VAddr+1)&0x3FFF;
    VDP->VRAM[VDP->VAddr]=V;
  }
}

/** RdData9918() *********************************************/
/** Read a value from the VDP Data Port.                    **/
/*************************************************************/
byte RdData9918(TMS9918 *VDP)
{
  register byte J;

/*
  J=VDP->DLatch;
  VDP->DLatch=VDP->VRAM[VDP->VAddr];
*/
  J          = VDP->VRAM[VDP->VAddr];
  VDP->VAddr = (VDP->VAddr+1)&0x3FFF;
  return(J);
}

/** CheckSprites() *******************************************/
/** This function is periodically called to check for the   **/
/** sprite collisions and return 1 if collision has occured **/
/*************************************************************/
byte CheckSprites(TMS9918 *VDP)
{
  register int LS,LD;
  register byte DH,DV,*PS,*PD,*T;
  byte I,J,N,*S,*D;

  /* Find first sprite to display */
  for(N=0,S=VDP->SprTab;(N<32)&&(S[0]!=208);N++,S+=4);

  if(TMS9918_Sprites16(VDP))
  {
    for(J=0,S=VDP->SprTab;J<N;J++,S+=4)
      if(S[3]&0x0F)
        for(I=J+1,D=S+4;I<N;I++,D+=4)
          if(D[3]&0x0F)
          {
            DV=S[0]-D[0];
            if((DV<16)||(DV>240))
            {
              DH=S[1]-D[1];
              if((DH<16)||(DH>240))
              {
                PS=VDP->SprGen+((int)(S[2]&0xFC)<<3);
                PD=VDP->SprGen+((int)(D[2]&0xFC)<<3);
                if(DV<16) PD+=DV; else { DV=256-DV;PS+=DV; }
                if(DH>240) { DH=256-DH;T=PS;PS=PD;PD=T; }
                while(DV<16)
                {
                  LS=((int)PS[0]<<8)+PS[16];
                  LD=((int)PD[0]<<8)+PD[16];
                  if(LD&(LS>>DH)) break;
                  else { DV++;PS++;PD++; }
                }
                if(DV<16) return(1);
              }
            }
          }
  }
  else
  {
    for(J=0,S=VDP->SprTab;J<N;J++,S+=4)
      if(S[3]&0x0F)
        for(I=J+1,D=S+4;I<N;I++,D+=4)
          if(D[3]&0x0F)
          {
            DV=S[0]-D[0];
            if((DV<8)||(DV>248))
            {
              DH=S[1]-D[1];
              if((DH<8)||(DH>248))
              {
                PS=VDP->SprGen+((int)S[2]<<3);
                PD=VDP->SprGen+((int)D[2]<<3);
                if(DV<8) PD+=DV; else { DV=256-DV;PS+=DV; }
                if(DH>248) { DH=256-DH;T=PS;PS=PD;PD=T; }
                while((DV<8)&&!(*PD&(*PS>>DH))) { DV++;PS++;PD++; }
                if(DV<8) return(1);
              }
            }
          }
  }

  /* No collision */
  return(0);
}
