/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2007, 2008, 2009 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file image.c
 */

#include "image.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#    include <gl/GL.h>
#    include <gl/GLU.h>
#else
#    include <OpenGL/gl.h>
#    include <OpenGL/glu.h>
#endif

#include "dmtx.h"
#include "rotate_test.h"

#ifndef max
#    define max(X, Y) (((X) > (Y)) ? (X) : (Y))
#endif

SDL_Surface *flipSurfaceVertically(SDL_Surface *inputSurface)
{
    if (inputSurface == NULL) {
        return NULL;
    }

    // 创建新的表面，用于存储翻转后的图像
    SDL_Surface *flippedSurface = SDL_CreateRGBSurface(
        0, inputSurface->w, inputSurface->h, inputSurface->format->BitsPerPixel, inputSurface->format->Rmask,
        inputSurface->format->Gmask, inputSurface->format->Bmask, inputSurface->format->Amask);

    if (flippedSurface == NULL) {
        SDL_Log("Unable to create flipped surface: %s", SDL_GetError());
        return NULL;
    }

    // 锁定原表面和新表面的像素，以便直接访问
    SDL_LockSurface(inputSurface);
    SDL_LockSurface(flippedSurface);

    // 遍历每一行，从下到上复制像素
    for (int y = 0; y < inputSurface->h; y++) {
        unsigned char *src = (unsigned char *)inputSurface->pixels + (inputSurface->pitch * (inputSurface->h - y - 1));
        unsigned char *dst = (unsigned char *)flippedSurface->pixels + (flippedSurface->pitch * y);
        memcpy(dst, src, inputSurface->pitch);
    }

    // 解锁表面
    SDL_UnlockSurface(inputSurface);
    SDL_UnlockSurface(flippedSurface);

    return flippedSurface;
}

/**
 *
 *
 */
SDL_Surface *loadTextureImage()
{
    SDL_Surface *bmp = NULL;
    char filepath[128];

    strcpy(filepath, "images/");
    strcat(filepath, gFilename[gFileIdx]);
    SDL_Log("Opening %s\n", filepath);

    SDL_Surface *image = SDL_LoadBMP(filepath);
    image = SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_RGB24, 0);
    bmp = flipSurfaceVertically(image);
    if (bmp) {
        // 你可以在这里使用 flippedSurface 进行渲染或其他操作
        SDL_FreeSurface(image);
    }
    assert(bmp != NULL);

    gFileIdx++;
    if (gFileIdx == gFileCount) {
        gFileIdx = 0;
    }

    /* Set up texture */
    glGenTextures(1, &barcodeTexture);
    glBindTexture(GL_TEXTURE_2D, barcodeTexture);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

/* Read barcode image */
#ifdef _WIN32
    int err = gluBuild2DMipmaps(GL_TEXTURE_2D, bmp->format->BytesPerPixel, bmp->w, bmp->h, getFormat(bmp),
                                GL_UNSIGNED_BYTE, bmp->pixels);
    if (err) {
        SDL_Log((const char *)gluErrorStringWIN(err));
    }
#else
    int err = gluBuild2DMipmaps(GL_TEXTURE_2D, bmp->format->BytesPerPixel, bmp->w, bmp->h, getFormat(bmp),
                                GL_UNSIGNED_BYTE, bmp->pixels);
    if (err) {
        SDL_Log(gluErrorString(err));
    }
#endif

    /* Create the barcode list */
    barcodeList = glGenLists(1);
    glNewList(barcodeList, GL_COMPILE);
    DrawBarCode();
    glEndList();

    return bmp;
}

unsigned int getFormat(SDL_Surface *bmp)
{
    if (bmp->format->BytesPerPixel == 4) {
        if (bmp->format->Rmask == 0x000000ff) {
            return GL_RGBA;
        } else {
            return GL_BGRA;
        }
    } else if (bmp->format->BytesPerPixel == 3) {
        if (bmp->format->Rmask == 0x000000ff) {
            return GL_RGB;
        } else {
            return GL_BGR;
        }
    } else if (bmp->format->BytesPerPixel == 2) {
        return GL_RGB;
    } else if (bmp->format->BytesPerPixel == 1) {
        return GL_RED;
    }

    return GL_RGB;
}

/**
 *
 *
 */
void plotPoint(DmtxImage *img, float rowFloat, float colFloat, int targetColor)
{
    int i, row, col;
    float xFloat, yFloat;
    int offset[4];
    int color[4];

    row = (int)rowFloat;
    col = (int)colFloat;

    xFloat = colFloat - col;
    yFloat = rowFloat - row;

    offset[0] = row * img->width + col;
    offset[1] = row * img->width + (col + 1);
    offset[2] = (row + 1) * img->width + col;
    offset[3] = (row + 1) * img->width + (col + 1);

    color[0] = clampRGB(255.0F * ((1.0F - xFloat) * (1.0F - yFloat)));
    color[1] = clampRGB(255.0F * (xFloat * (1.0F - yFloat)));
    color[2] = clampRGB(255.0F * ((1.0F - xFloat) * yFloat));
    color[3] = clampRGB(255.0F * (xFloat * yFloat));

    for (i = 0; i < 4; i++) {
        if ((i == 1 || i == 3) && col + 1 > 319) {
            continue;
        }
        if ((i == 2 || i == 3) && row + 1 > 319) {
            continue;
        }

        if (targetColor & (ColorWhite | ColorRed | ColorYellow)) {
            img->pxl[offset[i] * 3 + 0] = max(img->pxl[offset[i] * 3 + 0], color[i]);
        }

        if (targetColor & (ColorWhite | ColorGreen | ColorYellow)) {
            img->pxl[offset[i] * 3 + 1] = max(img->pxl[offset[i] * 3 + 1], color[i]);
        }

        if (targetColor & (ColorWhite | ColorBlue)) {
            img->pxl[offset[i] * 3 + 2] = max(img->pxl[offset[i] * 3 + 2], color[i]);
        }
    }
}

/**
 *
 *
 */
int clampRGB(float color)
{
    if (color < 0.0) {
        return 0;
    } else if (color > 255.0) {
        return 255;
    } else {
        return (int)(color + 0.5);
    }
}
