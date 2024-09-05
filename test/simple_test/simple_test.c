/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2008, 2009 Mike Laughton. All rights reserved.
 * Copyright 2010-2016 Vadim A. Misbakh-Soloviov. All rights reserved.
 * Copyright 2016 Tim Zaman. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact:
 * Vadim A. Misbakh-Soloviov <dmtx@mva.name>
 * Mike Laughton <mike@dragonflylogic.com>
 *
 * \file simple_test.c
 */

#include <assert.h>
#include <dmtx.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    size_t width, height, bytesPerPixel;
    unsigned char str[] = "30Q324343430794<OQQ";
    unsigned char *pxl;
    DmtxEncode *enc;
    DmtxImage *img;
    DmtxDecode *dec;
    DmtxRegion *reg;
    DmtxMessage *msg;

    printf("input:  \"%s\"\n", str);

    /* 1) ENCODE a new Data Matrix barcode image (in memory only) */

    enc = dmtxEncodeCreate();

    /*
     dmtxEncodeSetProp( enc, DmtxPropPixelPacking, DmtxPack16bppRGB );
     dmtxEncodeSetProp( enc, DmtxPropPixelPacking, DmtxPack32bppRGB );
     dmtxEncodeSetProp( enc, DmtxPropWidth, 160 );
     dmtxEncodeSetProp( enc, DmtxPropHeight, 160 );
    */

    assert(enc != NULL);
    dmtxEncodeDataMatrix(enc, (int)strlen((const char *)str), str);

    /* 2) COPY the new image data before releasing encoding memory */

    width = dmtxImageGetProp(enc->image, DmtxPropWidth);
    height = dmtxImageGetProp(enc->image, DmtxPropHeight);
    bytesPerPixel = dmtxImageGetProp(enc->image, DmtxPropBytesPerPixel);

    pxl = (unsigned char *)malloc(width * height * bytesPerPixel);
    assert(pxl != NULL);
    memcpy(pxl, enc->image->pxl, width * height * bytesPerPixel);

    dmtxEncodeDestroy(&enc);

    printf("width:  \"%zd\"\n", width);
    printf("height: \"%zd\"\n", height);
    printf("bpp:    \"%zd\"\n", bytesPerPixel);

    for (int i = 0; i < width * height; i++) {
        printf("%d", (pxl[i * 3]) == 0);
        if (i % width == width - 1) {
            printf("\n");
        }
    }

    /* 3) DECODE the Data Matrix barcode from the copied image */

    img = dmtxImageCreate(pxl, (int)width, (int)height, DmtxPack24bppRGB);
    assert(img != NULL);

    dec = dmtxDecodeCreate(img, 1);
    assert(dec != NULL);

    reg = dmtxRegionFindNext(dec, NULL);
    if (reg != NULL) {
        msg = dmtxDecodeMatrixRegion(dec, reg, DmtxUndefined);

        printf("msg->arraySize :  \"%zd\"\n", msg->arraySize);
        printf("msg->codeSize  :  \"%zd\"\n", msg->codeSize);
        printf("msg->outputSize:  \"%zd\"\n", msg->outputSize);
        int oned = (int)sqrt((double)msg->arraySize);
        for (int i = 0; i < msg->arraySize; i++) {
            printf(" %c.", msg->array[i]);
            if (i % oned == oned - 1) {
                printf("\n");
            }
        }
        printf("\n\n");
        for (int j = 0; j < msg->codeSize; j++) {
            printf(" %c.", msg->code[j]);
        }
        printf("\n\n");
        for (int k = 0; k < msg->outputSize; k++) {
            printf(" %c.", msg->output[k]);
        }
        printf("\n\n");

        if (msg != NULL) {
            fputs("output: \"", stdout);
            fwrite(msg->output, sizeof(unsigned char), msg->outputIdx, stdout);
            fputs("\"\n", stdout);
            dmtxMessageDestroy(&msg);
        }
        dmtxRegionDestroy(&reg);
    }

    dmtxDecodeDestroy(&dec);
    dmtxImageDestroy(&img);
    free(pxl);

    printf("%d\n", getSizeIdxFromSymbolDimension(12, 12));

    exit(0);
}
