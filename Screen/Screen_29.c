
/* $Log:	Screen.c,v $
 * Revision 1.1  12/.1/.0  .1:.1:.1  Robert
 * Initial revision
 *  */

/* Ekran podw�jnie buforowany */

#include <intuition/screens.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>

#include "Screen.h"

void zamknijEkran(struct Screen *s)
{
    struct screenInfo *si = (struct screenInfo *)s->UserData;

    FreeScreenBuffer(s, si->sb[1]);
    FreeScreenBuffer(s, si->sb[0]);
    CloseScreen(s);
    FreeBitMap(si->bm[1]);
    FreeBitMap(si->bm[0]);
    FreeMem(si, sizeof(*si));
}

struct Screen *otworzEkran(void)
{
    struct BitMap *bm[2];
    struct Screen *s;
    struct ScreenBuffer *sb[2];
    struct Rectangle clip =
    {
        0, 0, 319, 255
    };
    struct screenInfo *si;

    if (bm[0] = AllocBitMap(320, 256, 5, BMF_DISPLAYABLE|BMF_INTERLEAVED|BMF_CLEAR, NULL))
    {
        if (bm[1] = AllocBitMap(320, 256, 5, BMF_DISPLAYABLE|BMF_INTERLEAVED, NULL))
        {
            if (s = OpenScreenTags(NULL,
                SA_BitMap,      bm[0],
                SA_DClip,       &clip,
                SA_Depth,       5,
                SA_DisplayID,   LORES_KEY,
                SA_Quiet,       TRUE,
                SA_Exclusive,   TRUE,
                SA_ShowTitle,   FALSE,
                SA_BackFill,    LAYERS_NOBACKFILL,
                TAG_DONE))
            {
                if (sb[0] = AllocScreenBuffer(s, bm[0], 0))
                {
                    if (sb[1] = AllocScreenBuffer(s, bm[1], 0))
                    {
                        if (s->UserData = (APTR)si = AllocMem(sizeof(*si), MEMF_PUBLIC))
                        {
                            si->bm[0] = bm[0];
                            si->bm[1] = bm[1];
                            si->sb[0] = sb[0];
                            si->sb[1] = sb[1];
                            return(s);
                        }
                        FreeScreenBuffer(s, sb[1]);
                    }
                    FreeScreenBuffer(s, sb[0]);
                }
                CloseScreen(s);
            }
            FreeBitMap(bm[1]);
        }
        FreeBitMap(bm[0]);
    }
    return(NULL);
}
