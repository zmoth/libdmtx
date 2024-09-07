/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2008, 2009 Mike Laughton. All rights reserved.
 * Copyright 2012-2016 Vadim A. Misbakh-Soloviov. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact:
 * Vadim A. Misbakh-Soloviov <dmtx@mva.name>
 * Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxscangrid.c
 * \brief Scan grid tracking
 */

#include <assert.h>
#include <string.h>

#include "dmtx.h"
#include "dmtxstatic.h"

/**
 * \brief  Initialize scan grid pattern
 * \param  dec
 * \return Initialized grid
 */
static DmtxScanGrid initScanGrid(DmtxDecode *dec)
{
    int scale, smallestFeature;
    int xExtent, yExtent, maxExtent;
    int extent;
    DmtxScanGrid grid;

    memset(&grid, 0x00, sizeof(DmtxScanGrid));

    scale = dmtxDecodeGetProp(dec, DmtxPropScale);
    smallestFeature = dmtxDecodeGetProp(dec, DmtxPropScanGap) / scale;

    grid.xMin = dmtxDecodeGetProp(dec, DmtxPropXmin);
    grid.xMax = dmtxDecodeGetProp(dec, DmtxPropXmax);
    grid.yMin = dmtxDecodeGetProp(dec, DmtxPropYmin);
    grid.yMax = dmtxDecodeGetProp(dec, DmtxPropYmax);

    /* Values that get set once */
    xExtent = grid.xMax - grid.xMin;
    yExtent = grid.yMax - grid.yMin;
    maxExtent = max(xExtent, yExtent);

    DmtxAssert(maxExtent > 1);

    for (extent = 1; extent < maxExtent; extent = ((extent + 1) * 2) - 1) {
        if (extent <= smallestFeature) {
            grid.minExtent = extent;
        }
    }

    grid.maxExtent = extent;

    grid.xOffset = (grid.xMin + grid.xMax - grid.maxExtent) / 2;
    grid.yOffset = (grid.yMin + grid.yMax - grid.maxExtent) / 2;

    /* Values that get reset for every level */
    grid.total = 1;
    grid.extent = grid.maxExtent;

    setDerivedFields(&grid);

    return grid;
}

/**
 * \brief  Return the next good location (which may be the current location),
 *         and advance grid progress one position beyond that. If no good
 *         locations remain then return DmtxRangeEnd.
 * \param  grid
 * \return void
 */
static int popGridLocation(DmtxScanGrid *grid, DmtxPixelLoc *locPtr)
{
    int locStatus;

    do {
        locStatus = getGridCoordinates(grid, locPtr);

        /* Always leave grid pointing at next available location */
        grid->pixelCount++;

    } while (locStatus == DmtxRangeBad);

    return locStatus;
}

/**
 * \brief  Extract current grid position in pixel coordinates and return
 *         whether location is good, bad, or end
 * \param  grid
 * \return Pixel location
 */
static int getGridCoordinates(DmtxScanGrid *grid, DmtxPixelLoc *locPtr)
{
    int count, half, quarter;
    DmtxPixelLoc loc;

    /* Initially pixelCount may fall beyond acceptable limits. Update grid
     * state before testing coordinates */

    /* Jump to next cross pattern horizontally if current column is done */
    if (grid->pixelCount >= grid->pixelTotal) {
        grid->pixelCount = 0;
        grid->xCenter += grid->jumpSize;
    }

    /* Jump to next cross pattern vertically if current row is done */
    if (grid->xCenter > grid->maxExtent) {
        grid->xCenter = grid->startPos;
        grid->yCenter += grid->jumpSize;
    }

    /* Increment level when vertical step goes too far */
    if (grid->yCenter > grid->maxExtent) {
        grid->total *= 4;
        grid->extent /= 2;
        setDerivedFields(grid);
    }

    if (grid->extent == 0 || grid->extent < grid->minExtent) {
        locPtr->x = locPtr->y = -1;
        return DmtxRangeEnd;
    }

    count = grid->pixelCount;

    DmtxAssert(count < grid->pixelTotal);

    if (count == grid->pixelTotal - 1) {
        /* center pixel */
        loc.x = grid->xCenter;
        loc.y = grid->yCenter;
    } else {
        half = grid->pixelTotal / 2;
        quarter = half / 2;

        /* horizontal portion */
        if (count < half) {
            loc.x = grid->xCenter + ((count < quarter) ? (count - quarter) : (half - count));
            loc.y = grid->yCenter;
        }
        /* vertical portion */
        else {
            count -= half;
            loc.x = grid->xCenter;
            loc.y = grid->yCenter + ((count < quarter) ? (count - quarter) : (half - count));
        }
    }

    loc.x += grid->xOffset;
    loc.y += grid->yOffset;

    *locPtr = loc;

    if (loc.x < grid->xMin || loc.x > grid->xMax || loc.y < grid->yMin || loc.y > grid->yMax) {
        return DmtxRangeBad;
    }

    return DmtxRangeGood;
}

/**
 * \brief  Update derived fields based on current state
 * \param  grid
 * \return void
 */
static void setDerivedFields(DmtxScanGrid *grid)
{
    grid->jumpSize = grid->extent + 1;
    grid->pixelTotal = 2 * grid->extent - 1;
    grid->startPos = grid->extent / 2;
    grid->pixelCount = 0;
    grid->xCenter = grid->yCenter = grid->startPos;
}
