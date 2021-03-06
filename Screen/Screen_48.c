
/* Screen.c: Display context */

#include <stdio.h>

#include <exec/interrupts.h>
#include <exec/memory.h>
#include <intuition/screens.h>
#include <hardware/intbits.h>
#include <hardware/custom.h>
#include <graphics/gfxmacros.h>

#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/diskfont_protos.h>

#include "Screen.h"

extern void myCopper(void);
__far extern struct Custom custom;

struct BitMap *allocBitMap()
{
    return(AllocBitMap(320, 256, 5, BMF_DISPLAYABLE|BMF_INTERLEAVED|BMF_CLEAR, NULL));
}

struct Screen *openScreen(struct BitMap *bm, struct TextFont **tf)
{
    struct Screen *s;
    struct Rectangle dclip = { 0, 0, 319, 255 };
    static struct TextAttr ta =
    {
        "centurion.font", 9,
        FS_NORMAL,
        FPF_DISKFONT|FPF_DESIGNED
    };
    if (*tf = OpenDiskFont(&ta))
    {
        if (s = OpenScreenTags(NULL,
            SA_BitMap,      bm,
            SA_DisplayID,   LORES_KEY,
            SA_Font,        &ta,
            SA_DClip,       &dclip,
            SA_Quiet,       TRUE,
            SA_Exclusive,   TRUE,
            SA_ShowTitle,   FALSE,
            SA_BackFill,    LAYERS_NOBACKFILL,
            TAG_DONE))
        {
            return(s);
        }
        CloseFont(*tf);
    }
    else
    {
        printf("Prosze zainstalowac czcionke centurion.font z katalogu Fonts.\n");
    }
    return(NULL);
}

BOOL addCopperInt(struct Interrupt *is, struct copperData *cd, struct ViewPort *vp)
{
    is->is_Code = myCopper;
    is->is_Data = cd;
    is->is_Node.ln_Pri = 0;
    is->is_Node.ln_Name = "GearWorks";

    if ((cd->signal = AllocSignal(-1)) != -1)
    {
        cd->vp = vp;
        cd->task = FindTask(NULL);
        AddIntServer(INTB_COPER, is);
        return(TRUE);
    }
    return(FALSE);
}

void remCopperInt(struct Interrupt *is)
{
    struct copperData *cd = (struct copperData *)is->is_Data;

    RemIntServer(INTB_COPER, is);

    FreeSignal(cd->signal);
}

BOOL addCopperList(struct ViewPort *vp)
{
    struct UCopList *ucl;

    if (ucl = AllocMem(sizeof(*ucl), MEMF_PUBLIC|MEMF_CLEAR))
    {
        CINIT(ucl, 3);
        CWAIT(ucl, COPLINE, 0);
        CMOVE(ucl, custom.intreq, INTF_SETCLR|INTF_COPER);
        CEND(ucl);

        Forbid();
        vp->UCopIns = ucl;
        Permit();

        RethinkDisplay();
        return(TRUE);
    }
    return(FALSE);
}
