/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2007, 2008, 2009 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file rotate_test.h
 */

#ifndef __SCANDEMO_H__
#define __SCANDEMO_H__

#include <SDL.h>
#include <SDL_opengl.h>
#include <assert.h>
#include <dmtx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"

extern GLfloat view_rotx;
extern GLfloat view_roty;
extern GLfloat view_rotz;
extern GLfloat angle;

extern GLuint barcodeTexture;
extern GLint barcodeList;

extern SDL_Window *window;
extern SDL_Surface *screen;

extern DmtxImage *gImage;
extern SDL_Surface *texturePxl;
extern unsigned char *capturePxl;
extern unsigned char *passOnePxl;
extern unsigned char *passTwoPxl;

extern char *gFilename[];
extern int gFileIdx;
extern int gFileCount;

#endif
