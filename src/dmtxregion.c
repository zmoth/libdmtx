/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2008, 2009 Mike Laughton. All rights reserved.
 * Copyright 2012-2016 Vadim A. Misbakh-Soloviov. All rights reserved.
 * Copyright 2016 Tim Zaman. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact:
 * Vadim A. Misbakh-Soloviov <dmtx@mva.name>
 * Mike Laughton <mike@dragonflylogic.com>
 *
 * @file dmtxregion.c
 * @brief Detect barcode regions
 */

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "dmtx.h"
#include "dmtxstatic.h"

#define DMTX_HOUGH_RES 180

/**
 * @brief  Create copy of existing region struct
 * @param  None
 * @return Initialized DmtxRegion struct
 */
extern DmtxRegion *dmtxRegionCreate(DmtxRegion *reg)
{
    DmtxRegion *regCopy;

    regCopy = (DmtxRegion *)malloc(sizeof(DmtxRegion));
    if (regCopy == NULL) {
        return NULL;
    }

    memcpy(regCopy, reg, sizeof(DmtxRegion));

    return regCopy;
}

/**
 * @brief  Destroy region struct
 * @param  reg
 * @return void
 */
extern DmtxPassFail dmtxRegionDestroy(DmtxRegion **reg)
{
    if (reg == NULL || *reg == NULL) {
        return DmtxFail;
    }

    free(*reg);

    *reg = NULL;

    return DmtxPass;
}

/**
 * @brief 寻找下一个二维码区域
 * @param dec Pointer to DmtxDecode information struct
 * @param timeout 超时时间 (如果为NULL则不限时)
 */
extern DmtxRegion *dmtxRegionFindNext(DmtxDecode *dec, DmtxTime *timeout)
{
    int locStatus;
    DmtxPixelLoc loc;
    DmtxRegion *reg;

    /* Continue until we find a region or run out of chances */
    for (;;) {
        locStatus = popGridLocation(&(dec->grid), &loc);
        if (locStatus == DmtxRangeEnd) {
            break;
        }

        /* 扫描确认loc坐标位置是否存在二维码区域 */
        reg = dmtxRegionScanPixel(dec, loc.x, loc.y);
        if (reg != NULL) {
            return reg;  // 成功找到一个二维码区域
        }

        /* 超时检测 */
        if (timeout != NULL && dmtxTimeExceeded(*timeout)) {
            break;
        }
    }

    return NULL;
}

/**
 * @brief 将坐标点(x,y)作为二维码L型框的边缘点去匹配二维码包围框
 */
extern DmtxRegion *dmtxRegionScanPixel(DmtxDecode *dec, int x, int y)
{
    unsigned char *cache;
    DmtxRegion reg;
    DmtxPointFlow flowBegin;
    DmtxPixelLoc loc;

    loc.x = x;
    loc.y = y;

    cache = dmtxDecodeGetCache(dec, loc.x, loc.y);
    if (cache == NULL) {
        return NULL;
    }

    if ((int)(*cache & 0x80) != 0x00) {
        return NULL;
    }

    /* Test for presence of any reasonable edge at this location */
    flowBegin = matrixRegionSeekEdge(dec, loc);
    if (flowBegin.mag < (int)(dec->edgeThresh * 7.65 + 0.5)) {
        return NULL;
    }

    memset(&reg, 0x00, sizeof(DmtxRegion));

    /* Determine barcode orientation */
    if (matrixRegionOrientation(dec, &reg, flowBegin) == DmtxFail) {
        return NULL;
    }
    if (dmtxRegionUpdateXfrms(dec, &reg) == DmtxFail) {
        return NULL;
    }

    /* 匹配顶部点线 */
    if (matrixRegionAlignCalibEdge(dec, &reg, DmtxEdgeTop) == DmtxFail) {
        return NULL;
    }
    if (dmtxRegionUpdateXfrms(dec, &reg) == DmtxFail) {
        return NULL;
    }

    /* 匹配右侧点线 */
    if (matrixRegionAlignCalibEdge(dec, &reg, DmtxEdgeRight) == DmtxFail) {
        return NULL;
    }
    if (dmtxRegionUpdateXfrms(dec, &reg) == DmtxFail) {
        return NULL;
    }

    if (cbBuildMatrixRegion) {
        cbBuildMatrixRegion(&reg);
    }

    /* 计算最匹配的二维码符号尺寸 */
    if (matrixRegionFindSize(dec, &reg) == DmtxFail) {
        return NULL;
    }

    /* Found a valid matrix region */
    return dmtxRegionCreate(&reg);
}

/**
 * @brief 寻找指定像素位置梯度流向，并检查该点是否能形成闭环。如果成功暂定该点在DataMatrix的'L'型边上
 *
 * 该函数分析指定像素位置的各颜色通道，以确定哪个通道展现了最明显的边缘特征。
 * 然后，它通过分析正负方向上的邻近像素强度，精确定位并返回最强边缘的方向向量。
 * 如果找不到清晰的边缘特征，则返回表示空白边缘(dmtxBlankEdge)的默认值。
 *
 * @param dec 解码上下文，包含图像数据和解码设置。
 * @param loc 待分析的像素位置。
 *
 * @return DmtxPointFlow 结构，表示找到的边缘方向和强度；如果未找到有效边缘，则返回 dmtxBlankEdge。
 */
static DmtxPointFlow matrixRegionSeekEdge(DmtxDecode *dec, DmtxPixelLoc loc)
{
    int i;
    int strongIdx;
    int channelCount;
    DmtxPointFlow flow, flowPlane[3] = {};
    DmtxPointFlow flowPos, flowPosBack;
    DmtxPointFlow flowNeg, flowNegBack;

    channelCount = dec->image->channelCount;

    /* Find whether red, green, or blue shows the strongest edge */
    strongIdx = 0;
    for (i = 0; i < channelCount; i++) {
        flowPlane[i] = getPointFlow(dec, i, loc, dmtxNeighborNone);
        if (i > 0 && flowPlane[i].mag > flowPlane[strongIdx].mag) {
            strongIdx = i;
        }
    }

    if (flowPlane[strongIdx].mag < 10) {  // 如果最强边缘的幅度小于阈值，返回空白边缘
        return dmtxBlankEdge;
    }

    flow = flowPlane[strongIdx];

    // 寻找正负方向上的最强邻近边缘
    flowPos = findStrongestNeighbor(dec, flow, +1);
    flowNeg = findStrongestNeighbor(dec, flow, -1);

    // 来回验证是否该起点的能不能按照相同的位置原路返回
    //
    // 这个机制是为了保证选取的起始点位在规定的搜索时间内能够闭合：
    // 闭合的自然条件是按照原来的方向返回该点位如果这个机制无法通过，
    // 则说明选择的点位在返回时和出发时有两个不同的方向，这种情况在一个闭合的轮廓边界中是不允许的
    if (flowPos.mag != 0 && flowNeg.mag != 0) {
        flowPosBack = findStrongestNeighbor(dec, flowPos, -1);
        flowNegBack = findStrongestNeighbor(dec, flowNeg, +1);
        if (flowPos.arrive == (flowPosBack.arrive + 4) % 8 && flowNeg.arrive == (flowNegBack.arrive + 4) % 8) {
            // 如果沿着flow点往正负方向走，如果下一个点能够通过同样的方式走回到flow点，那么认为这个点是有可能形成一个闭合的形状的
            flow.arrive = dmtxNeighborNone;
            if (cbPlotPoint) {
                cbPlotPoint(flow.loc, 0.0F, 1, 1);
            }
            return flow;
        }
    }

    return dmtxBlankEdge;
}

/**
 * @brief 确定数据矩阵区域的方向和关键边界
 *
 * 此函数分析并确定给定解码上下文中数据矩阵区域的方向性、极性和关键坐标点。
 * 它通过跟随边缘、查找最佳直线、评估交叉点等步骤来实现对区域的精确定位。
 *
 * @param dec 解码上下文，包含了解码设置和辅助信息
 * @param reg 待分析的数据矩阵区域结构体，用于存储识别结果
 * @param begin 起始追踪点，用于初始化追踪过程
 *
 * @retval DmtxPass 成功确定区域方向
 * @retval DmtxFail 未能成功确定区域方向或区域不符合预期条件
 */
static DmtxPassFail matrixRegionOrientation(DmtxDecode *dec, DmtxRegion *reg, DmtxPointFlow begin)
{
    int cross;
    int minArea;
    int scale;
    int symbolShape;
    int maxDiagonal;
    DmtxPassFail err;
    DmtxBestLine line1x, line2x;
    DmtxBestLine line2n, line2p;
    DmtxFollow fTmp;

    if (dec->sizeIdxExpected == DmtxSymbolSquareAuto ||
        (dec->sizeIdxExpected >= DmtxSymbol10x10 && dec->sizeIdxExpected <= DmtxSymbol144x144)) {
        symbolShape = DmtxSymbolSquareAuto;
    } else if (dec->sizeIdxExpected == DmtxSymbolRectAuto ||
               (dec->sizeIdxExpected >= DmtxSymbol8x18 && dec->sizeIdxExpected <= DmtxSymbol16x48)) {
        symbolShape = DmtxSymbolRectAuto;
    } else {
        symbolShape = DmtxSymbolShapeAuto;
    }

    if (dec->edgeMax != DmtxUndefined) {
        if (symbolShape == DmtxSymbolRectAuto) {
            maxDiagonal = (int)(1.23 * dec->edgeMax + 0.5); /* sqrt(5/4) + 10% */
        } else {
            maxDiagonal = (int)(1.56 * dec->edgeMax + 0.5); /* sqrt(2) + 10% */
        }
    } else {
        maxDiagonal = DmtxUndefined;
    }

    /* 以十字搜索像素点为起点，分别从正负方向寻边 */
    err = trailBlazeContinuous(dec, reg, begin, maxDiagonal);
    if (err == DmtxFail || reg->stepsTotal < 40) {
        trailClear(dec, reg, 0x40);
        return DmtxFail;
    }

    /* Filter out region candidates that are smaller than expected */
    if (dec->edgeMin != DmtxUndefined) {
        scale = dmtxDecodeGetProp(dec, DmtxPropScale);

        if (symbolShape == DmtxSymbolSquareAuto) {
            minArea = (dec->edgeMin * dec->edgeMin) / (scale * scale);
        } else {
            minArea = (2 * dec->edgeMin * dec->edgeMin) / (scale * scale);
        }

        if ((reg->boundMax.x - reg->boundMin.x) * (reg->boundMax.y - reg->boundMin.y) < minArea) {
            trailClear(dec, reg, 0x40);
            return DmtxFail;
        }
    }

    /* 寻找第一条直线 */
    line1x = findBestSolidLine(dec, reg, 0, 0, +1, DmtxUndefined);
    if (line1x.mag < 5) {
        trailClear(dec, reg, 0x40);
        return DmtxFail;
    }

    err = findTravelLimits(dec, reg, &line1x);
    if (err == DmtxFail || line1x.distSq < 100 || line1x.devn * 10 >= sqrt((double)line1x.distSq)) {
        trailClear(dec, reg, 0x40);
        return DmtxFail;
    }
    DmtxAssert(line1x.stepPos >= line1x.stepNeg);

    /* 第一条直线正向搜索另一条直线 */
    fTmp = followSeek(dec, reg, line1x.stepPos + 5);
    line2p = findBestSolidLine(dec, reg, fTmp.step, line1x.stepNeg, +1, line1x.angle);

    /* 第一条直线负向搜索另一条直线 */
    fTmp = followSeek(dec, reg, line1x.stepNeg - 5);
    line2n = findBestSolidLine(dec, reg, fTmp.step, line1x.stepPos, -1, line1x.angle);
    if (max(line2p.mag, line2n.mag) < 5) {
        return DmtxFail;
    }

    if (line2p.mag > line2n.mag) {
        line2x = line2p;
        err = findTravelLimits(dec, reg, &line2x);
        if (err == DmtxFail || line2x.distSq < 100 || line2x.devn * 10 >= sqrt((double)line2x.distSq)) {
            return DmtxFail;
        }

        cross = ((line1x.locPos.x - line1x.locNeg.x) * (line2x.locPos.y - line2x.locNeg.y)) -
                ((line1x.locPos.y - line1x.locNeg.y) * (line2x.locPos.x - line2x.locNeg.x));
        if (cross > 0) {
            /* Condition 2 */
            reg->polarity = +1;
            reg->locR = line2x.locPos;
            reg->stepR = line2x.stepPos;
            reg->locT = line1x.locNeg;
            reg->stepT = line1x.stepNeg;
            reg->leftLoc = line1x.locBeg;
            reg->leftAngle = line1x.angle;
            reg->bottomLoc = line2x.locBeg;
            reg->bottomAngle = line2x.angle;
            reg->leftLine = line1x;
            reg->bottomLine = line2x;
        } else {
            /* Condition 3 */
            reg->polarity = -1;
            reg->locR = line1x.locNeg;
            reg->stepR = line1x.stepNeg;
            reg->locT = line2x.locPos;
            reg->stepT = line2x.stepPos;
            reg->leftLoc = line2x.locBeg;
            reg->leftAngle = line2x.angle;
            reg->bottomLoc = line1x.locBeg;
            reg->bottomAngle = line1x.angle;
            reg->leftLine = line2x;
            reg->bottomLine = line1x;
        }
    } else {
        line2x = line2n;
        err = findTravelLimits(dec, reg, &line2x);
        if (err == DmtxFail || line2x.distSq < 100 || line2x.devn / sqrt((double)line2x.distSq) >= 0.1) {
            return DmtxFail;
        }

        cross = ((line1x.locNeg.x - line1x.locPos.x) * (line2x.locNeg.y - line2x.locPos.y)) -
                ((line1x.locNeg.y - line1x.locPos.y) * (line2x.locNeg.x - line2x.locPos.x));
        if (cross > 0) {
            /* Condition 1 */
            reg->polarity = -1;
            reg->locR = line2x.locNeg;
            reg->stepR = line2x.stepNeg;
            reg->locT = line1x.locPos;
            reg->stepT = line1x.stepPos;
            reg->leftLoc = line1x.locBeg;
            reg->leftAngle = line1x.angle;
            reg->bottomLoc = line2x.locBeg;
            reg->bottomAngle = line2x.angle;
            reg->leftLine = line1x;
            reg->bottomLine = line2x;
        } else {
            /* Condition 4 */
            reg->polarity = +1;
            reg->locR = line1x.locPos;
            reg->stepR = line1x.stepPos;
            reg->locT = line2x.locNeg;
            reg->stepT = line2x.stepNeg;
            reg->leftLoc = line2x.locBeg;
            reg->leftAngle = line2x.angle;
            reg->bottomLoc = line1x.locBeg;
            reg->bottomAngle = line1x.angle;
            reg->leftLine = line2x;
            reg->bottomLine = line1x;
        }
    }

    if (cbPlotPoint) {
        cbPlotPoint(reg->locR, 180.0F /*青*/, 1, 1);
        cbPlotPoint(reg->locT, 180.0F /*青*/, 1, 1);
    }

    reg->leftKnown = reg->bottomKnown = 1;

    return DmtxPass;
}

/**
 * @brief 计算两个像素点之间的欧几里得距离的平方
 */
static long distanceSquared(DmtxPixelLoc a, DmtxPixelLoc b)
{
    long xDelta, yDelta;

    xDelta = a.x - b.x;
    yDelta = a.y - b.y;

    return (xDelta * xDelta) + (yDelta * yDelta);
}

/**
 *
 *
 */
extern DmtxPassFail dmtxRegionUpdateCorners(DmtxDecode *dec, DmtxRegion *reg, DmtxVector2 p00, DmtxVector2 p10,
                                            DmtxVector2 p11, DmtxVector2 p01)
{
    double xMax, yMax;
    double tx, ty, phi, shx, scx, scy, skx, sky;
    double dimOT, dimOR, dimTX, dimRX, ratio;
    DmtxVector2 vOT, vOR, vTX, vRX, vTmp;
    DmtxMatrix3 m, mtxy, mphi, mshx, mscx, mscy, mscxy, msky, mskx;

    xMax = (double)(dmtxDecodeGetProp(dec, DmtxPropWidth) - 1);
    yMax = (double)(dmtxDecodeGetProp(dec, DmtxPropHeight) - 1);

    if (p00.x < 0.0 || p00.y < 0.0 || p00.x > xMax || p00.y > yMax || p01.x < 0.0 || p01.y < 0.0 || p01.x > xMax ||
        p01.y > yMax || p10.x < 0.0 || p10.y < 0.0 || p10.x > xMax || p10.y > yMax) {
        return DmtxFail;
    }

    dimOT = dmtxVector2Mag(dmtxVector2Sub(&vOT, &p01, &p00)); /* XXX could use MagSquared() */
    dimOR = dmtxVector2Mag(dmtxVector2Sub(&vOR, &p10, &p00));
    dimTX = dmtxVector2Mag(dmtxVector2Sub(&vTX, &p11, &p01));
    dimRX = dmtxVector2Mag(dmtxVector2Sub(&vRX, &p11, &p10));

    /* Verify that sides are reasonably long */
    if (dimOT <= 8.0 || dimOR <= 8.0 || dimTX <= 8.0 || dimRX <= 8.0) {
        return DmtxFail;
    }

    /* Verify that the 4 corners define a reasonably fat quadrilateral */
    ratio = dimOT / dimRX;
    if (ratio <= 0.5 || ratio >= 2.0) {
        return DmtxFail;
    }

    ratio = dimOR / dimTX;
    if (ratio <= 0.5 || ratio >= 2.0) {
        return DmtxFail;
    }

    /* Verify this is not a bowtie shape */
    if (dmtxVector2Cross(&vOR, &vRX) <= 0.0 || dmtxVector2Cross(&vOT, &vTX) >= 0.0) {
        return DmtxFail;
    }

    if (rightAngleTrueness(p00, p10, p11, M_PI_2) <= dec->squareDevn) {
        return DmtxFail;
    }
    if (rightAngleTrueness(p10, p11, p01, M_PI_2) <= dec->squareDevn) {
        return DmtxFail;
    }

    /* Calculate values needed for transformations */
    tx = -1 * p00.x;
    ty = -1 * p00.y;
    dmtxMatrix3Translate(mtxy, tx, ty);

    phi = atan2(vOT.x, vOT.y);
    dmtxMatrix3Rotate(mphi, phi);
    dmtxMatrix3Multiply(m, mtxy, mphi);

    dmtxMatrix3VMultiply(&vTmp, &p10, m);
    shx = -vTmp.y / vTmp.x;
    dmtxMatrix3Shear(mshx, 0.0, shx);
    dmtxMatrix3MultiplyBy(m, mshx);

    scx = 1.0 / vTmp.x;
    dmtxMatrix3Scale(mscx, scx, 1.0);
    dmtxMatrix3MultiplyBy(m, mscx);

    dmtxMatrix3VMultiply(&vTmp, &p11, m);
    scy = 1.0 / vTmp.y;
    dmtxMatrix3Scale(mscy, 1.0, scy);
    dmtxMatrix3MultiplyBy(m, mscy);

    dmtxMatrix3VMultiply(&vTmp, &p11, m);
    skx = vTmp.x;
    dmtxMatrix3LineSkewSide(mskx, 1.0, skx, 1.0);
    dmtxMatrix3MultiplyBy(m, mskx);

    dmtxMatrix3VMultiply(&vTmp, &p01, m);
    sky = vTmp.y;
    dmtxMatrix3LineSkewTop(msky, sky, 1.0, 1.0);
    dmtxMatrix3Multiply(reg->raw2fit, m, msky);

    /* Create inverse matrix by reverse (avoid straight matrix inversion) */
    dmtxMatrix3LineSkewTopInv(msky, sky, 1.0, 1.0);
    dmtxMatrix3LineSkewSideInv(mskx, 1.0, skx, 1.0);
    dmtxMatrix3Multiply(m, msky, mskx);

    dmtxMatrix3Scale(mscxy, 1.0 / scx, 1.0 / scy);
    dmtxMatrix3MultiplyBy(m, mscxy);

    dmtxMatrix3Shear(mshx, 0.0, -shx);
    dmtxMatrix3MultiplyBy(m, mshx);

    dmtxMatrix3Rotate(mphi, -phi);
    dmtxMatrix3MultiplyBy(m, mphi);

    dmtxMatrix3Translate(mtxy, -tx, -ty);
    dmtxMatrix3Multiply(reg->fit2raw, m, mtxy);

    return DmtxPass;
}

/**
 *
 *
 */
extern DmtxPassFail dmtxRegionUpdateXfrms(DmtxDecode *dec, DmtxRegion *reg)
{
    double radians;
    DmtxRay2 rLeft, rBottom, rTop, rRight;
    DmtxVector2 p00, p10, p11, p01;

    DmtxAssert(reg->leftKnown != 0 && reg->bottomKnown != 0);

    /* Build ray representing left edge */
    rLeft.p.x = (double)reg->leftLoc.x;
    rLeft.p.y = (double)reg->leftLoc.y;
    radians = reg->leftAngle * (M_PI / DMTX_HOUGH_RES);
    rLeft.v.x = cos(radians);
    rLeft.v.y = sin(radians);
    rLeft.tMin = 0.0;
    rLeft.tMax = dmtxVector2Norm(&rLeft.v);

    /* Build ray representing bottom edge */
    rBottom.p.x = (double)reg->bottomLoc.x;
    rBottom.p.y = (double)reg->bottomLoc.y;
    radians = reg->bottomAngle * (M_PI / DMTX_HOUGH_RES);
    rBottom.v.x = cos(radians);
    rBottom.v.y = sin(radians);
    rBottom.tMin = 0.0;
    rBottom.tMax = dmtxVector2Norm(&rBottom.v);

    /* Build ray representing top edge */
    if (reg->topKnown != 0) {
        rTop.p.x = (double)reg->topLoc.x;
        rTop.p.y = (double)reg->topLoc.y;
        radians = reg->topAngle * (M_PI / DMTX_HOUGH_RES);
        rTop.v.x = cos(radians);
        rTop.v.y = sin(radians);
        rTop.tMin = 0.0;
        rTop.tMax = dmtxVector2Norm(&rTop.v);
    } else {
        rTop.p.x = (double)reg->locT.x;
        rTop.p.y = (double)reg->locT.y;
        radians = reg->bottomAngle * (M_PI / DMTX_HOUGH_RES);
        rTop.v.x = cos(radians);
        rTop.v.y = sin(radians);
        rTop.tMin = 0.0;
        rTop.tMax = rBottom.tMax;
    }

    /* Build ray representing right edge */
    if (reg->rightKnown != 0) {
        rRight.p.x = (double)reg->rightLoc.x;
        rRight.p.y = (double)reg->rightLoc.y;
        radians = reg->rightAngle * (M_PI / DMTX_HOUGH_RES);
        rRight.v.x = cos(radians);
        rRight.v.y = sin(radians);
        rRight.tMin = 0.0;
        rRight.tMax = dmtxVector2Norm(&rRight.v);
    } else {
        rRight.p.x = (double)reg->locR.x;
        rRight.p.y = (double)reg->locR.y;
        radians = reg->leftAngle * (M_PI / DMTX_HOUGH_RES);
        rRight.v.x = cos(radians);
        rRight.v.y = sin(radians);
        rRight.tMin = 0.0;
        rRight.tMax = rLeft.tMax;
    }

    /* Calculate 4 corners, real or imagined */
    if (dmtxRay2Intersect(&p00, &rLeft, &rBottom) == DmtxFail) {
        return DmtxFail;
    }

    if (dmtxRay2Intersect(&p10, &rBottom, &rRight) == DmtxFail) {
        return DmtxFail;
    }

    if (dmtxRay2Intersect(&p11, &rRight, &rTop) == DmtxFail) {
        return DmtxFail;
    }

    if (dmtxRay2Intersect(&p01, &rTop, &rLeft) == DmtxFail) {
        return DmtxFail;
    }

    if (dmtxRegionUpdateCorners(dec, reg, p00, p10, p11, p01) != DmtxPass) {
        return DmtxFail;
    }

    return DmtxPass;
}

/**
 *
 *
 */
static double rightAngleTrueness(DmtxVector2 c0, DmtxVector2 c1, DmtxVector2 c2, double angle)
{
    DmtxVector2 vA, vB;
    DmtxMatrix3 m;

    dmtxVector2Norm(dmtxVector2Sub(&vA, &c0, &c1));
    dmtxVector2Norm(dmtxVector2Sub(&vB, &c2, &c1));

    dmtxMatrix3Rotate(m, angle);
    dmtxMatrix3VMultiplyBy(&vB, m);

    return dmtxVector2Dot(&vA, &vB);
}

/**
 * @brief 读取模块（码元）颜色值
 *
 * @param dec 解码上下文，包含解码所需的状态信息。
 * @param reg 当前处理的区域信息，用于定位和变换。
 * @param symbolRow 二维码坐标系下的行坐标
 * @param symbolCol 二维码坐标系下的列坐标
 * @param sizeIdx 二维码种类索引
 * @param colorPlane 需要读取的颜色平面索引
 *
 * @return 返回模块位置的平均颜色值。
 */
static int readModuleColor(DmtxDecode *dec, DmtxRegion *reg, int symbolRow, int symbolCol, int sizeIdx, int colorPlane)
{
    int i;
    int symbolRows, symbolCols;
    int color, colorTmp;
    double sampleX[] = {0.5, 0.4, 0.5, 0.6, 0.5};
    double sampleY[] = {0.5, 0.5, 0.4, 0.5, 0.6};
    DmtxVector2 p;

    symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, sizeIdx);
    symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, sizeIdx);

    /* 从给定坐标及其周围共5个点位获取图像像素并求平均值 */
    color = 0;
    for (i = 0; i < 5; i++) {
        p.x = (1.0 / symbolCols) * (symbolCol + sampleX[i]);
        p.y = (1.0 / symbolRows) * (symbolRow + sampleY[i]);

        dmtxMatrix3VMultiplyBy(&p, reg->fit2raw);  // 从二维码坐标转换到图像坐标

        // dmtxLogDebug("%dx%d", (int)(p.x + 0.5), (int)(p.y + 0.5));

        if (cbPlotModule) {
            cbPlotModule(dec, reg, (int)(p.x + 0.5), (int)(p.y + 0.5), 0);
        }

        dmtxDecodeGetPixelValue(dec, (int)(p.x + 0.5), (int)(p.y + 0.5), colorPlane, &colorTmp);
        color += colorTmp;
    }
    // printf("\n");
    return color / 5;
}

/**
 * @brief 确定二维码尺寸（点线中黑白点的总数）
 *
 * 此函数遍历可能的条形码尺寸，通过计算校准模块上的对比度来确定最佳尺寸索引。
 * 它还会验证找到的尺寸索引是否与条形码图像的边缘和空白空间相匹配。
 *
 * @param[in] dec  解码上下文，包含解码设置和图像信息
 * @param reg  区域结构，用于存储找到的区域信息
 */
static DmtxPassFail matrixRegionFindSize(DmtxDecode *dec, DmtxRegion *reg)
{
    int row, col;
    int sizeIdxBeg, sizeIdxEnd;
    int sizeIdx, bestSizeIdx;
    int symbolRows, symbolCols;
    int jumpCount, errors;
    int color;
    int colorOnAvg, bestColorOnAvg;
    int colorOffAvg, bestColorOffAvg;
    int contrast, bestContrast;
    //   DmtxImage *img;

    //   img = dec->image;
    bestSizeIdx = DmtxUndefined;
    bestContrast = 0;
    bestColorOnAvg = bestColorOffAvg = 0;

    if (dec->sizeIdxExpected == DmtxSymbolShapeAuto) {
        sizeIdxBeg = 0;
        sizeIdxEnd = DmtxSymbolSquareCount + DmtxSymbolRectCount;
    } else if (dec->sizeIdxExpected == DmtxSymbolSquareAuto) {
        sizeIdxBeg = 0;
        sizeIdxEnd = DmtxSymbolSquareCount;
    } else if (dec->sizeIdxExpected == DmtxSymbolRectAuto) {
        sizeIdxBeg = DmtxSymbolSquareCount;
        sizeIdxEnd = DmtxSymbolSquareCount + DmtxSymbolRectCount;
    } else {
        sizeIdxBeg = dec->sizeIdxExpected;
        sizeIdxEnd = dec->sizeIdxExpected + 1;
    }

    /* 遍历每种DataMatrix种类模板，通过顶部和右侧的点线取颜色计算寻找对比度最大的模板 */
    for (sizeIdx = sizeIdxBeg; sizeIdx < sizeIdxEnd; sizeIdx++) {
        symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, sizeIdx);
        symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, sizeIdx);
        colorOnAvg = colorOffAvg = 0;

        /* 对DataMatrix顶部点线黑白码元分别求和 */
        row = symbolRows - 1;
        for (col = 0; col < symbolCols; col++) {
            color = readModuleColor(dec, reg, row, col, sizeIdx, reg->flowBegin.plane);
            if ((col & 0x01) != 0x00) {
                colorOffAvg += color;
            } else {
                colorOnAvg += color;
            }
        }

        /* 对DataMatrix右侧点线黑白码元分别求和 */
        col = symbolCols - 1;
        for (row = 0; row < symbolRows; row++) {
            color = readModuleColor(dec, reg, row, col, sizeIdx, reg->flowBegin.plane);
            if ((row & 0x01) != 0x00) {
                colorOffAvg += color;
            } else {
                colorOnAvg += color;
            }
        }

        colorOnAvg = (colorOnAvg * 2) / (symbolRows + symbolCols);
        colorOffAvg = (colorOffAvg * 2) / (symbolRows + symbolCols);

        contrast = abs(colorOnAvg - colorOffAvg);
        if (contrast < 20) {
            continue;  // bit1码元与bit0码元的差值小于20直接认为该模板无效
        }

        /* 遍历所有类型，寻找效果最好的 */
        if (contrast > bestContrast) {
            bestContrast = contrast;
            bestSizeIdx = sizeIdx;
            bestColorOnAvg = colorOnAvg;
            bestColorOffAvg = colorOffAvg;
        }
    }

    /* 如果所有的模板都不是很匹配，直接返回错误 */
    if (bestSizeIdx == DmtxUndefined || bestContrast < 20) {
        return DmtxFail;
    }

    reg->sizeIdx = bestSizeIdx;       // 最佳DataMatrix种类模板的索引号，共30种
    reg->onColor = bestColorOnAvg;    // bit1的码元颜色值
    reg->offColor = bestColorOffAvg;  // bit0的码元颜色值

    reg->symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, reg->sizeIdx);
    reg->symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, reg->sizeIdx);
    reg->mappingRows = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixRows, reg->sizeIdx);
    reg->mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, reg->sizeIdx);

    /**
     * 以左上角水平往右检查
     *
     * TL----XX----XX----->
     * XX 01 02 03 04 XX
     * XX 05 06 07 08
     * XX 09 10 11 12 XX
     * XX 13 14 15 16
     * XX XX XX XX XX XX
     */
    jumpCount = countJumpTally(dec, reg, 0, reg->symbolRows - 1, DmtxDirRight);
    errors = abs(1 + jumpCount - reg->symbolCols);
    if (jumpCount < 0 || errors > 2) {
        return DmtxFail;
    }

    /**
     * 以右下角角竖直往上检查
     *
     *                ^
     *                |
     * XX    XX    XX |
     * XX 01 02 03 04 XX
     * XX 05 06 07 08 |
     * XX 09 10 11 12 XX
     * XX 13 14 15 16 |
     * XX XX XX XX XX BR
     */
    jumpCount = countJumpTally(dec, reg, reg->symbolCols - 1, 0, DmtxDirUp);
    errors = abs(1 + jumpCount - reg->symbolRows);
    if (jumpCount < 0 || errors > 2) {
        return DmtxFail;
    }

    /**
     * 以左下角水平往右检查
     *
     * XX    XX    XX
     * XX 01 02 03 04 XX
     * XX 05 06 07 08
     * XX 09 10 11 12 XX
     * XX 13 14 15 16
     * BL XX XX XX XX XX-->
     */
    errors = countJumpTally(dec, reg, 0, 0, DmtxDirRight);
    if (jumpCount < 0 || errors > 2) {
        return DmtxFail;
    }

    /**
     * 以左下角竖直往上检查
     *
     *  ^
     *  |
     * XX    XX    XX
     * XX 01 02 03 04 XX
     * XX 05 06 07 08
     * XX 09 10 11 12 XX
     * XX 13 14 15 16
     * BL XX XX XX XX XX
     */
    errors = countJumpTally(dec, reg, 0, 0, DmtxDirUp);
    if (errors < 0 || errors > 2) {
        return DmtxFail;
    }

    /**
     * 分析DataMatrix周围的空间
     *
     * XX    XX    XX
     * XX 01 02 03 04 XX
     * XX 05 06 07 08
     * XX 09 10 11 12 XX
     * XX 13 14 15 16
     * XX XX XX XX XX XX
     * 00----------------->
     */
    errors = countJumpTally(dec, reg, 0, -1, DmtxDirRight);
    if (errors < 0 || errors > 2) {
        return DmtxFail;
    }

    /**
     *  ^
     *  |
     *  | XX    XX    XX
     *  | XX 01 02 03 04 XX
     *  | XX 05 06 07 08
     *  | XX 09 10 11 12 XX
     *  | XX 13 14 15 16
     * 00 XX XX XX XX XX XX
     */
    errors = countJumpTally(dec, reg, -1, 0, DmtxDirUp);
    if (errors < 0 || errors > 2) {
        return DmtxFail;
    }

    /**
     * 00----------------->
     * XX    XX    XX
     * XX 01 02 03 04 XX
     * XX 05 06 07 08
     * XX 09 10 11 12 XX
     * XX 13 14 15 16
     * XX XX XX XX XX XX
     */
    errors = countJumpTally(dec, reg, 0, reg->symbolRows, DmtxDirRight);
    if (errors < 0 || errors > 2) {
        return DmtxFail;
    }

    /**
     *                    ^
     *                    |
     * XX    XX    XX     |
     * XX 01 02 03 04 XX  |
     * XX 05 06 07 08     |
     * XX 09 10 11 12 XX  |
     * XX 13 14 15 16     |
     * XX XX XX XX XX XX 00
     */
    errors = countJumpTally(dec, reg, reg->symbolCols, 0, DmtxDirUp);
    if (errors < 0 || errors > 2) {
        return DmtxFail;
    }

    return DmtxPass;
}

/**
 * @brief 计算一个方向上颜色跳变次数
 *
 * 此函数遍历图像中的一行或一列，统计从亮模块到暗模块或反之的转换次数。
 *
 * @param dec 解码上下文，提供解码环境信息
 * @param reg 区域对象，包含当前分析区域的属性
 * @param xStart 开始搜索的坐标X位置
 * @param yStart 开始搜索的坐标Y位置
 * @param dir 搜索方向，向右(DmtxDirRight)或向上(DmtxDirUp)
 */
static int countJumpTally(DmtxDecode *dec, DmtxRegion *reg, int xStart, int yStart, DmtxDirection dir)
{
    int x, xInc = 0;
    int y, yInc = 0;
    int state = DmtxModuleOn;
    int jumpCount = 0;
    int jumpThreshold;
    int tModule, tPrev;
    int darkOnLight;  // 白底黑码：1，黑底白码：0
    int color;

    DmtxAssert(xStart == 0 || yStart == 0);
    DmtxAssert(dir == DmtxDirRight || dir == DmtxDirUp);

    if (dir == DmtxDirRight) {
        xInc = 1;
    } else {
        yInc = 1;
    }

    if (xStart == -1 || xStart == reg->symbolCols || yStart == -1 || yStart == reg->symbolRows) {
        state = DmtxModuleOff;
    }

    darkOnLight = (int)(reg->offColor > reg->onColor);
    jumpThreshold = abs((int)(0.4 * (reg->onColor - reg->offColor) + 0.5));
    color = readModuleColor(dec, reg, yStart, xStart, reg->sizeIdx, reg->flowBegin.plane);
    tModule = (darkOnLight) ? reg->offColor - color : color - reg->offColor;

    for (x = xStart + xInc, y = yStart + yInc;
         (dir == DmtxDirRight && x < reg->symbolCols) || (dir == DmtxDirUp && y < reg->symbolRows);
         x += xInc, y += yInc) {
        tPrev = tModule;
        color = readModuleColor(dec, reg, y, x, reg->sizeIdx, reg->flowBegin.plane);
        tModule = (darkOnLight) ? reg->offColor - color : color - reg->offColor;

        if (state == DmtxModuleOff) {
            if (tModule > tPrev + jumpThreshold) {
                jumpCount++;
                state = DmtxModuleOn;
            }
        } else {
            if (tModule < tPrev - jumpThreshold) {
                jumpCount++;
                state = DmtxModuleOff;
            }
        }
    }

    return jumpCount;
}

/**
 * @brief 从像素坐标寻找其在指定颜色平面的梯度方向
 */
static DmtxPointFlow getPointFlow(DmtxDecode *dec, int colorPlane, DmtxPixelLoc loc, int arrive)
{
    static const int coefficient[] = {0, 1, 2, 1, 0, -1, -2, -1};
    unsigned int err;
    int patternIdx, coefficientIdx;
    int compass, compassMax;
    int mag[4] = {0};
    int xAdjust, yAdjust;
    int color, colorPattern[8];
    DmtxPointFlow flow;

    // 以loc坐标为中心按照如下所示顺序获取周边的8个像素值
    // @ref dmtxNeighborNone
    //       Y+
    //       |
    //    6  5  4
    // —— 7  8  3  ——> X+
    //    0  1  2
    //       |
    for (patternIdx = 0; patternIdx < 8; patternIdx++) {
        xAdjust = loc.x + dmtxPatternX[patternIdx];
        yAdjust = loc.y + dmtxPatternY[patternIdx];
        err = dmtxDecodeGetPixelValue(dec, xAdjust, yAdjust, colorPlane, &colorPattern[patternIdx]);
        if (err == DmtxFail) {
            return dmtxBlankEdge;
        }
    }

    /* 计算四个方向上的流动强度 (-45, 0, 45, 90) */
    compassMax = 0;
    for (compass = 0; compass < 4; compass++) {
        /* Add portion from each position in the convolution matrix pattern */
        for (patternIdx = 0; patternIdx < 8; patternIdx++) {
            coefficientIdx = (patternIdx - compass + 8) % 8;
            if (coefficient[coefficientIdx] == 0) {
                continue;
            }

            color = colorPattern[patternIdx];

            switch (coefficient[coefficientIdx]) {
                case 2:
                    mag[compass] += color;
                    /* Fall through */
                case 1:
                    mag[compass] += color;
                    break;
                case -2:
                    mag[compass] -= color;
                    /* Fall through */
                case -1:
                    mag[compass] -= color;
                    break;
            }
        }

        /* 确定最强的梯度流动方向 */
        if (compass != 0 && abs(mag[compass]) > abs(mag[compassMax])) {
            compassMax = compass;
        }
    }

    /* Convert signed compass direction into unique flow directions (0-7) */
    flow.plane = colorPlane;  // 颜色平面（彩色RGB三个平面中的一个）
    flow.arrive = arrive;     // 抵达方向 8
    flow.depart = (mag[compassMax] > 0) ? compassMax + 4
                                        : compassMax;  // 离开方向  假设是水平方向就需要判断是向左(<-),还是向右(->)
    flow.mag = abs(mag[compassMax]);  // 梯度强度
    flow.loc = loc;                   // 原始像素位置

    return flow;
}

/**
 *
 *
 */
static DmtxPointFlow findStrongestNeighbor(DmtxDecode *dec, DmtxPointFlow center, int sign)
{
    int i;
    int strongIdx;
    int attempt, attemptDiff;
    int occupied;
    unsigned char *cache;
    DmtxPixelLoc loc;
    DmtxPointFlow flow[8];

    attempt = (sign < 0) ? center.depart : (center.depart + 4) % 8;

    occupied = 0;
    strongIdx = DmtxUndefined;
    for (i = 0; i < 8; i++) {
        loc.x = center.loc.x + dmtxPatternX[i];
        loc.y = center.loc.y + dmtxPatternY[i];

        cache = dmtxDecodeGetCache(dec, loc.x, loc.y);
        if (cache == NULL) {
            continue;
        }

        if ((int)(*cache & 0x80) != 0x00) {
            if (++occupied > 2) {
                return dmtxBlankEdge;
            }
            continue;
        }

        attemptDiff = abs(attempt - i);
        if (attemptDiff > 4) {
            attemptDiff = 8 - attemptDiff;
        }
        if (attemptDiff > 1) {
            continue;
        }

        flow[i] = getPointFlow(dec, center.plane, loc, i);

        if (strongIdx == DmtxUndefined || flow[i].mag > flow[strongIdx].mag ||
            (flow[i].mag == flow[strongIdx].mag && ((i & 0x01) != 0))) {
            strongIdx = i;
        }
    }

    return (strongIdx == DmtxUndefined) ? dmtxBlankEdge : flow[strongIdx];
}

/**
 * @brief 根据指定步数从当前区域的起始点追踪到新位置
 *
 * 此函数从数据矩阵区域的起始追踪点出发，按照给定的步数（正向或负向），
 * 逐步调用`FollowStep`函数进行追踪，并返回达到的新位置信息。此功能
 * 支持快速跳转到区域内的指定追踪步骤，用于优化和控制追踪流程。
 *
 * @param dec 解码上下文，用于缓存访问
 * @param reg 当前数据矩阵区域信息，包含追踪起始位置、步数上限等
 * @param seek 相对于起始点的追踪步数，正值表示正向追踪，负值表示反向追踪
 *
 * @return 返回追踪到达的新位置信息（DmtxFollow 结构体）
 */
static DmtxFollow followSeek(DmtxDecode *dec, DmtxRegion *reg, int seek)
{
    int i;
    int sign;
    DmtxFollow follow;

    follow.loc = reg->flowBegin.loc;
    follow.step = 0;
    follow.ptr = dmtxDecodeGetCache(dec, follow.loc.x, follow.loc.y);
    DmtxAssert(follow.ptr != NULL);
    follow.neighbor = *follow.ptr;

    sign = (seek > 0) ? +1 : -1;
    for (i = 0; i != seek; i += sign) {
        follow = followStep(dec, reg, follow, sign);
        DmtxAssert(follow.ptr != NULL);
        DmtxAssert(abs(follow.step) <= reg->stepsTotal);
    }

    return follow;
}

/**
 * @brief 根据指定像素坐标初始化追踪起始信息
 */
static DmtxFollow followSeekLoc(DmtxDecode *dec, DmtxPixelLoc loc)
{
    DmtxFollow follow;

    follow.loc = loc;
    follow.step = 0;
    follow.ptr = dmtxDecodeGetCache(dec, follow.loc.x, follow.loc.y);
    DmtxAssert(follow.ptr != NULL);
    follow.neighbor = *follow.ptr;

    return follow;
}

/**
 * @brief 寻找followBeg的下一个点
 *
 * @param dec 解码上下文，包含缓存访问等信息
 * @param reg 当前处理的区域信息，包括追踪步数、跳跃目标位置等
 * @param followBeg 起始点
 * @param sign 追踪方向，+1 表示正向追踪，-1 表示负向追踪
 */
static DmtxFollow followStep(DmtxDecode *dec, DmtxRegion *reg, DmtxFollow followBeg, int sign)
{
    int patternIdx;
    int stepMod;
    int factor;
    DmtxFollow follow;

    DmtxAssert(abs(sign) == 1);
    DmtxAssert((int)(followBeg.neighbor & 0x40) != 0x00);

    factor = reg->stepsTotal + 1;
    if (sign > 0) {
        stepMod = (factor + (followBeg.step % factor)) % factor;
    } else {
        stepMod = (factor - (followBeg.step % factor)) % factor;
    }

    /* End of positive trail -- magic jump */
    if (sign > 0 && stepMod == reg->jumpToNeg) {
        follow.loc = reg->finalNeg;
    }
    /* End of negative trail -- magic jump */
    else if (sign < 0 && stepMod == reg->jumpToPos) {
        follow.loc = reg->finalPos;
    }
    /* Trail in progress -- normal jump */
    else {
        patternIdx = (sign < 0) ? followBeg.neighbor & 0x07 : ((followBeg.neighbor & 0x38) >> 3);
        follow.loc.x = followBeg.loc.x + dmtxPatternX[patternIdx];
        follow.loc.y = followBeg.loc.y + dmtxPatternY[patternIdx];
    }

    follow.step = followBeg.step + sign;
    follow.ptr = dmtxDecodeGetCache(dec, follow.loc.x, follow.loc.y);
    DmtxAssert(follow.ptr != NULL);
    follow.neighbor = *follow.ptr;

    return follow;
}

/**
 *
 *
 */
static DmtxFollow followStep2(DmtxDecode *dec, DmtxFollow followBeg, int sign)
{
    int patternIdx;
    DmtxFollow follow;

    DmtxAssert(abs(sign) == 1);
    DmtxAssert((int)(followBeg.neighbor & 0x40) != 0x00);

    patternIdx = (sign < 0) ? followBeg.neighbor & 0x07 : ((followBeg.neighbor & 0x38) >> 3);
    follow.loc.x = followBeg.loc.x + dmtxPatternX[patternIdx];
    follow.loc.y = followBeg.loc.y + dmtxPatternY[patternIdx];

    follow.step = followBeg.step + sign;
    follow.ptr = dmtxDecodeGetCache(dec, follow.loc.x, follow.loc.y);
    DmtxAssert(follow.ptr != NULL);
    follow.neighbor = *follow.ptr;

    return follow;
}

/**
 * @brief 从flowBegin点出发，分别从正负方向寻找连续的边界线
 *
 * vaiiiooo
 * --------
 * 0x80 v = visited bit
 * 0x40 a = assigned bit
 * 0x38 u = 3 bits points upstream 0-7
 * 0x07 d = 3 bits points downstream 0-7
 *
 */
static DmtxPassFail trailBlazeContinuous(DmtxDecode *dec, DmtxRegion *reg, DmtxPointFlow flowBegin, int maxDiagonal)
{
    int posAssigns, negAssigns, clears;
    int sign;  // 方向标志，+1为正向，-1为负向
    int steps;
    unsigned char *cache, *cacheNext, *cacheBeg;
    DmtxPointFlow flow, flowNext;
    DmtxPixelLoc boundMin, boundMax;

    boundMin = boundMax = flowBegin.loc;
    cacheBeg = dmtxDecodeGetCache(dec, flowBegin.loc.x, flowBegin.loc.y);
    if (cacheBeg == NULL) {
        return DmtxFail;
    }
    *cacheBeg = (0x80 | 0x40); /* Mark location as visited and assigned */

    reg->flowBegin = flowBegin;

    posAssigns = negAssigns = 0;
    for (sign = 1; sign >= -1; sign -= 2) {  // 分别进行正向和负向探索
        flow = flowBegin;
        cache = cacheBeg;

        for (steps = 0;; steps++) {
            // 检查是否超过最大对角线限制
            if (maxDiagonal != DmtxUndefined &&
                (boundMax.x - boundMin.x > maxDiagonal || boundMax.y - boundMin.y > maxDiagonal)) {
                break;
            }

            /* 寻找梯度最大的下一个点 */
            flowNext = findStrongestNeighbor(dec, flow, sign);
            if (flowNext.mag < 50) {
                break;
            }

            /* Get the neighbor's cache location */
            cacheNext = dmtxDecodeGetCache(dec, flowNext.loc.x, flowNext.loc.y);
            if (cacheNext == NULL) {
                break;
            }
            DmtxAssert(!(*cacheNext & 0x80));

            /* Mark departure from current location. If flowing downstream
             * (sign < 0) then departure vector here is the arrival vector
             * of the next location. Upstream flow uses the opposite rule. */
            *cache |= (sign < 0) ? flowNext.arrive : flowNext.arrive << 3;

            /* Mark known direction for next location */
            /* If testing downstream (sign < 0) then next upstream is opposite of next arrival */
            /* If testing upstream (sign > 0) then next downstream is opposite of next arrival */
            *cacheNext = (sign < 0) ? (((flowNext.arrive + 4) % 8) << 3) : ((flowNext.arrive + 4) % 8);
            *cacheNext |= (0x80 | 0x40); /* Mark location as visited and assigned */
            if (sign > 0) {
                posAssigns++;
            } else {
                negAssigns++;
            }
            cache = cacheNext;
            flow = flowNext;

            if (flow.loc.x > boundMax.x) {
                boundMax.x = flow.loc.x;
            } else if (flow.loc.x < boundMin.x) {
                boundMin.x = flow.loc.x;
            }
            if (flow.loc.y > boundMax.y) {
                boundMax.y = flow.loc.y;
            } else if (flow.loc.y < boundMin.y) {
                boundMin.y = flow.loc.y;
            }

            if (cbPlotPoint) {
                cbPlotPoint(flow.loc, (sign > 0) ? 0.0F /*红*/ : 180.0F /*青*/, 1, 2);
            }
        }

        if (sign > 0) {
            reg->finalPos = flow.loc;
            reg->jumpToNeg = steps;
        } else {
            reg->finalNeg = flow.loc;
            reg->jumpToPos = steps;
        }
    }
    reg->stepsTotal = reg->jumpToPos + reg->jumpToNeg;
    reg->boundMin = boundMin;
    reg->boundMax = boundMax;

    /* Clear "visited" bit from trail */
    clears = trailClear(dec, reg, 0x80);
    DmtxAssert(posAssigns + negAssigns == clears - 1);

    /* XXX clean this up ... redundant test above */
    if (maxDiagonal != DmtxUndefined &&
        (boundMax.x - boundMin.x > maxDiagonal || boundMax.y - boundMin.y > maxDiagonal)) {
        return DmtxFail;
    }

    return DmtxPass;
}

/**
 * recives bresline, and follows strongest neighbor unless it involves
 * ratcheting bresline inward or backward (although back + outward is allowed).
 *
 */
static int trailBlazeGapped(DmtxDecode *dec, DmtxRegion *reg, DmtxBresLine line, int streamDir)
{
    unsigned char *beforeCache, *afterCache;
    DmtxBoolean onEdge;
    int distSq, distSqMax;
    int travel, outward;
    int xDiff, yDiff;
    int steps;
    int stepDir, dirMap[] = {0, 1, 2, 7, 8, 3, 6, 5, 4};
    DmtxPassFail err;
    DmtxPixelLoc beforeStep, afterStep;
    DmtxPointFlow flow, flowNext;
    DmtxPixelLoc loc0;
    int xStep, yStep;

    loc0 = line.loc;
    flow = getPointFlow(dec, reg->flowBegin.plane, loc0, dmtxNeighborNone);
    distSqMax = (line.xDelta * line.xDelta) + (line.yDelta * line.yDelta);
    steps = 0;
    onEdge = DmtxTrue;

    beforeStep = loc0;
    beforeCache = dmtxDecodeGetCache(dec, loc0.x, loc0.y);
    if (beforeCache == NULL) {
        return DmtxFail;
    }
    *beforeCache = 0x00; /* probably should just overwrite one direction */

    do {
        if (onEdge == DmtxTrue) {
            flowNext = findStrongestNeighbor(dec, flow, streamDir);
            if (flowNext.mag == DmtxUndefined) {
                break;
            }

            err = bresLineGetStep(line, flowNext.loc, &travel, &outward);
            if (err == DmtxFail) {
                return DmtxFail;
            }

            if (flowNext.mag < 50 || outward < 0 || (outward == 0 && travel < 0)) {
                onEdge = DmtxFalse;
            } else {
                bresLineStep(&line, travel, outward);
                flow = flowNext;
            }
        }

        if (onEdge == DmtxFalse) {
            bresLineStep(&line, 1, 0);
            flow = getPointFlow(dec, reg->flowBegin.plane, line.loc, dmtxNeighborNone);
            if (flow.mag > 50) {
                onEdge = DmtxTrue;
            }
        }

        afterStep = line.loc;
        afterCache = dmtxDecodeGetCache(dec, afterStep.x, afterStep.y);
        if (afterCache == NULL) {
            break;
        }

        /* Determine step direction using pure magic */
        xStep = afterStep.x - beforeStep.x;
        yStep = afterStep.y - beforeStep.y;
        DmtxAssert(abs(xStep) <= 1 && abs(yStep) <= 1);
        stepDir = dirMap[3 * yStep + xStep + 4];
        DmtxAssert(stepDir != 8);

        if (streamDir < 0) {
            *beforeCache |= (0x40 | stepDir);
            *afterCache = (((stepDir + 4) % 8) << 3);
        } else {
            *beforeCache |= (0x40 | (stepDir << 3));
            *afterCache = ((stepDir + 4) % 8);
        }

        /* Guaranteed to have taken one step since top of loop */
        xDiff = line.loc.x - loc0.x;
        yDiff = line.loc.y - loc0.y;
        distSq = (xDiff * xDiff) + (yDiff * yDiff);

        beforeStep = line.loc;
        beforeCache = afterCache;
        steps++;

    } while (distSq < distSqMax);

    return steps;
}

/**
 *
 *
 */
static int trailClear(DmtxDecode *dec, DmtxRegion *reg, int clearMask)
{
    int clears;
    DmtxFollow follow;

    DmtxAssert((clearMask | 0xff) == 0xff);

    /* Clear "visited" bit from trail */
    clears = 0;
    follow = followSeek(dec, reg, 0);
    while (abs(follow.step) <= reg->stepsTotal) {
        DmtxAssert((int)(*follow.ptr & clearMask) != 0x00);
        *follow.ptr &= (clearMask ^ 0xff);
        follow = followStep(dec, reg, follow, +1);
        clears++;
    }

    return clears;
}

/**
 * @brief 查找最佳实线
 *
 * 该函数用于在给定的解码上下文和区域内，根据霍夫变换找到最佳实线。
 * 它考虑了路径流的方向，并尝试避免指定的角度。
 *
 * @param dec 解码上下文
 * @param reg 区域信息
 * @param step0 起始步长
 * @param step1 结束步长
 * @param streamDir 流的方向
 * @param houghAvoid 霍夫变换中要避免的角度
 *
 * @return DmtxBestLine 返回最佳实线的信息
 */
static DmtxBestLine findBestSolidLine(DmtxDecode *dec, DmtxRegion *reg, int step0, int step1, int streamDir,
                                      int houghAvoid)
{
    int hough[3][DMTX_HOUGH_RES] = {{0}};
    int houghMin, houghMax;
    char houghTest[DMTX_HOUGH_RES];
    int i;
    int step;
    int sign;
    int tripSteps;
    int angleBest;
    int hOffset, hOffsetBest;
    int xDiff, yDiff;
    int dH;
    DmtxRay2 rH;
    DmtxFollow follow;
    DmtxBestLine line;
    DmtxPixelLoc rHp;

    memset(&line, 0x00, sizeof(DmtxBestLine));
    memset(&rH, 0x00, sizeof(DmtxRay2));
    angleBest = 0;
    hOffset = hOffsetBest = 0;

    sign = 0;

    /* Always follow path flowing away from the trail start */
    if (step0 != 0) {
        if (step0 > 0) {
            sign = +1;
            tripSteps = (step1 - step0 + reg->stepsTotal) % reg->stepsTotal;
        } else {
            sign = -1;
            tripSteps = (step0 - step1 + reg->stepsTotal) % reg->stepsTotal;
        }
        if (tripSteps == 0) {
            tripSteps = reg->stepsTotal;
        }
    } else if (step1 != 0) {
        sign = (step1 > 0) ? +1 : -1;
        tripSteps = abs(step1);
    } else if (step1 == 0) {
        sign = +1;
        tripSteps = reg->stepsTotal;
    }
    DmtxAssert(sign == streamDir);

    follow = followSeek(dec, reg, step0);
    rHp = follow.loc;

    line.stepBeg = line.stepPos = line.stepNeg = step0;
    line.locBeg = follow.loc;
    line.locPos = follow.loc;
    line.locNeg = follow.loc;

    /* Predetermine which angles to test */
    for (i = 0; i < DMTX_HOUGH_RES; i++) {
        if (houghAvoid == DmtxUndefined) {
            houghTest[i] = 1;
        } else {
            houghMin = (houghAvoid + DMTX_HOUGH_RES / 6) % DMTX_HOUGH_RES;
            houghMax = (houghAvoid - DMTX_HOUGH_RES / 6 + DMTX_HOUGH_RES) % DMTX_HOUGH_RES;
            if (houghMin > houghMax) {
                houghTest[i] = (i > houghMin || i < houghMax) ? 1 : 0;
            } else {
                houghTest[i] = (i > houghMin && i < houghMax) ? 1 : 0;
            }
        }
    }

    /* Test each angle for steps along path */
    for (step = 0; step < tripSteps; step++) {
        xDiff = follow.loc.x - rHp.x;
        yDiff = follow.loc.y - rHp.y;

        /* Increment Hough accumulator */
        for (i = 0; i < DMTX_HOUGH_RES; i++) {
            if ((int)houghTest[i] == 0) {
                continue;
            }

            dH = (rHvX[i] * yDiff) - (rHvY[i] * xDiff);
            if (dH >= -384 && dH <= 384) {
                if (dH > 128) {
                    hOffset = 2;
                } else if (dH >= -128) {
                    hOffset = 1;
                } else {
                    hOffset = 0;
                }

                hough[hOffset][i]++;

                /* New angle takes over lead */
                if (hough[hOffset][i] > hough[hOffsetBest][angleBest]) {
                    angleBest = i;
                    hOffsetBest = hOffset;
                }
            }
        }

        if (cbPlotPoint) {
            cbPlotPoint(follow.loc, (sign > 1) ? 120.0F + step : 300.0F + step, 1, 2);
        }

        follow = followStep(dec, reg, follow, sign);
    }

    line.angle = angleBest;
    line.hOffset = hOffsetBest;
    line.mag = hough[hOffsetBest][angleBest];

    return line;
}

/**
 *
 *
 */
static DmtxBestLine findBestSolidLine2(DmtxDecode *dec, DmtxPixelLoc loc0, int tripSteps, int sign, int houghAvoid)
{
    int hough[3][DMTX_HOUGH_RES] = {{0}};
    int houghMin, houghMax;
    char houghTest[DMTX_HOUGH_RES];
    int i;
    int step;
    int angleBest;
    int hOffset, hOffsetBest;
    int xDiff, yDiff;
    int dH;
    DmtxRay2 rH;
    DmtxBestLine line;
    DmtxPixelLoc rHp;
    DmtxFollow follow;

    memset(&line, 0x00, sizeof(DmtxBestLine));
    memset(&rH, 0x00, sizeof(DmtxRay2));
    angleBest = 0;
    hOffset = hOffsetBest = 0;

    follow = followSeekLoc(dec, loc0);
    rHp = line.locBeg = line.locPos = line.locNeg = follow.loc;
    line.stepBeg = line.stepPos = line.stepNeg = 0;

    /* Predetermine which angles to test */
    for (i = 0; i < DMTX_HOUGH_RES; i++) {
        if (houghAvoid == DmtxUndefined) {
            houghTest[i] = 1;
        } else {
            houghMin = (houghAvoid + DMTX_HOUGH_RES / 6) % DMTX_HOUGH_RES;
            houghMax = (houghAvoid - DMTX_HOUGH_RES / 6 + DMTX_HOUGH_RES) % DMTX_HOUGH_RES;
            if (houghMin > houghMax) {
                houghTest[i] = (i > houghMin || i < houghMax) ? 1 : 0;
            } else {
                houghTest[i] = (i > houghMin && i < houghMax) ? 1 : 0;
            }
        }
    }

    /* Test each angle for steps along path */
    for (step = 0; step < tripSteps; step++) {
        xDiff = follow.loc.x - rHp.x;
        yDiff = follow.loc.y - rHp.y;

        /* Increment Hough accumulator */
        for (i = 0; i < DMTX_HOUGH_RES; i++) {
            if ((int)houghTest[i] == 0) {
                continue;
            }

            dH = (rHvX[i] * yDiff) - (rHvY[i] * xDiff);
            if (dH >= -384 && dH <= 384) {
                if (dH > 128) {
                    hOffset = 2;
                } else if (dH >= -128) {
                    hOffset = 1;
                } else {
                    hOffset = 0;
                }

                hough[hOffset][i]++;

                /* New angle takes over lead */
                if (hough[hOffset][i] > hough[hOffsetBest][angleBest]) {
                    angleBest = i;
                    hOffsetBest = hOffset;
                }
            }
        }

        if (cbPlotPoint) {
            cbPlotPoint(follow.loc, (sign > 1) ? 300.0F /*品红*/ : 120.0F /*绿*/, 1, 2);
        }

        follow = followStep2(dec, follow, sign);
    }

    line.angle = angleBest;
    line.hOffset = hOffsetBest;
    line.mag = hough[hOffsetBest][angleBest];

    return line;
}

/**
 *
 *
 */
static DmtxPassFail findTravelLimits(DmtxDecode *dec, DmtxRegion *reg, DmtxBestLine *line)
{
    int i;
    int distSq, distSqMax;
    int xDiff, yDiff;
    int posRunning, negRunning;
    int posTravel, negTravel;
    int posWander, posWanderMin, posWanderMax, posWanderMinLock, posWanderMaxLock;
    int negWander, negWanderMin, negWanderMax, negWanderMinLock, negWanderMaxLock;
    int cosAngle, sinAngle;
    DmtxFollow followPos, followNeg;
    DmtxPixelLoc loc0, posMax, negMax;

    /* line->stepBeg is already known to sit on the best Hough line */
    followPos = followNeg = followSeek(dec, reg, line->stepBeg);
    loc0 = followPos.loc;

    cosAngle = rHvX[line->angle];
    sinAngle = rHvY[line->angle];

    distSqMax = 0;
    posMax = negMax = followPos.loc;

    posTravel = negTravel = 0;
    posWander = posWanderMin = posWanderMax = posWanderMinLock = posWanderMaxLock = 0;
    negWander = negWanderMin = negWanderMax = negWanderMinLock = negWanderMaxLock = 0;

    for (i = 0; i < reg->stepsTotal / 2; i++) {
        posRunning = (int)(i < 10 || abs(posWander) < abs(posTravel));
        negRunning = (int)(i < 10 || abs(negWander) < abs(negTravel));

        if (posRunning != 0) {
            xDiff = followPos.loc.x - loc0.x;
            yDiff = followPos.loc.y - loc0.y;
            posTravel = (cosAngle * xDiff) + (sinAngle * yDiff);
            posWander = (cosAngle * yDiff) - (sinAngle * xDiff);

            if (posWander >= -3 * 256 && posWander <= 3 * 256) {
                distSq = (int)distanceSquared(followPos.loc, negMax);
                if (distSq > distSqMax) {
                    posMax = followPos.loc;  // 更新
                    distSqMax = distSq;
                    line->stepPos = followPos.step;
                    line->locPos = followPos.loc;
                    posWanderMinLock = posWanderMin;
                    posWanderMaxLock = posWanderMax;
                }
            } else {
                posWanderMin = min(posWanderMin, posWander);
                posWanderMax = max(posWanderMax, posWander);
            }
        } else if (!negRunning) {
            break;
        }

        if (negRunning != 0) {
            xDiff = followNeg.loc.x - loc0.x;
            yDiff = followNeg.loc.y - loc0.y;
            negTravel = (cosAngle * xDiff) + (sinAngle * yDiff);
            negWander = (cosAngle * yDiff) - (sinAngle * xDiff);

            if (negWander >= -3 * 256 && negWander < 3 * 256) {
                distSq = (int)distanceSquared(followNeg.loc, posMax);
                if (distSq > distSqMax) {
                    negMax = followNeg.loc;  // 更新
                    distSqMax = distSq;
                    line->stepNeg = followNeg.step;
                    line->locNeg = followNeg.loc;
                    negWanderMinLock = negWanderMin;
                    negWanderMaxLock = negWanderMax;
                }
            } else {
                negWanderMin = min(negWanderMin, negWander);
                negWanderMax = max(negWanderMax, negWander);
            }
        } else if (!posRunning) {
            break;
        }

        if (cbPlotPoint) {
            cbPlotPoint(followPos.loc, 60.0F /*黄*/, 1, 2);
            cbPlotPoint(followNeg.loc, 240.0F /*蓝*/, 1, 2);
        }

        followPos = followStep(dec, reg, followPos, +1);
        followNeg = followStep(dec, reg, followNeg, -1);
    }
    line->devn = max(posWanderMaxLock - posWanderMinLock, negWanderMaxLock - negWanderMinLock) / 256;
    line->distSq = distSqMax;

    if (cbPlotPoint) {
        cbPlotPoint(posMax, 120.0F /*绿*/, 1, 1);
        cbPlotPoint(negMax, 120.0F /*绿*/, 1, 1);
    }

    return DmtxPass;
}

/**
 *
 *
 */
static DmtxPassFail matrixRegionAlignCalibEdge(DmtxDecode *dec, DmtxRegion *reg, int edgeLoc)
{
    int streamDir;
    int steps;
    int avoidAngle;
    int symbolShape;
    DmtxVector2 pTmp;
    DmtxPixelLoc loc0, loc1, locOrigin;
    DmtxBresLine line;
    DmtxFollow follow;
    DmtxBestLine bestLine;

    /* 确定原点的像素坐标 */
    pTmp.x = 0.0;
    pTmp.y = 0.0;
    dmtxMatrix3VMultiplyBy(&pTmp, reg->fit2raw);
    locOrigin.x = (int)(pTmp.x + 0.5);
    locOrigin.y = (int)(pTmp.y + 0.5);

    if (dec->sizeIdxExpected == DmtxSymbolSquareAuto ||
        (dec->sizeIdxExpected >= DmtxSymbol10x10 && dec->sizeIdxExpected <= DmtxSymbol144x144)) {
        symbolShape = DmtxSymbolSquareAuto;
    } else if (dec->sizeIdxExpected == DmtxSymbolRectAuto ||
               (dec->sizeIdxExpected >= DmtxSymbol8x18 && dec->sizeIdxExpected <= DmtxSymbol16x48)) {
        symbolShape = DmtxSymbolRectAuto;
    } else {
        symbolShape = DmtxSymbolShapeAuto;
    }

    /* Determine end locations of test line */
    if (edgeLoc == DmtxEdgeTop) {
        streamDir = reg->polarity * -1;
        avoidAngle = reg->leftLine.angle;
        follow = followSeekLoc(dec, reg->locT);
        pTmp.x = 0.8;
        pTmp.y = (symbolShape == DmtxSymbolRectAuto) ? 0.2 : 0.6;
    } else {
        DmtxAssert(edgeLoc == DmtxEdgeRight);
        streamDir = reg->polarity;
        avoidAngle = reg->bottomLine.angle;
        follow = followSeekLoc(dec, reg->locR);
        pTmp.x = (symbolShape == DmtxSymbolSquareAuto) ? 0.7 : 0.9;
        pTmp.y = 0.8;
    }

    dmtxMatrix3VMultiplyBy(&pTmp, reg->fit2raw);
    loc1.x = (int)(pTmp.x + 0.5);
    loc1.y = (int)(pTmp.y + 0.5);

    loc0 = follow.loc;
    line = bresLineInit(loc0, loc1, locOrigin);
    steps = trailBlazeGapped(dec, reg, line, streamDir);

    bestLine = findBestSolidLine2(dec, loc0, steps, streamDir, avoidAngle);
    if (bestLine.mag < 5) {
        ;
    }

    if (edgeLoc == DmtxEdgeTop) {
        reg->topKnown = 1;
        reg->topAngle = bestLine.angle;
        reg->topLoc = bestLine.locBeg;
    } else {
        reg->rightKnown = 1;
        reg->rightAngle = bestLine.angle;
        reg->rightLoc = bestLine.locBeg;
    }

    return DmtxPass;
}

/**
 *
 *
 */
static DmtxBresLine bresLineInit(DmtxPixelLoc loc0, DmtxPixelLoc loc1, DmtxPixelLoc locInside)
{
    int cp;
    DmtxBresLine line;
    DmtxPixelLoc *locBeg, *locEnd;

    /* XXX Verify that loc0 and loc1 are inbounds */

    /* Values that stay the same after initialization */
    line.loc0 = loc0;
    line.loc1 = loc1;
    line.xStep = (loc0.x < loc1.x) ? +1 : -1;
    line.yStep = (loc0.y < loc1.y) ? +1 : -1;
    line.xDelta = abs(loc1.x - loc0.x);
    line.yDelta = abs(loc1.y - loc0.y);
    line.steep = (int)(line.yDelta > line.xDelta);

    /* Take cross product to determine outward step */
    if (line.steep != 0) {
        /* Point first vector up to get correct sign */
        if (loc0.y < loc1.y) {
            locBeg = &loc0;
            locEnd = &loc1;
        } else {
            locBeg = &loc1;
            locEnd = &loc0;
        }
        cp = (((locEnd->x - locBeg->x) * (locInside.y - locEnd->y)) -
              ((locEnd->y - locBeg->y) * (locInside.x - locEnd->x)));

        line.xOut = (cp > 0) ? +1 : -1;
        line.yOut = 0;
    } else {
        /* Point first vector left to get correct sign */
        if (loc0.x > loc1.x) {
            locBeg = &loc0;
            locEnd = &loc1;
        } else {
            locBeg = &loc1;
            locEnd = &loc0;
        }
        cp = (((locEnd->x - locBeg->x) * (locInside.y - locEnd->y)) -
              ((locEnd->y - locBeg->y) * (locInside.x - locEnd->x)));

        line.xOut = 0;
        line.yOut = (cp > 0) ? +1 : -1;
    }

    /* Values that change while stepping through line */
    line.loc = loc0;
    line.travel = 0;
    line.outward = 0;
    line.error = (line.steep) ? line.yDelta / 2 : line.xDelta / 2;

    if (cbPlotPoint) {
        cbPlotPoint(loc0, 240.0F /*蓝*/, 1, 1);
        cbPlotPoint(loc1, 240.0F /*蓝*/, 1, 1);
    }

    return line;
}

/**
 *
 *
 */
static DmtxPassFail bresLineGetStep(DmtxBresLine line, DmtxPixelLoc target, int *travel, int *outward)
{
    /* Determine necessary step along and outward from Bresenham line */
    if (line.steep != 0) {
        *travel = (line.yStep > 0) ? target.y - line.loc.y : line.loc.y - target.y;
        bresLineStep(&line, *travel, 0);
        *outward = (line.xOut > 0) ? target.x - line.loc.x : line.loc.x - target.x;
        DmtxAssert(line.yOut == 0);
    } else {
        *travel = (line.xStep > 0) ? target.x - line.loc.x : line.loc.x - target.x;
        bresLineStep(&line, *travel, 0);
        *outward = (line.yOut > 0) ? target.y - line.loc.y : line.loc.y - target.y;
        DmtxAssert(line.xOut == 0);
    }

    return DmtxPass;
}

/**
 *
 *
 */
static DmtxPassFail bresLineStep(DmtxBresLine *line, int travel, int outward)
{
    int i;
    DmtxBresLine lineNew;

    lineNew = *line;

    DmtxAssert(abs(travel) < 2);
    DmtxAssert(abs(outward) >= 0);

    /* Perform forward step */
    if (travel > 0) {
        lineNew.travel++;
        if (lineNew.steep != 0) {
            lineNew.loc.y += lineNew.yStep;
            lineNew.error -= lineNew.xDelta;
            if (lineNew.error < 0) {
                lineNew.loc.x += lineNew.xStep;
                lineNew.error += lineNew.yDelta;
            }
        } else {
            lineNew.loc.x += lineNew.xStep;
            lineNew.error -= lineNew.yDelta;
            if (lineNew.error < 0) {
                lineNew.loc.y += lineNew.yStep;
                lineNew.error += lineNew.xDelta;
            }
        }
    } else if (travel < 0) {
        lineNew.travel--;
        if (lineNew.steep != 0) {
            lineNew.loc.y -= lineNew.yStep;
            lineNew.error += lineNew.xDelta;
            if (lineNew.error >= lineNew.yDelta) {
                lineNew.loc.x -= lineNew.xStep;
                lineNew.error -= lineNew.yDelta;
            }
        } else {
            lineNew.loc.x -= lineNew.xStep;
            lineNew.error += lineNew.yDelta;
            if (lineNew.error >= lineNew.xDelta) {
                lineNew.loc.y -= lineNew.yStep;
                lineNew.error -= lineNew.xDelta;
            }
        }
    }

    for (i = 0; i < outward; i++) {
        /* Outward steps */
        lineNew.outward++;
        lineNew.loc.x += lineNew.xOut;
        lineNew.loc.y += lineNew.yOut;
    }

    *line = lineNew;

    return DmtxPass;
}

/**
 *
 *
 */
#ifdef NOTDEFINED
static void WriteDiagnosticImage(DmtxDecode *dec, DmtxRegion *reg, char *imagePath)
{
    int row, col;
    int width, height;
    unsigned char *cache;
    int rgb[3];
    FILE *fp;
    DmtxVector2 p;
    DmtxImage *img;

    DmtxAssert(reg != NULL);

    fp = fopen(imagePath, "wb");
    if (fp == NULL) {
        exit(3);
    }

    width = dmtxDecodeGetProp(dec, DmtxPropWidth);
    height = dmtxDecodeGetProp(dec->image, DmtxPropHeight);

    img = dmtxImageCreate(NULL, width, height, DmtxPack24bppRGB);

    /* Populate image */
    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            cache = dmtxDecodeGetCache(dec, col, row);
            if (cache == NULL) {
                rgb[0] = 0;
                rgb[1] = 0;
                rgb[2] = 128;
            } else {
                dmtxDecodeGetPixelValue(dec, col, row, 0, &rgb[0]);
                dmtxDecodeGetPixelValue(dec, col, row, 1, &rgb[1]);
                dmtxDecodeGetPixelValue(dec, col, row, 2, &rgb[2]);

                p.X = col;
                p.Y = row;
                dmtxMatrix3VMultiplyBy(&p, reg->raw2fit);

                if (p.X < 0.0 || p.X > 1.0 || p.Y < 0.0 || p.Y > 1.0) {
                    rgb[0] = 0;
                    rgb[1] = 0;
                    rgb[2] = 128;
                } else if (p.X + p.Y > 1.0) {
                    rgb[0] += (0.4 * (255 - rgb[0]));
                    rgb[1] += (0.4 * (255 - rgb[1]));
                    rgb[2] += (0.4 * (255 - rgb[2]));
                }
            }

            dmtxImageSetRgb(img, col, row, rgb);
        }
    }

    /* Write additional markers */
    rgb[0] = 255;
    rgb[1] = 0;
    rgb[2] = 0;
    dmtxImageSetRgb(img, reg->toploc.x, reg->toploc.y, rgb);
    dmtxImageSetRgb(img, reg->rightloc.x, reg->rightloc.y, rgb);

    /* Write image to PNM file */
    fprintf(fp, "P6\n%d %d\n255\n", width, height);
    for (row = height - 1; row >= 0; row--) {
        for (col = 0; col < width; col++) {
            dmtxImageGetRgb(img, col, row, rgb);
            fwrite(rgb, sizeof(char), 3, fp);
        }
    }

    dmtxImageDestroy(&img);

    fclose(fp);
}
#endif
