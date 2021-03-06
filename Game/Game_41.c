
/* Game.c: Main game code */

/* $Id: Game.c,v 1.1 12/.0/.1 .2:.4:.0 Robert Exp $ */

#include <stdio.h>

#include <dos/dos.h>
#include <exec/interrupts.h>
#include <libraries/iffparse.h>
#include <graphics/rpattr.h>
#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/iffparse_protos.h>

#include "debug.h"

#include "Init.h"

#include "Tile.h"
#include "Game.h"

#define SNDCHAN 0 /* Channel for sound effects */

#define SPEED 10

#define NAME 64

#define LEFT_KEY 0x4f
#define RIGHT_KEY 0x4e
#define UP_KEY 0x4c
#define DOWN_KEY 0x4d

#define ID_MAGA MAKE_ID('M','A','G','A')
#define ID_NAGL MAKE_ID('N','A','G','L')
#define ID_PLAN MAKE_ID('P','L','A','N')
#define ID_STAN MAKE_ID('S','T','A','N')

WORD globLevel = 1;

void initBoard(struct Board *b)
{
    WORD x, y;
    struct Cell floor = { FLOOR_KIND, NORMAL_FLOOR, NORMAL_FLOOR };
    struct Cell wall = { WALL_KIND, NORMAL_WALL };

    for (y = 0; y < BOARD_HEIGHT; y++)
    {
        for (x = 0; x < BOARD_WIDTH; x++)
        {
            if (x == 0 || x == BOARD_WIDTH - 1 || y == 0 || y == BOARD_HEIGHT - 1)
                b->board[y][x] = wall;
            else
                b->board[y][x] = floor;
        }
    }
}

BOOL scanBoard(struct Board *b)
{
    struct boardInfo *info = &b->info;
    WORD x, y;
    BOOL hero = FALSE;

    info->boxes = 0;
    info->keys = 0;
    info->placed = 0;

    for (y = 0; y < BOARD_HEIGHT; y++)
    {
        for (x = 0; x < BOARD_WIDTH; x++)
        {
            struct Cell *c = &b->board[y][x];

            if (c->kind == OBJECT_KIND)
            {
                if (c->subKind == HERO_OBJECT)
                {
                    hero = TRUE;
                    info->heroX = x;
                    info->heroY = y;
                }
                else if (c->subKind == BOX_OBJECT)
                {
                    info->boxes++;
                }
            }
        }
    }
    return(hero);
}

void drawMessage(struct gameInit *gi, struct RastPort *rp, STRPTR text, WORD x, WORD y)
{
    SetABPenDrMd(rp, 27, 20, JAM1);
    Move(rp, x, y + rp->Font->tf_Baseline);
    Text(rp, text, strlen(text));

    SetABPenDrMd(rp, 4, 20, JAM1);
    Move(rp, x - 1, y - 1 + rp->Font->tf_Baseline);
    Text(rp, text, strlen(text));
}

LONG standardMessage(struct gameInit *gi, struct Window *reqWin)
{
    struct copperData *cd = &gi->copdata;
    BOOL done = FALSE;

    BltBitMap(gi->gfx, 128, 128, gi->bm[1], 64, 64, 192, 128, 0xc0, 0xff, NULL);

    reqWin->RPort->BitMap = gi->bm[1];

    drawMessage(gi, reqWin->RPort, "Gratulacje!", 64, 24);
    drawMessage(gi, reqWin->RPort, "Przeszed�e� poziom!", 64, 64);

    SetSignal(0L, 1L << cd->signal);

    Wait(1L << cd->signal);
    drawTile(gi->bm[1], 64, 64, gi->bm[0], 64, 64, 192, 128);

    AddGList(reqWin, gi->ri.gads, -1, -1, NULL);
    RefreshGList(gi->ri.gads, reqWin, NULL, -1);

    while (!done)
    {
        struct IntuiMessage *msg;

        WaitPort(reqWin->UserPort);

        while (msg = (struct IntuiMessage *)GetMsg(reqWin->UserPort))
        {
            if (msg->Class == IDCMP_GADGETUP)
            {
                /* Check GID */
                done = TRUE;
            }
            else if (msg->Class == IDCMP_RAWKEY)
            {
                if (msg->Code == 0x40 || msg->Code == 0x44)
                {
                    /* OK */
                    done = TRUE;
                }
                else if (msg->Code == 0x45)
                {
                    /* Cancel */
                    done = TRUE;
                }
            }
            ReplyMsg((struct Message *)msg);
        }
    }
}

LONG levelSelection(struct gameInit *gi, struct Window *reqWin)
{
    struct copperData *cd = &gi->copdata;
    BOOL done = FALSE;
    UBYTE text[64];
    struct Gadget prevGad, nextGad;
    WORD level = gi->state.info.level;

    initButton(&prevGad, NULL, RID_PREV, 64, 40, gi->wi.img + IMG_PREV, gi->wi.img + IMG_PREV_PRESSED);
    initButton(&nextGad, NULL, RID_NEXT, 128, 40, gi->wi.img + IMG_NEXT, gi->wi.img + IMG_NEXT_PRESSED);

    BltBitMap(gi->gfx, 128, 128, gi->bm[1], 64, 64, 192, 128, 0xc0, 0xff, NULL);

    reqWin->RPort->BitMap = gi->bm[1];

    sprintf(text, "Poziom %d", level);
    drawMessage(gi, reqWin->RPort, "Wybierz poziom", 64, 24);
    drawMessage(gi, reqWin->RPort, text, 64, 64);

    SetSignal(0L, 1L << cd->signal);

    Wait(1L << cd->signal);
    drawTile(gi->bm[1], 64, 64, gi->bm[0], 64, 64, 192, 128);

    AddGList(reqWin, gi->ri.gads, -1, -1, NULL);
    AddGadget(reqWin, &prevGad, -1);
    AddGadget(reqWin, &nextGad, -1);
    RefreshGList(gi->ri.gads, reqWin, NULL, -1);
    RefreshGList(&prevGad, reqWin, NULL, -1);
    RefreshGList(&nextGad, reqWin, NULL, -1);

    reqWin->RPort->BitMap = gi->bm[0];

    while (!done)
    {
        struct IntuiMessage *msg;

        WaitPort(reqWin->UserPort);

        while (msg = (struct IntuiMessage *)GetMsg(reqWin->UserPort))
        {
            if (msg->Class == IDCMP_GADGETUP)
            {
                struct Gadget *gad = (struct Gadget *)msg->IAddress;
                /* Check GID */
                if (gad->GadgetID == RID_NEXT)
                {
                    WORD maxLevel = 6;

                    if (level < maxLevel)
                        level++;
                    sprintf(text, "Poziom %d", level);
                    SetAPen(reqWin->RPort, 20);
                    RectFill(reqWin->RPort, 94, 63, 112, 74);
                    drawMessage(gi, reqWin->RPort, text, 64, 64);
                }
                else if (gad->GadgetID == RID_PREV)
                {
                    WORD maxLevel = 6;

                    if (level > 1)
                        level--;
                    sprintf(text, "Poziom %d", level);
                    SetAPen(reqWin->RPort, 20);
                    RectFill(reqWin->RPort, 94, 64, 112, 74);
                    drawMessage(gi, reqWin->RPort, text, 64, 64);
                }
                else if (gad->GadgetID == RID_CLOSE || gad->GadgetID == RID_OPT1 || gad->GadgetID == RID_OPT2)
                    done = TRUE;
            }
            else if (msg->Class == IDCMP_RAWKEY)
            {
                if (msg->Code == 0x40 || msg->Code == 0x44)
                {
                    /* OK */
                    done = TRUE;
                }
                else if (msg->Code == 0x45)
                {
                    /* Cancel */
                    done = TRUE;
                }
            }
            ReplyMsg((struct Message *)msg);
        }
    }

    RemoveGadget(reqWin, &prevGad);
    RemoveGadget(reqWin, &nextGad);
}

LONG dispMessageBox(struct gameInit *gi, STRPTR opt1, STRPTR opt2, LONG (*drawContents)(struct gameInit *gi, struct Window *r))
{
    struct Window *reqWin;
    struct Requester req;

    initReq(&gi->ri, &gi->wi, opt1, opt2);

    InitRequester(&req);
    Request(&req, gi->w);

    if (reqWin = openReqWindow(gi->w, 64, 64, 192, 128, gi->ri.gads))
    {
        drawContents(gi, reqWin);
        CloseWindow(reqWin);
    }

    EndRequest(&req, gi->w);
}

BOOL moveHero(struct Board *b, struct gameInit *gi, WORD dx, WORD dy)
{
    struct boardInfo *info = &b->info;

    WORD heroX = info->heroX, heroY = info->heroY;

    struct Cell *c = &b->board[heroY][heroX], *dest = c + (dy * BOARD_WIDTH) + dx;
    WORD tilex, tiley;
    WORD x, y;
    WORD snd = SAMPLE_DIG;

    SetSignal(0L, 1L << gi->copdata.signal);
    Wait(1L << gi->copdata.signal);

    if (dest->kind == WALL_KIND)
    {
        if (dest->subKind != DOOR_WALL || info->keys == 0)
        {
            return(FALSE);
        }
        snd = SAMPLE_FRUIT;
        info->keys--;

        playSample(gi->ioa, gi->samples + snd, SNDCHAN);

        dest->kind = FLOOR_KIND;
        dest->subKind = NORMAL_FLOOR;

        tilex = dest->subKind;
        tiley = 8 + dest->kind;

        x = heroX + dx;
        y = heroY + dy;

        BltBitMap(gi->gfx, tilex << 4, tiley << 4, gi->bm[0], x << 4, y << 4, 16, 16, 0xc0, 0xff, NULL);

        updateStatus(gi->w->RPort, b);

        return(FALSE);
    }

    if (dest->kind == ITEM_KIND)
    {
        if (dest->subKind == KEY_ITEM)
        {
            info->keys++;
            snd = SAMPLE_KEY;
        }
        else if (dest->subKind == FRUIT_ITEM)
        {
            snd = SAMPLE_FRUIT;
            info->points += 250;
        }
    }

    if (dest->kind == OBJECT_KIND)
    {
        if (dest->subKind != BOX_OBJECT)
        {
            return(FALSE);
        }

        struct Cell *past = dest + (dy * BOARD_WIDTH) + dx;

        if (past->kind != FLOOR_KIND)
        {
            return(FALSE);
        }

        snd = SAMPLE_BOX;

        past->kind = OBJECT_KIND;
        past->subKind = dest->subKind;

        tilex = past->subKind;
        tiley = 8 + past->kind;

        x = heroX + dx + dx;
        y = heroY + dy + dy;

        if (past->floor == FLAGSTONE_FLOOR)
        {
            tilex = PLACED_OBJECT;
            info->placed++;
        }

        if (dest->floor == FLAGSTONE_FLOOR)
        {
            info->placed--;
        }

        BltBitMap(gi->gfx, tilex << 4, tiley << 4, gi->bm[0], x << 4, y << 4, 16, 16, 0xc0, 0xff, NULL);
    }

    dest->kind = OBJECT_KIND;
    dest->subKind = c->subKind;

    c->kind = FLOOR_KIND;
    c->subKind = c->floor;

    tilex = c->subKind;
    tiley = 8 + c->kind;

    x = heroX;
    y = heroY;

    BltBitMap(gi->gfx, tilex << 4, tiley << 4, gi->bm[0], x << 4, y << 4, 16, 16, 0xc0, 0xff, NULL);

    tilex = dest->subKind;
    tiley = 8 + dest->kind;

    x += dx;
    y += dy;

    BltBitMap(gi->gfx, tilex << 4, tiley << 4, gi->bm[0], x << 4, y << 4, 16, 16, 0xc0, 0xff, NULL);

    info->heroX += dx;
    info->heroY += dy;

    if (snd != SAMPLE_DIG)
        playSample(gi->ioa, gi->samples + snd, SNDCHAN);

    updateStatus(gi->w->RPort, b);

    if (info->placed == info->boxes)
    {
        /* Komunikat o przej�ciu poziomu */
        dispMessageBox(gi, "OK", "Kontynuuj", standardMessage);
    }

    return(TRUE);
}

WORD testBoard(struct Board *b, struct gameInit *gi)
{
    struct boardInfo *info = &b->info;
    struct IntuiMessage *msg;
    BOOL done = FALSE;
    LONG prevSeconds = 0;
    enum
    {
        LEFT,
        RIGHT,
        UP,
        DOWN
    };
    UWORD dir = 0;
    WORD counter = 0;
    ULONG signals[2];

    BOOL pause = FALSE;

    signals[0] = 1L << gi->w->UserPort->mp_SigBit;
    signals[1] = 1L << gi->copdata.signal;

    while (msg = (struct IntuiMessage *)GetMsg(gi->w->UserPort))
    {
        ReplyMsg((struct Message *)msg);
    }

    RemoveGList(gi->w, gi->wi.gads, -1);
    initTexts(gi->wi.text);
    AddGList(gi->w, gi->wi.gads, -1, -1, NULL);
    RefreshGList(gi->wi.gads, gi->w, NULL, -1);

    while (!done)
    {
        ULONG result = Wait(signals[0]|signals[1]);

        if (result & signals[1])
        {
            if (info->placed == info->boxes)
            {
                return(RESULT_COMPLETED);
            }

            if (pause)
            {
                if (counter < SPEED)
                {
                    if (++counter == SPEED)
                    {
                        pause = FALSE;
                    }
                }
            }

            if (!pause && (dir & (1 << LEFT)) == (1 << LEFT))
            {
                moveHero(b, gi, -1, 0);
                pause = TRUE;
                counter = 0;
            }
            else if (!pause && (dir & (1 << RIGHT)) == (1 << RIGHT))
            {
                moveHero(b, gi, 1, 0);
                pause = TRUE;
                counter = 0;
            }
            else if (!pause && (dir & (1 << UP)) == (1 << UP))
            {
                moveHero(b, gi, 0, -1);
                pause = TRUE;
                counter = 0;
            }
            else if (!pause && (dir & (1 << DOWN)) == (1 << DOWN))
            {
                moveHero(b, gi, 0, 1);
                pause = TRUE;
                counter = 0;
            }
        }

        if (result & signals[0])
        {
            while (msg = (struct IntuiMessage *)GetMsg(gi->w->UserPort))
            {
                ULONG class = msg->Class;
                WORD code = msg->Code, mx = msg->MouseX, my = msg->MouseY;
                APTR iaddr = msg->IAddress;
                LONG seconds = msg->Seconds;

                ReplyMsg((struct Message *)msg);

                if (class == IDCMP_GADGETUP)
                {
                    struct Gadget *gad = (struct Gadget *)iaddr;

                    if (gad->GadgetID == GID_MENU1)
                    {
                        return(RESULT_START);
                    }
                    else if (gad->GadgetID == GID_MENU4)
                    {
                        dispMessageBox(gi, "OK", "Anuluj", levelSelection);
                    }
                    else if (gad->GadgetID == GID_MENU2)
                    {
                        return(RESULT_LOAD);
                    }
                    else if (gad->GadgetID == GID_MENU3)
                    {
                        return(RESULT_RESTART);
                    }
                    else if (gad->GadgetID == GID_MENU5)
                    {
                        return(RESULT_EDIT);
                    }
                }
                else if (class == IDCMP_RAWKEY)
                {
                    if (code == 0x45)
                    {
                        return(RESULT_QUIT);
                    }
                    else if (code == LEFT_KEY)
                    {
                        dir |= 1 << LEFT;
                    }
                    else if (code == RIGHT_KEY)
                    {
                        dir |= 1 << RIGHT;
                    }
                    else if (code == UP_KEY)
                    {
                        dir |= 1 << UP;
                    }
                    else if (code == DOWN_KEY)
                    {
                        dir |= 1 << DOWN;
                    }
                    else if (code == (LEFT_KEY|IECODE_UP_PREFIX))
                    {
                        dir &= ~(1 << LEFT);
                    }
                    else if (code == (RIGHT_KEY|IECODE_UP_PREFIX))
                    {
                        dir &= ~(1 << RIGHT);
                    }
                    else if (code == (UP_KEY|IECODE_UP_PREFIX))
                    {
                        dir &= ~(1 << UP);
                    }
                    else if (code == (DOWN_KEY|IECODE_UP_PREFIX))
                    {
                        dir &= ~(1 << DOWN);
                    }
                    prevSeconds = seconds;

                    if (!pause && (dir & (1 << LEFT)) == (1 << LEFT))
                    {
                        moveHero(b, gi, -1, 0);
                        pause = TRUE;
                        counter = 0;
                    }
                    else if (!pause && (dir & (1 << RIGHT)) == (1 << RIGHT))
                    {
                        moveHero(b, gi, 1, 0);
                        pause = TRUE;
                        counter = 0;
                    }
                    else if (!pause && (dir & (1 << UP)) == (1 << UP))
                    {
                        moveHero(b, gi, 0, -1);
                        pause = TRUE;
                        counter = 0;
                    }
                    else if (!pause && (dir & (1 << DOWN)) == (1 << DOWN))
                    {
                        moveHero(b, gi, 0, 1);
                        pause = TRUE;
                        counter = 0;
                    }

                    /* while (msg = (struct IntuiMessage *)GetMsg(gi->w->UserPort))
                    {
                        ReplyMsg((struct Message *)msg);
                    }*/
                }
                else if (class == IDCMP_REFRESHWINDOW)
                {
                    struct Rectangle bounds;
                    struct Window *w = gi->w;
                    struct copperData *cd = &gi->copdata;

                    BeginRefresh(w);
                    GetRPAttrs(w->RPort, RPTAG_DrawBounds, &bounds, TAG_DONE);

                    drawBoard(w->RPort, b, gi->bm[1], gi->gfx, bounds.MinX >> 4, bounds.MinY >> 4, bounds.MaxX >> 4, bounds.MaxY >> 4);

                    SetSignal(0L, 1L << cd->signal);
                    Wait(1L << cd->signal);

                    drawTile(gi->bm[1], bounds.MinX, bounds.MinY, gi->bm[0], bounds.MinX, bounds.MinY, bounds.MaxX - bounds.MinX + 1, bounds.MaxY - bounds.MinY + 1);
                    EndRefresh(w, TRUE);
                }
            }
        }
    }
}

WORD loadBoard(STRPTR name, struct Board *b, struct boardHeader *bh, struct gameState *gs)
{
    struct IFFHandle *iff;
    WORD result = LOAD_FAILURE;

    D(bug("Loading %s\n", name));

    if (iff = openIFF(name, IFFF_READ))
    {
        if (!PropChunk(iff, ID_MAGA, ID_NAGL))
        {
            if (!PropChunk(iff, ID_MAGA, ID_STAN))
            {
                if (!StopChunk(iff, ID_MAGA, ID_PLAN))
                {
                    if (!StopOnExit(iff, ID_MAGA, ID_FORM))
                    {
                        LONG err = ParseIFF(iff, IFFPARSE_SCAN);
                        if (err == 0 || err == IFFERR_EOC || err == IFFERR_EOF)
                        {
                            struct StoredProperty *sp;
                            if (gs && (sp = FindProp(iff, ID_MAGA, ID_STAN)))
                            {
                                *gs = *(struct gameState *)sp->sp_Data;
                                result = STATE_LOADED;
                                D(bug("State read: %d\n", gs->info.level));
                                b->info = gs->info;
                            }
                            if (sp = FindProp(iff, ID_MAGA, ID_NAGL))
                            {
                                struct boardHeader *temp = (struct boardHeader *)sp->sp_Data;
                                *bh = *(struct boardHeader *)sp->sp_Data;

                                if (sp->sp_Size > 6)
                                {
                                    struct boardInfo *info = (struct boardInfo *)(temp + 1);

                                    D(bug("Loading extra.\n"));

                                    b->info = *info;
                                    if (gs)
                                    {
                                        gs->info = b->info;
                                    }
                                }
                                else
                                {
                                    scanBoard(b);
                                    if (gs)
                                    {
                                        gs->info = b->info;
                                    }
                                }

                                if (ReadChunkBytes(iff, b->board, sizeof(b->board)) == sizeof(b->board))
                                {
                                    result = LEVEL_LOADED;
                                    D(bug("Game version: %d\n", bh->version));
                                }
                            }
                        }
                    }
                }
            }
        }
        closeIFF(iff);
    }
    return(result);
}

BOOL saveBoard(STRPTR name, struct Board *b, struct boardHeader *bh, struct gameState *gs)
{
    struct IFFHandle *iff;
    BOOL result = FALSE;

    if (iff = openIFF(name, IFFF_WRITE))
    {
        if (!PushChunk(iff, ID_MAGA, ID_FORM, IFFSIZE_UNKNOWN))
        {
            if (gs)
            {
                if (!PushChunk(iff, ID_MAGA, ID_STAN, sizeof(*gs)))
                {
                    gs->info = b->info;
                    if (WriteChunkBytes(iff, gs, sizeof(*gs)) == sizeof(*gs))
                    {
                        if (!PopChunk(iff))
                        {
                            result = TRUE;
                        }
                    }
                }
            }
            else
                result = TRUE;
            if (result && (!PushChunk(iff, ID_MAGA, ID_NAGL, sizeof(*bh) + sizeof(b->info))))
            {
                if (WriteChunkBytes(iff, bh, sizeof(*bh)) == sizeof(*bh))
                {
                    if (WriteChunkBytes(iff, &b->info, sizeof(b->info)) == sizeof(b->info))
                    {
                        if (!PopChunk(iff))
                        {
                            if (!PushChunk(iff, ID_MAGA, ID_PLAN, sizeof(b->board)))
                            {
                                if (WriteChunkBytes(iff, b->board, sizeof(b->board)) == sizeof(b->board))
                                {
                                    if (!PopChunk(iff))
                                    {
                                        if (!PopChunk(iff))
                                        {
                                            result = TRUE;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        closeIFF(iff);
    }
    return(FALSE);
}

void printLevel(struct Window *w, struct windowInfo *wi, struct gameState *gs)
{
    ULONG lock;

    lock = LockIBase(0);
    sprintf(wi->text[GID_MENU2].IText, "Edytor %03d", gs->info.level);
    UnlockIBase(lock);

    RefreshGList(wi->gads + GID_MENU2, w, NULL, 1);
}

WORD editBoard(struct gameInit *gi)
{
    struct Window *w = gi->w;
    struct copperData *cd = &gi->copdata;
    struct BitMap **bm = gi->bm;
    struct BitMap *gfx = gi->gfx;
    struct windowInfo *wi = &gi->wi;
    struct reqInfo *ri = &gi->ri;

    ULONG signals[ 2 ] = { 0 };
    BOOL done = FALSE;
    WORD tilex = 0, tiley = 1 + 8;
    struct Requester req;

    BOOL paint = FALSE;
    WORD oldx = -1, oldy = -1;

    ULONG total;

    signals[0] = 1L << w->UserPort->mp_SigBit;
    signals[1] = 1L << cd->signal;

    total = signals[0]|signals[1];

    RemoveGList(w, wi->gads, -1);
    initEditTexts(wi->text);
    AddGList(w, wi->gads, -1, -1, NULL);
    RefreshGList(wi->gads, w, NULL, -1);

    InitRequester(&req);

    while (!done)
    {
        ULONG result = Wait(signals[0]);

        if (result & signals[0])
        {
            struct IntuiMessage *msg;

            while (msg = (struct IntuiMessage *)GetMsg(w->UserPort))
            {
                ULONG class = msg->Class;
                WORD code = msg->Code, mx = msg->MouseX, my = msg->MouseY;
                APTR iaddr = msg->IAddress;

                ReplyMsg((struct Message *)msg);

                if (class == IDCMP_GADGETUP)
                {
                    struct Gadget *gad = (struct Gadget *)iaddr;

                    if (gad->GadgetID == GID_MENU1)
                    {
                        struct Window *menu;

                        Request(&req, w);

                        if (menu = openMenuWindow(w, 0, 64, 80, NULL))
                        {
                            struct IntuiMessage *im;

                            BltBitMap(gfx, 0, 32, bm[1], menu->LeftEdge, menu->TopEdge, 64, 16, 0xc0, 0xff, NULL);

                            BltBitMap(gfx, 0, 128, bm[1], menu->LeftEdge, menu->TopEdge + 16, 64, 64, 0xc0, 0xff, NULL);

                            SetSignal(0L, 1L << cd->signal);
                            Wait(1L << cd->signal);
                            drawTile(bm[1], menu->LeftEdge, menu->TopEdge, bm[0], menu->LeftEdge, menu->TopEdge, menu->Width, menu->Height);
                            Move(menu->RPort, 4, 4 + menu->RPort->Font->tf_Baseline);
                            SetAPen(menu->RPort, 4);
                            SetDrMd(menu->RPort, JAM1);
                            Text(menu->RPort, "Kafelek", 7);

                            SetAPen(menu->RPort, 10);
                            Move(menu->RPort, 0, 15);
                            Draw(menu->RPort, 0, menu->Height - 1);
                            SetAPen(menu->RPort, 0);
                            Draw(menu->RPort, menu->Width - 1, menu->Height - 1);
                            Draw(menu->RPort, menu->Width - 1, 16);

                            WaitPort(menu->UserPort);

                            while (im = (struct IntuiMessage *)GetMsg(menu->UserPort))
                            {
                                WORD mx = im->MouseX, my = im->MouseY;
                                if (my >= 16)
                                {
                                    tiley = 8 + (my >> 4) - 1;
                                    tilex = mx >> 4;
                                }
                                ReplyMsg((struct Message *)im);
                            }

                            CloseWindow(menu);
                        }
                        EndRequest(&req, w);
                    }
                    else if (gad->GadgetID == GID_MENU2)
                    {
                        UBYTE name[NAME];

                        sprintf(name, "Data1/Levels/Level%03d.lev", gi->board.info.level);

                        if (loadBoard(name, &gi->board, &gi->header, &gi->state))
                        {
                            drawBoard(w->RPort, &gi->board, gi->bm[1], gi->gfx, 0, 1, 19, 14);

                            SetSignal(0L, 1L << gi->copdata.signal);
                            Wait(1L << gi->copdata.signal);
                            drawTile(gi->bm[1], 0, 0, gi->bm[0], 0, 0, 320, 240);
                        }
                    }
                    else if (gad->GadgetID == GID_MENU3)
                    {
                        UBYTE name[NAME];

                        sprintf(name, "Data1/Levels/Level%03d.lev", gi->board.info.level);

                        if (saveBoard(name, &gi->board, &gi->header, &gi->state))
                        {
                        }
                    }
                    else if (gad->GadgetID == GID_MENU4)
                    {
                        return(RESULT_QUIT);
                    }
                    else if (gad->GadgetID == GID_MENU5)
                    {
                        return(RESULT_PLAY);
                    }
                }
                else if (class == IDCMP_REFRESHWINDOW)
                {
                    struct Rectangle bounds;

                    BeginRefresh(w);
                    GetRPAttrs(w->RPort, RPTAG_DrawBounds, &bounds, TAG_DONE);

                    drawBoard(w->RPort, &gi->board, bm[1], gfx, bounds.MinX >> 4, bounds.MinY >> 4, bounds.MaxX >> 4, bounds.MaxY >> 4);

                    SetSignal(0L, 1L << cd->signal);
                    Wait(1L << cd->signal);

                    drawTile(bm[1], bounds.MinX, bounds.MinY, bm[0], bounds.MinX, bounds.MinY, bounds.MaxX - bounds.MinX + 1, bounds.MaxY - bounds.MinY + 1);
                    EndRefresh(w, TRUE);
                }
                else if (class == IDCMP_MOUSEBUTTONS)
                {
                    if (code == IECODE_LBUTTON)
                    {
                        if (my < 240)
                        {
                            struct Cell *c = &gi->board.board[my >> 4][mx >> 4];
                            BltBitMap(gfx, tilex << 4, tiley << 4, bm[0], mx & 0xfff0, my & 0xfff0, 16, 16, 0xc0, 0xff, NULL);
                            c->kind = tiley - 8;
                            c->subKind = tilex;

                            if (c->kind == FLOOR_KIND)
                                c->floor = c->subKind;
                            else
                                c->floor = NORMAL_FLOOR;

                            paint = TRUE;
                            oldx = mx >> 4;
                            oldy = my >> 4;
                        }
                    }
                    else if (code == (IECODE_LBUTTON|IECODE_UP_PREFIX))
                    {
                        paint = FALSE;
                    }
                }
                else if (class == IDCMP_MOUSEMOVE)
                {
                    if (paint && my < 240 && (oldx != (mx >> 4) || oldy != (my >> 4)))
                    {
                        struct Cell *c = &gi->board.board[my >> 4][mx >> 4];
                        c->kind = tiley - 8;
                        c->subKind = tilex;

                        if (c->kind == FLOOR_KIND)
                            c->floor = c->subKind;
                        else
                            c->floor = NORMAL_FLOOR;

                        BltBitMap(gfx, tilex << 4, tiley << 4, bm[0], mx & 0xfff0, my & 0xfff0, 16, 16, 0xc0, 0xff, NULL);
                        oldx = mx >> 4;
                        oldy = my >> 4;
                    }
                }
            }
        }
    }

    saveBoard("T:Temp.bak", &gi->board, &gi->header, NULL);
    return(RESULT_QUIT);
}

void convert(struct gameInit *initData)
{
    UBYTE name[NAME];
    WORD lev;

    for (lev = 6; lev <= 6; lev++)
    {
        sprintf(name, "Data1/Levels/Level%03d.lev", lev);
        initGameBoard(initData, name);

        initData->board.info.level = lev;

        saveBoard(name, &initData->board, &initData->header, NULL);
    }
}


int main(void)
{
    struct gameInit *initData;

    if (initData = AllocMem(sizeof(*initData), MEMF_PUBLIC|MEMF_CLEAR))
    {
        if (initGame(initData))
        {
            BOOL done = FALSE;
            WORD result;
            UBYTE name[NAME];
            WORD level = initData->state.info.level = 1;
            BOOL levelLoaded = FALSE;

            if (loadBoard("States/State.iff", &initData->board, &initData->header, &initData->state) == LEVEL_LOADED)
            {
                D(bug("Load state OK\n"));
                level = initData->state.info.level;
                levelLoaded = TRUE;

                drawBoard(initData->w->RPort, &initData->board, initData->bm[1], initData->gfx, 0, 1, 19, 14);

                SetSignal(0L, 1L << initData->copdata.signal);
                Wait(1L << initData->copdata.signal);
                drawTile(initData->bm[1], 0, 0, initData->bm[0], 0, 0, 320, 240);
            }
            else
                D(bug("Load state failure\n"));


            while (!done)
            {
                globLevel = level;
                D(bug("Loop... %d (%d)\n", level, levelLoaded));
                sprintf(name, "Data1/Levels/Level%03d.lev", level);
                if (!levelLoaded)
                {
                    D(bug("Loading new level %d\n", level));
                    levelLoaded = initGameBoard(initData, name);
                }

                if (levelLoaded)
                {
                    result = testBoard(&initData->board, initData);

                    levelLoaded = FALSE;
                    if (result == RESULT_QUIT)
                        done = TRUE;
                    else if (result == RESULT_COMPLETED)
                    {
                        level = initData->state.info.level = initData->board.info.level + 1;
                        saveBoard("States/State.iff", &initData->board, &initData->header, &initData->state);
                    }
                    else if (result == RESULT_LOAD || result == RESULT_PLAY)
                    {
                        if (loadBoard("States/State.iff", &initData->board, &initData->header, &initData->state) == LEVEL_LOADED)
                        {
                            level = initData->state.info.level;
                            levelLoaded = TRUE;

                            drawBoard(initData->w->RPort, &initData->board, initData->bm[1], initData->gfx, 0, 1, 19, 14);

                            SetSignal(0L, 1L << initData->copdata.signal);
                            Wait(1L << initData->copdata.signal);
                            drawTile(initData->bm[1], 0, 0, initData->bm[0], 0, 0, 320, 240);
                        }
                    }
                    else if (result == RESULT_START)
                        level = initData->state.info.level = 1;
                    else if (result == RESULT_EDIT)
                        if ((result = editBoard(initData)) == RESULT_QUIT)
                            done = TRUE;
                }
                else
                    done = TRUE;
            }

            saveBoard("States/State.iff", &initData->board, &initData->header, &initData->state);


            freeGame(initData);
        }
        FreeMem(initData, sizeof(*initData));
    }
    return(RETURN_OK);
}
