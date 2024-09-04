/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2007, 2008, 2009 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file image.h
 */

#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <SDL.h>

#include "dmtx.h"

#define IMAGE_NO_ERROR 0
#define IMAGE_ERROR 1
#define IMAGE_NOT_PNG 2

typedef enum
{
    ColorWhite = 0x01 << 0,
    ColorRed = 0x01 << 1,
    ColorGreen = 0x01 << 2,
    ColorBlue = 0x01 << 3,
    ColorYellow = 0x01 << 4
} ColorEnum;

/*void captureImage(DmtxImage *img, DmtxImage *imgTmp);*/
SDL_Surface *loadTextureImage();
unsigned int getFormat(SDL_Surface *bmp);
void plotPoint(DmtxImage *img, float rowFloat, float colFloat, int targetColor);
int clampRGB(float color);

#endif
