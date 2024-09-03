/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2007, 2008, 2009 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file rotate_test.c
 */

#include "rotate_test.h"

#include "image.h"

GLfloat view_rotx = 0.0, view_roty = 0.0, view_rotz = 0.0;
GLfloat angle = 0.0;

GLuint barcodeTexture;
GLint barcodeList;

DmtxImage *gImage = NULL;
SDL_Surface *texturePxl = NULL;
unsigned char *capturePxl = NULL;
unsigned char *passOnePxl = NULL;
unsigned char *passTwoPxl = NULL;

char *gFilename[] = {"test_image18.bmp", "test_image16.bmp", "test_image17.bmp", "test_image01.bmp", "test_image05.bmp",
                     "test_image06.bmp", "test_image07.bmp", "test_image12.bmp", "test_image13.bmp", "test_image08.bmp",
                     "test_image09.bmp", "test_image10.bmp", "test_image04.bmp", "test_image11.bmp", "test_image02.bmp",
                     "test_image03.bmp", "test_image14.bmp", "test_image15.bmp"};
int gFileIdx = 0;
int gFileCount = 18;

int main(int argc, char *argv[])
{
    int i = 0;
    int count = 0;
    int done = 0;
    SDL_Event event;
    SDL_Surface *screen = NULL;
    unsigned char outputString[1024];
    DmtxDecode *dec = NULL;
    DmtxRegion *reg = NULL;
    DmtxMessage *msg = NULL;
    DmtxTime timeout;

    /* Initialize display window */
    screen = initDisplay();

    /* Load input image to DmtxImage */
    texturePxl = loadTextureImage();
    assert(texturePxl != NULL);

    capturePxl = (unsigned char *)malloc(texturePxl->w * texturePxl->h * texturePxl->format->BytesPerPixel);
    assert(capturePxl != NULL);

    passOnePxl = (unsigned char *)malloc(texturePxl->w * texturePxl->h * texturePxl->format->BytesPerPixel);
    assert(passOnePxl != NULL);

    passTwoPxl = (unsigned char *)malloc(texturePxl->w * texturePxl->h * texturePxl->format->BytesPerPixel);
    assert(passTwoPxl != NULL);

    // done = 0;
    while (!done) {
        SDL_Delay(50);

        while (SDL_PollEvent(&event)) {
            done = HandleEvent(&event, screen);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        DrawGeneratedImage(screen);

        memset(passOnePxl, 0x00, texturePxl->w * texturePxl->h * texturePxl->format->BytesPerPixel);
        memset(passTwoPxl, 0x00, texturePxl->w * texturePxl->h * texturePxl->format->BytesPerPixel);

        /* Capture screenshot of generated image */
        glReadPixels(2, 324, texturePxl->w, texturePxl->h, getFormat(texturePxl), GL_UNSIGNED_BYTE, capturePxl);
        gImage = dmtxImageCreate(capturePxl, texturePxl->w, texturePxl->h, DmtxPack24bppRGB);
        assert(gImage != NULL);

        /* Pixels from glReadPixels are Y-flipped according to libdmtx */
        dmtxImageSetProp(gImage, DmtxPropImageFlip, DmtxFlipY);

        /* Start fresh scan */
        dec = dmtxDecodeCreate(gImage, 1);
        assert(dec != NULL);

        for (;;) {
            timeout = dmtxTimeAdd(dmtxTimeNow(), 500);

            reg = dmtxRegionFindNext(dec, &timeout);
            if (reg != NULL) {
                msg = dmtxDecodeMatrixRegion(dec, reg, DmtxUndefined);
                if (msg != NULL) {
                    fputs("output: \"", stdout);
                    fwrite(msg->output, sizeof(unsigned char), msg->outputIdx, stdout);
                    fputs("\"\n", stdout);
                    dmtxMessageDestroy(&msg);
                }
                dmtxRegionDestroy(&reg);
            }
            break;
        }

        dmtxDecodeDestroy(&dec);
        dmtxImageDestroy(&gImage);

        DrawBorders(screen);
        DrawPane2(screen, passOnePxl);
        DrawPane4(screen, passTwoPxl);

        SDL_GL_SwapWindow(window);
    }

    free(passTwoPxl);
    free(passOnePxl);
    free(capturePxl);
    SDL_FreeSurface(texturePxl);

    return 0;
}
