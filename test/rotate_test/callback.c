/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2007, 2008, 2009 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file callback.c
 */

#include "callback.h"

#include <stdlib.h>

#include "display.h"
#include "dmtx.h"
#include "image.h"
#include "rotate_test.h"

#define DMTX_DISPLAY_SQUARE 1
#define DMTX_DISPLAY_POINT 2
#define DMTX_DISPLAY_CIRCLE 3

/**
 *
 *
 */
void BuildMatrixCallback2(DmtxRegion *region)
{
    int i, j;
    int offset;
    float scale = 1.0F / 200.0F;
    DmtxVector2 point;
    DmtxMatrix3 m0, m1, mInv;
    int rgb[3];

    dmtxMatrix3Translate(m0, -(320 - 200) / 2.0, -(320 - 200) / 2.0);
    dmtxMatrix3Scale(m1, scale, scale);
    dmtxMatrix3Multiply(mInv, m0, m1);
    dmtxMatrix3MultiplyBy(mInv, region->fit2raw);

    glDisable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT, GL_LINE);

    for (i = 0; i < 320; i++) {
        for (j = 0; j < 320; j++) {
            point.x = j;
            point.y = i;
            dmtxMatrix3VMultiplyBy(&point, mInv);
            dmtxImageGetPixelValue(gImage, (int)(point.x + 0.5), (int)(point.y + 0.5), 0, &rgb[0]);
            dmtxImageGetPixelValue(gImage, (int)(point.x + 0.5), (int)(point.y + 0.5), 1, &rgb[1]);
            dmtxImageGetPixelValue(gImage, (int)(point.x + 0.5), (int)(point.y + 0.5), 2, &rgb[2]);

            offset = (320 * i + j) * 3;
            passTwoPxl[offset + 0] = rgb[0];
            passTwoPxl[offset + 1] = rgb[1];
            passTwoPxl[offset + 2] = rgb[2];
            /*       dmtxPixelFromColor3(passTwoPxl[i*320+j], &clr); */
        }
    }

    DrawPane3(NULL, passTwoPxl);

    glViewport(646, 324, 320, 320);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-0.5, 319.5, -0.5, 319.5, -1.0, 10.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(0.0F, 1.0F, 0.0F);
    glBegin(GL_QUADS);
    glVertex2f(60.0F, 60.0F);
    glVertex2f(260.0F, 60.0F);
    glVertex2f(260.0F, 260.0F);
    glVertex2f(60.0F, 260.0F);
    glEnd();
}

/**
 *
 *
 */
void BuildMatrixCallback3(DmtxMatrix3 mChainInv)
{
    int i, j;
    int offset;
    float scale = 1.0F / 100.0F;
    DmtxVector2 point;
    DmtxMatrix3 m0, m1, mInv;
    int rgb[3];

    dmtxMatrix3Scale(m0, scale, scale);
    dmtxMatrix3Translate(m1, -(320 - 200) / 2.0, -(320 - 200) / 2.0);

    dmtxMatrix3Multiply(mInv, m1, m0);
    dmtxMatrix3MultiplyBy(mInv, mChainInv);

    glDisable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT, GL_LINE);

    for (i = 0; i < 320; i++) {
        for (j = 0; j < 320; j++) {
            point.x = j;
            point.y = i;
            dmtxMatrix3VMultiplyBy(&point, mInv);
            dmtxImageGetPixelValue(gImage, (int)(point.x + 0.5), (int)(point.y + 0.5), 0, &rgb[0]);
            dmtxImageGetPixelValue(gImage, (int)(point.x + 0.5), (int)(point.y + 0.5), 1, &rgb[1]);
            dmtxImageGetPixelValue(gImage, (int)(point.x + 0.5), (int)(point.y + 0.5), 2, &rgb[2]);

            offset = (320 * i + j) * 3;
            passTwoPxl[offset + 0] = rgb[0];
            passTwoPxl[offset + 1] = rgb[1];
            passTwoPxl[offset + 2] = rgb[2];
        }
    }

    DrawPane4(NULL, passTwoPxl);

    glViewport(2, 2, 320, 320);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-0.5, 319.5, -0.5, 319.5, -1.0, 10.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(0.0F, 1.0F, 0.0F);
    glBegin(GL_QUADS);
    glVertex2f(60.0F, 60.0F);
    glVertex2f(160.0F, 60.0F);
    glVertex2f(160.0F, 160.0F);
    glVertex2f(60.0F, 160.0F);
    /**
       glVertex2f( 60.0,  60.0);
       glVertex2f(260.0,  60.0);
       glVertex2f(260.0, 260.0);
       glVertex2f( 60.0, 260.0);
    */
    glEnd();
}

/**
 *
 *
 */
void BuildMatrixCallback4(DmtxMatrix3 mChainInv)
{
    int i, j;
    int offset;
    float scale = 1.0F / 200.0F;
    DmtxVector2 point;
    DmtxMatrix3 m0, m1, mInv;
    int rgb[3];

    dmtxMatrix3Scale(m0, scale, scale);
    dmtxMatrix3Translate(m1, -(320 - 200) / 2.0, -(320 - 200) / 2.0);

    dmtxMatrix3Multiply(mInv, m1, m0);
    dmtxMatrix3MultiplyBy(mInv, mChainInv);

    glDisable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT, GL_LINE);

    for (i = 0; i < 320; i++) {
        for (j = 0; j < 320; j++) {
            point.x = j;
            point.y = i;
            dmtxMatrix3VMultiplyBy(&point, mInv);
            dmtxImageGetPixelValue(gImage, (int)(point.x + 0.5), (int)(point.y + 0.5), 0, &rgb[0]);
            dmtxImageGetPixelValue(gImage, (int)(point.x + 0.5), (int)(point.y + 0.5), 1, &rgb[1]);
            dmtxImageGetPixelValue(gImage, (int)(point.x + 0.5), (int)(point.y + 0.5), 2, &rgb[2]);

            offset = (320 * i + j) * 3;
            passTwoPxl[offset + 0] = rgb[0];
            passTwoPxl[offset + 1] = rgb[1];
            passTwoPxl[offset + 2] = rgb[2];
        }
    }

    DrawPane5(NULL, passTwoPxl);

    glViewport(324, 2, 320, 320);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-0.5, 319.5, -0.5, 319.5, -1.0, 10.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(0.0F, 1.0F, 0.0F);
    glBegin(GL_QUADS);
    glVertex2f(60.0F, 60.0F);
    glVertex2f(260.0F, 60.0F);
    glVertex2f(260.0F, 260.0F);
    glVertex2f(60.0F, 260.0F);
    glEnd();
}

void hsv2rgb(float h, float s, float v)
{
    double c, h_, x;
    double r, g, b;

    h = fmod(h, 360.0F);

    c = s;
    h_ = h / 60;
    x = c * (1 - fabs(fmod(h_, 2) - 1.0));
    r = g = b = v - c;

    if (h_ < 1) {
        r += c;
        g += x;
    } else if (h_ < 2) {
        r += x;
        g += c;
    } else if (h_ < 3) {
        g += c;
        b += x;
    } else if (h_ < 4) {
        g += x;
        b += c;
    } else if (h_ < 5) {
        r += x;
        b += c;
    } else if (h_ < 6) {
        r += c;
        b += x;
    }

    glColor3f((float)r, (float)g, (float)b);
}

/**
 *
 *
 */
void PlotPointCallback(DmtxPixelLoc loc, float colorHue, int paneNbr, int dispType)
{
    DmtxVector2 point;

    point.x = loc.x;
    point.y = loc.y;

    switch (paneNbr) {
        case 1:
            glViewport(2, 324, 320, 320);
            break;
        case 2:
            glViewport(324, 324, 320, 320);
            /*       image = passOnePxl; */
            break;
        case 3:
            glViewport(646, 324, 320, 320);
            break;
        case 4:
            glViewport(2, 2, 320, 320);
            break;
        case 5:
            glViewport(324, 2, 320, 320);
            break;
        case 6:
            glViewport(646, 2, 320, 320);
            break;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-0.5, 319.5, -0.5, 319.5, -1.0, 10.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT, GL_LINE);

    hsv2rgb(colorHue, 1.0F, 1.0F);

    if (dispType == DMTX_DISPLAY_SQUARE) {
        glBegin(GL_QUADS);
        glVertex2f((float)point.x - 3, (float)point.y - 3);
        glVertex2f((float)point.x + 3, (float)point.y - 3);
        glVertex2f((float)point.x + 3, (float)point.y + 3);
        glVertex2f((float)point.x - 3, (float)point.y + 3);
        glEnd();
    } else if (dispType == DMTX_DISPLAY_POINT) {
        int x = (int)(point.x + 0.5);
        int y = (int)(point.y + 0.5);

        glBegin(GL_POINTS);
        glVertex2f((float)x, (float)y);
        glEnd();
    }
}

/**
 *
 *
 */
void XfrmPlotPointCallback(DmtxVector2 point, DmtxMatrix3 xfrm, int paneNbr, int dispType)
{
    float scale = 100.0F;
    DmtxMatrix3 m, m0, m1;

    if (xfrm != NULL) {
        dmtxMatrix3Copy(m, xfrm);
    } else {
        dmtxMatrix3Identity(m);
    }

    dmtxMatrix3Scale(m0, scale, scale);
    dmtxMatrix3Translate(m1, (320 - 200) / 2.0, (320 - 200) / 2.0);
    dmtxMatrix3MultiplyBy(m, m0);
    dmtxMatrix3MultiplyBy(m, m1);

    dmtxMatrix3VMultiplyBy(&point, m);
    /* PlotPointCallback(point, 3, paneNbr, dispType); */
}

/**
 *
 *
 */
void PlotModuleCallback(DmtxDecode *info, DmtxRegion *region, int row, int col, int color)
{
    int modSize, halfModsize, padSize;
    /* float t; */

    /* Adjust for addition of finder bar */
    row++;
    col++;

    halfModsize = (int)(100.0 / (region->mappingCols + 2) + 0.5); /* Because 100 == 200/2 */
    modSize = 2 * halfModsize;
    padSize = (320 - ((region->mappingCols + 2) * modSize)) / 2;

    /* Set for 6th pane */
    DrawPaneBorder(645, 1, 322, 322);
    glRasterPos2i(1, 1);

    glPolygonMode(GL_FRONT, GL_FILL);

    /* Clamp color to extreme foreground or background color */
    /* t = dmtxDistanceAlongRay3(&(region->gradient.ray), &color);
       t = (t < region->gradient.tMid) ? region->gradient.tMin : region->gradient.tMax;
       dmtxPointAlongRay3(&color, &(region->gradient.ray), t); */

    if (color == 1) {
        glColor3f(0.0F, 0.0F, 0.0F);
    } else {
        glColor3f(1.0F, 1.0F, 1.0F);
    }

    glBegin(GL_QUADS);
    glVertex2f((float)(modSize * (col + 0.5) + padSize - halfModsize),
               (float)(modSize * (row + 0.5) + padSize - halfModsize));
    glVertex2f((float)(modSize * (col + 0.5) + padSize + halfModsize),
               (float)(modSize * (row + 0.5) + padSize - halfModsize));
    glVertex2f((float)(modSize * (col + 0.5) + padSize + halfModsize),
               (float)(modSize * (row + 0.5) + padSize + halfModsize));
    glVertex2f((float)(modSize * (col + 0.5) + padSize - halfModsize),
               (float)(modSize * (row + 0.5) + padSize + halfModsize));
    glEnd();
}

/**
 *
 *
 */
void FinalCallback(DmtxDecode *decode, DmtxRegion *region)
{
    int row, col;
    int symbolRows, symbolCols;
    int moduleStatus = 0;
    /* DmtxColor3 black = { 0.0, 0.0, 0.0 };
       DmtxColor3 white = { 255.0, 255.0, 255.0 }; */

    symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, region->sizeIdx);
    symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, region->sizeIdx);

    for (row = 0; row < symbolRows; row++) {
        for (col = 0; col < symbolCols; col++) {
            /*       moduleStatus = dmtxSymbolModuleStatus(message, region->sizeIdx, row, col); */
            PlotModuleCallback(decode, region, row, col, (moduleStatus & DmtxModuleOnRGB) ? 1 : 0);
        }
    }
}
