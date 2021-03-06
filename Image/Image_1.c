
/* Image.c: Wycinanie obraz�w/Bob�w z bitmap */

/* $Log:	Image.c,v $
 * Revision 1.1  12/.0/.1  .1:.2:.1  Robert
 * Initial revision
 *  */

#include <exec/memory.h>
#include <intuition/intuition.h>
#include <graphics/gels.h>
#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>

#include "Image.h"

#define WordWidth(w) (((w)+15)>>4)

/* allocImageData() - Zaalokuj odpowiedni� pami�� na dane obrazu */

UWORD *allocImageData(UWORD width, UWORD height, UBYTE depth)
{
	UWORD *data;
	LONG planeSize = RASSIZE(width, height);
	LONG fullSize = planeSize * depth;

	if (data = (UWORD *)AllocVec(fullSize, MEMF_CHIP))
	{
		return(data);
	}
	return(NULL);
}

/* initImage() - Zainicjuj obraz */

void initImage(struct Image *img, UWORD *data, UWORD width, UWORD height, UBYTE depth, UBYTE planePick, UBYTE planeOnOff)
{
	img->Width = width;
	img->Height = height;
	img->LeftEdge = 0;
	img->TopEdge = 0;
	img->Depth = depth;
	img->PlanePick = planePick;
	img->PlaneOnOff = planeOnOff;
	img->ImageData = data;
	img->NextImage = NULL;
}

/* initBob() - Zainicjuj Boba */

BOOL initBob(struct VSprite *vs, struct Bob *bob, WORD *data, UWORD width, UWORD height, UBYTE depth, UBYTE planePick, UBYTE planeOnOff)
{
	vs->Width = WordWidth(width);
	vs->Height = height;
	vs->Depth = depth;
	vs->ImageData = data;
	vs->VSBob = bob;
	vs->PlanePick = planePick;
	vs->PlaneOnOff = planeOnOff;

	if (bob->ImageShadow = (WORD *)AllocRaster(width, height))
	{
		bob->BobVSprite = vs;
		return(TRUE);
	}
	return(FALSE);
}

/* freeBob() - Zwolnij mask�Boba */

void freeBob(struct Bob *bob)
{
	FreeRaster((PLANEPTR)bob->ImageShadow, bob->BobVSprite->Width << 4, bob->BobVSprite->Height);
}

/* cutImageFromBitMap() - Wytnij fragment bitmapy */

void cutImageFromBitMap(UWORD *data, struct BitMap *bm, UWORD left, UWORD top, UWORD width, UWORD height, UBYTE depth)
{
	LONG planeSize = RASSIZE(width, height);
	struct BitMap auxbm;
	UBYTE i;

	InitBitMap(&auxbm, depth, width, height);

	auxbm.Planes[0] = (PLANEPTR)data;

	for (i = 1; i < depth; i++)
	{
		auxbm.Planes[i] = auxbm.Planes[i - 1] + planeSize;
	}

	BltBitMap(bm, left, top, &auxbm, 0, 0, width, height, 0xc0, 0xff, NULL);
}
