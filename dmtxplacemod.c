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
 * \file dmtxplacemod.c
 * \brief Data Matrix module placement
 */

#include <assert.h>

#include "dmtx.h"
#include "dmtxstatic.h"

/**
 * receives symbol row and col and returns status
 * DmtxModuleOn / !DmtxModuleOn (DmtxModuleOff)
 * DmtxModuleAssigned
 * DmtxModuleVisited
 * DmtxModuleData / !DmtxModuleData (DmtxModuleAlignment)
 * row and col are expressed in symbol coordinates, so (0,0) is the intersection of the "L"
 */
int dmtxSymbolModuleStatus(DmtxMessage *message, int sizeIdx, int symbolRow, int symbolCol)
{
    int symbolRowReverse;
    int mappingRow, mappingCol;
    int dataRegionRows, dataRegionCols;
    int symbolRows, mappingCols;

    dataRegionRows = dmtxGetSymbolAttribute(DmtxSymAttribDataRegionRows, sizeIdx);
    dataRegionCols = dmtxGetSymbolAttribute(DmtxSymAttribDataRegionCols, sizeIdx);
    symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, sizeIdx);
    mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, sizeIdx);

    symbolRowReverse = symbolRows - symbolRow - 1;
    mappingRow = symbolRowReverse - 1 - 2 * (symbolRowReverse / (dataRegionRows + 2));
    mappingCol = symbolCol - 1 - 2 * (symbolCol / (dataRegionCols + 2));

    /* Solid portion of alignment patterns */
    if (symbolRow % (dataRegionRows + 2) == 0 || symbolCol % (dataRegionCols + 2) == 0) {
        return (DmtxModuleOnRGB | (!DmtxModuleData));
    }

    /* Horinzontal calibration bars */
    if ((symbolRow + 1) % (dataRegionRows + 2) == 0) {
        return (((symbolCol & 0x01) ? 0 : DmtxModuleOnRGB) | (!DmtxModuleData));
    }

    /* Vertical calibration bars */
    if ((symbolCol + 1) % (dataRegionCols + 2) == 0) {
        return (((symbolRow & 0x01) ? 0 : DmtxModuleOnRGB) | (!DmtxModuleData));
    }

    /* Data modules */
    return (message->array[mappingRow * mappingCols + mappingCol] | DmtxModuleData);
}

/**
 * @brief 逻辑关系：位与模块位置之间的映射
 *
 * 此函数根据指定的大小索引(sizeIdx)和模块颜色标识(moduleOnColor)，在模块数组(modules)上布置
 * ECC200 Data Matrix 码的编码字(codewords)。
 *
 * 处理标准模式和特殊角落模式的布局。
 *
 * @param[in, out] modules 用于绘制Data Matrix码的二维模块数组
 * @param[in]      codewords 存储编码数据的字节数组
 * @param[in]      sizeIdx Data Matrix符号的大小索引
 * @param[in]      moduleOnColor 指定模块颜色属性的标志，如红色、绿色或蓝色
 *
 * @return 已读取的编码字数量
 */
static int ModulePlacementEcc200(unsigned char *modules, unsigned char *codewords, int sizeIdx, int moduleOnColor)
{
    int row, col, chr;
    int mappingRows, mappingCols;

    DmtxAssert(moduleOnColor & (DmtxModuleOnRed | DmtxModuleOnGreen | DmtxModuleOnBlue));

    mappingRows = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixRows, sizeIdx);
    mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, sizeIdx);

    /* 初始化：寻找第一个字符的第8位的起始位置 */
    chr = 0;
    row = 4;
    col = 0;

    do {
        /* 检查并处理四个特殊角落的码字排布模式 */
        if ((row == mappingRows) && (col == 0)) {
            PatternShapeSpecial1(modules, mappingRows, mappingCols, &(codewords[chr++]), moduleOnColor);
        } else if ((row == mappingRows - 2) && (col == 0) && (mappingCols % 4 != 0)) {
            PatternShapeSpecial2(modules, mappingRows, mappingCols, &(codewords[chr++]), moduleOnColor);
        } else if ((row == mappingRows - 2) && (col == 0) && (mappingCols % 8 == 4)) {
            PatternShapeSpecial3(modules, mappingRows, mappingCols, &(codewords[chr++]), moduleOnColor);
        } else if ((row == mappingRows + 4) && (col == 2) && (mappingCols % 8 == 0)) {
            PatternShapeSpecial4(modules, mappingRows, mappingCols, &(codewords[chr++]), moduleOnColor);
        }

        /* 以对角线方式斜向上扫描并插入字符 */
        do {
            if ((row < mappingRows) && (col >= 0) && !(modules[row * mappingCols + col] & DmtxModuleVisited)) {
                PatternShapeStandard(modules, mappingRows, mappingCols, row, col, &(codewords[chr++]), moduleOnColor);
            }
            row -= 2;
            col += 2;
        } while ((row >= 0) && (col < mappingCols));
        row += 1;
        col += 3;

        /* 同样以对角线方式向下扫描并插入字符 */
        do {
            if ((row >= 0) && (col < mappingCols) && !(modules[row * mappingCols + col] & DmtxModuleVisited)) {
                PatternShapeStandard(modules, mappingRows, mappingCols, row, col, &(codewords[chr++]), moduleOnColor);
            }
            row += 2;
            col -= 2;
        } while ((row < mappingRows) && (col >= 0));
        row += 3;
        col += 1;
        /* 重复此过程，直到扫描完整个modules数组 */
    } while ((row < mappingRows) || (col < mappingCols));

    /* 处理右下角的固定模式 */
    if (!(modules[mappingRows * mappingCols - 1] & DmtxModuleVisited)) {
        modules[mappingRows * mappingCols - 1] |= moduleOnColor;
        modules[(mappingRows * mappingCols) - mappingCols - 2] |= moduleOnColor;
    } /* XXX should this fixed pattern also be used in reading somehow? */

    /* XXX compare that chr == region->dataSize here */
    return chr; /* XXX number of codewords read off */
}

/**
 * @brief 将标准码字放置到指定的模块位置
 *
 * ```
 * |1|2|
 * |3|4|5|
 * |6|7|8|
 * ```
 *
 * @param modules 指向存储模块数据的二维数组的指针
 * @param mappingRows 模块映射的行数
 * @param mappingCols 模块映射的列数
 * @param row 标准码字第8位所在的行坐标
 * @param col 标准码字第8位所在的列坐标
 * @param codeword 码字
 * @param moduleOnColor 表示模块开启颜色的整数值
 */
static void PatternShapeStandard(unsigned char *modules, int mappingRows, int mappingCols, int row, int col,
                                 unsigned char *codeword, int moduleOnColor)
{
    PlaceModule(modules, mappingRows, mappingCols, row - 2, col - 2, codeword, DmtxMaskBit1, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, row - 2, col - 1, codeword, DmtxMaskBit2, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, row - 1, col - 2, codeword, DmtxMaskBit3, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, row - 1, col - 1, codeword, DmtxMaskBit4, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, row - 1, col, codeword, DmtxMaskBit5, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, row, col - 2, codeword, DmtxMaskBit6, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, row, col - 1, codeword, DmtxMaskBit7, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, row, col, codeword, DmtxMaskBit8, moduleOnColor);
}

/**
 * @brief 特殊排布1
 *
 * 左下角:
 * ```
 * |1|2|3|
 * ```
 *
 * 右上角:
 * ```
 * |4|5|
 *   |6|
 *   |7|
 *   |8|
 * ```
 * @param  modules
 * @param  mappingRows
 * @param  mappingCols
 * @param  codeword
 * @param  moduleOnColor
 */
static void PatternShapeSpecial1(unsigned char *modules, int mappingRows, int mappingCols, unsigned char *codeword,
                                 int moduleOnColor)
{
    PlaceModule(modules, mappingRows, mappingCols, mappingRows - 1, 0, codeword, DmtxMaskBit1, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, mappingRows - 1, 1, codeword, DmtxMaskBit2, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, mappingRows - 1, 2, codeword, DmtxMaskBit3, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 0, mappingCols - 2, codeword, DmtxMaskBit4, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 0, mappingCols - 1, codeword, DmtxMaskBit5, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 1, mappingCols - 1, codeword, DmtxMaskBit6, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 2, mappingCols - 1, codeword, DmtxMaskBit7, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 3, mappingCols - 1, codeword, DmtxMaskBit8, moduleOnColor);
}

/**
 * @brief 特殊排布2
 *
 * 左下角:
 * ```
 * |1|
 * |2|
 * |3|
 * ```
 *
 * 右上角:
 * ```
 * |4|5|6|7|
 *       |8|
 * ```
 *
 * @param  modules
 * @param  mappingRows
 * @param  mappingCols
 * @param  codeword
 * @param  moduleOnColor
 */
static void PatternShapeSpecial2(unsigned char *modules, int mappingRows, int mappingCols, unsigned char *codeword,
                                 int moduleOnColor)
{
    PlaceModule(modules, mappingRows, mappingCols, mappingRows - 3, 0, codeword, DmtxMaskBit1, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, mappingRows - 2, 0, codeword, DmtxMaskBit2, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, mappingRows - 1, 0, codeword, DmtxMaskBit3, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 0, mappingCols - 4, codeword, DmtxMaskBit4, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 0, mappingCols - 3, codeword, DmtxMaskBit5, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 0, mappingCols - 2, codeword, DmtxMaskBit6, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 0, mappingCols - 1, codeword, DmtxMaskBit7, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 1, mappingCols - 1, codeword, DmtxMaskBit8, moduleOnColor);
}

/**
 * @brief 特殊排布3
 *
 * 左下角:
 * ```
 * |1|
 * |2|
 * |3|
 * ```
 *
 * 右上角:
 * ```
 * |4|5|
 *   |6|
 *   |7|
 *   |8|
 * ```
 * @param  modules
 * @param  mappingRows
 * @param  mappingCols
 * @param  codeword
 * @param  moduleOnColor
 */
static void PatternShapeSpecial3(unsigned char *modules, int mappingRows, int mappingCols, unsigned char *codeword,
                                 int moduleOnColor)
{
    PlaceModule(modules, mappingRows, mappingCols, mappingRows - 3, 0, codeword, DmtxMaskBit1, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, mappingRows - 2, 0, codeword, DmtxMaskBit2, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, mappingRows - 1, 0, codeword, DmtxMaskBit3, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 0, mappingCols - 2, codeword, DmtxMaskBit4, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 0, mappingCols - 1, codeword, DmtxMaskBit5, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 1, mappingCols - 1, codeword, DmtxMaskBit6, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 2, mappingCols - 1, codeword, DmtxMaskBit7, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 3, mappingCols - 1, codeword, DmtxMaskBit8, moduleOnColor);
}

/**
 * @brief 特殊排布4
 *
 * 左下角:
 * ```
 * |1|
 * ```
 *
 * 右下角:
 * ```
 * |2|
 * ```
 *
 * 右上角:
 * ```
 * |3|4|5|
 * |6|7|8|
 * ```
 *
 * @param  modules
 * @param  mappingRows
 * @param  mappingCols
 * @param  codeword
 * @param  moduleOnColor
 */
static void PatternShapeSpecial4(unsigned char *modules, int mappingRows, int mappingCols, unsigned char *codeword,
                                 int moduleOnColor)
{
    PlaceModule(modules, mappingRows, mappingCols, mappingRows - 1, 0, codeword, DmtxMaskBit1, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, mappingRows - 1, mappingCols - 1, codeword, DmtxMaskBit2,
                moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 0, mappingCols - 3, codeword, DmtxMaskBit3, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 0, mappingCols - 2, codeword, DmtxMaskBit4, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 0, mappingCols - 1, codeword, DmtxMaskBit5, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 1, mappingCols - 3, codeword, DmtxMaskBit6, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 1, mappingCols - 2, codeword, DmtxMaskBit7, moduleOnColor);
    PlaceModule(modules, mappingRows, mappingCols, 1, mappingCols - 1, codeword, DmtxMaskBit8, moduleOnColor);
}

/**
 * @brief 位模块放置
 *
 * 此函数负责在Data Matrix码的模块矩阵中放置或读取单个位模块，取决于当前操作是编码还是解码过程。
 * 它处理边界 wrap-around 逻辑，并根据给定的编码字和模块颜色更新模块状态。
 *
 * @param modules 指向包含模块信息的一维数组，按行优先顺序排列
 * @param mappingRows 数据矩阵映射区域的行数
 * @param mappingCols 数据矩阵映射区域的列数
 * @param row 当前处理模块的行索引
 * @param col 当前处理模块的列索引
 * @param codeword 指向当前处理的编码字的指针，用于解码时读取或编码时设置模块状态
 * @param mask 用于位操作的掩码，帮助区分编码字中的特定位
 * @param moduleOnColor 指定放置模块的颜色属性标记
 */
static void PlaceModule(unsigned char *modules, int mappingRows, int mappingCols, int row, int col,
                        unsigned char *codeword, int mask, int moduleOnColor)
{
    if (row < 0) {
        row += mappingRows;
        col += 4 - ((mappingRows + 4) % 8);
    }
    if (col < 0) {
        col += mappingCols;
        row += 4 - ((mappingCols + 4) % 8);
    }

    /* If module has already been assigned then we are decoding the pattern into codewords */
    if ((modules[row * mappingCols + col] & DmtxModuleAssigned) != 0) {
        if ((modules[row * mappingCols + col] & moduleOnColor) != 0) {
            *codeword |= mask;
        } else {
            *codeword &= (0xff ^ mask);
        }
    } else { /* Otherwise we are encoding the codewords into a pattern */
        if ((*codeword & mask) != 0x00) {
            modules[row * mappingCols + col] |= moduleOnColor;
        }

        modules[row * mappingCols + col] |= DmtxModuleAssigned;
    }

    modules[row * mappingCols + col] |= DmtxModuleVisited;
}
