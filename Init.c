
/* Init.c: Game init */

/* $Id$ */

#include <stdio.h>

#include "Init.h"

#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/exec_protos.h>

struct Library *IntuitionBase;

BOOL openLibs(void)
{
    if (IntuitionBase = OpenLibrary("intuition.library", 39))
    {
        return(TRUE);
    }
    printf("This program requires Amiga OS3.0+\n");
    return(FALSE);
}

void closeLibs(void)
{
    CloseLibrary(IntuitionBase);
}

/* Init Screen part */
BOOL initGameScreen(struct gameInit *gi)
{
    if (gi->bm[0] = allocBitMap())
    {
        if (gi->bm[1] = allocBitMap())
        {
            if (gi->s = openScreen(gi->bm[0], &gi->tf))
            {
                if (addCopperList(&gi->s->ViewPort))
                {
                    if (addCopperInt(&gi->copis, &gi->copdata, &gi->s->ViewPort))
                    {
                        return(TRUE);
                    }
                }
                CloseScreen(gi->s);
                CloseFont(gi->tf);
            }
            FreeBitMap(gi->bm[1]);
        }
        FreeBitMap(gi->bm[0]);
    }
    return(FALSE);
}

void freeGameScreen(struct gameInit *gi)
{
    remCopperInt(&gi->copis);
    CloseScreen(gi->s);
    CloseFont(gi->tf);
    FreeBitMap(gi->bm[1]);
    FreeBitMap(gi->bm[0]);
}

/* Init game graphics */
BOOL initGameGfx(struct gameInit *gi)
{
    struct IFFHandle *iff;
    BOOL result = FALSE;

    if (iff = openIFF("Data1/Gfx/Graphics.iff", IFFF_READ))
    {
        if (scanILBM(iff))
        {
            if (loadCMAP(iff, gi->s))
            {
                if (gi->gfx = loadBitMap(iff))
                {
                    result = TRUE;
                }
            }
        }
        closeIFF(iff);
    }
    return(result);
}

void freeGameGfx(struct gameInit *gi)
{
    FreeBitMap(gi->gfx);
}

/* Init game sound effects */
BOOL initGameSfx(struct gameInit *gi)
{
    if (gi->ioa = allocChannels())
    {
        if (loadSample("Data1/Sfx/Step.iff", gi->samples + SAMPLE_DIG))
        {
            if (loadSample("Data1/Sfx/Box.iff", gi->samples + SAMPLE_BOX))
            {
                return(TRUE);
            }
            freeSample(gi->samples + SAMPLE_DIG);
        }
        freeChannels(gi->ioa);
    }
    return(FALSE);
}

void freeGameSfx(struct gameInit *gi)
{
    freeSample(gi->samples + SAMPLE_BOX);
    freeSample(gi->samples + SAMPLE_DIG);
    freeChannels(gi->ioa);
}

/* Init game windows */
BOOL initGameWindows(struct gameInit *gi)
{
    if (initWindow(&gi->wi, gi->gfx))
    {
        initEditorMenu(&gi->mi, &gi->wi);

        if (gi->w = openBDWindow(gi->s, &gi->wi))
        {
            return(TRUE);
        }
        freeWindow(&gi->wi);
    }
    return(FALSE);
}

void freeGameWindows(struct gameInit *gi)
{
    CloseWindow(gi->w);
    freeWindow(&gi->wi);
}

BOOL initGame(struct gameInit *gi)
{
    if (openLibs())
    {
        if (initGameScreen(gi))
        {
            if (initGameGfx(gi))
            {
                if (initGameSfx(gi))
                {
                    if (initGameWindows(gi))
                    {
                        return(TRUE);
                    }
                    freeGameSfx(gi);
                }
                freeGameGfx(gi);
            }
            freeGameScreen(gi);
        }
        closeLibs();
    }
    return(FALSE);
}

void freeGame(struct gameInit *gi)
{
    freeGameWindows(gi);
    freeGameSfx(gi);
    freeGameGfx(gi);
    freeGameScreen(gi);
    closeLibs();
}
