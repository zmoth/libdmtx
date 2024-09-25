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
 * @file dmtx.h
 * @brief Main libdmtx header
 */

#ifndef __DMTX_H__
#define __DMTX_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Time headers required for DmtxTime struct below */
#include <time.h>
#ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#endif

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#    define M_PI_2 1.57079632679489661923
#endif

#define DmtxUndefined (-1)

#define DmtxPassFail unsigned int
#define DmtxPass 1
#define DmtxFail 0

#define DmtxBoolean unsigned int
#define DmtxTrue 1
#define DmtxFalse 0

#define DmtxFormatMatrix 0
#define DmtxFormatMosaic 1

#define DmtxSymbolSquareCount 24 /**< 正方形二维码种类个数 */
#define DmtxSymbolRectCount 6    /**< 长方形二维码种类个数 */

#define DmtxModuleOff 0x00           /**< bit0 */
#define DmtxModuleOnRed 0x01         /**< 红 */
#define DmtxModuleOnGreen 0x02       /**< 绿 */
#define DmtxModuleOnBlue 0x04        /**< 蓝 */
#define DmtxModuleOnRGB 0x07         /**< OnRed | OnGreen | OnBlue */
#define DmtxModuleOn DmtxModuleOnRGB /**< bit1 */
#define DmtxModuleUnsure 0x08        /**< 不确定 */
#define DmtxModuleAssigned 0x10      /**< 已分配 */
#define DmtxModuleVisited 0x20       /**< 已访问 */
#define DmtxModuleData 0x40

#define dmtxLogTrace(...) dmtxLog(DmtxLogTrace, __FILE__, __LINE__, __VA_ARGS__)
#define dmtxLogDebug(...) dmtxLog(DmtxLogDebug, __FILE__, __LINE__, __VA_ARGS__)
#define dmtxLogInfo(...) dmtxLog(DmtxLogInfo, __FILE__, __LINE__, __VA_ARGS__)
#define dmtxLogWarn(...) dmtxLog(DmtxLogWarn, __FILE__, __LINE__, __VA_ARGS__)
#define dmtxLogError(...) dmtxLog(DmtxLogError, __FILE__, __LINE__, __VA_ARGS__)
#define dmtxLogFatal(...) dmtxLog(DmtxLogFatal, __FILE__, __LINE__, __VA_ARGS__)

#define IN    /* 表示输入参数，指针指向的值不会修改 */
#define OUT   /* 表示输出参数，指针指向的值会修改，且不会读 */
#define INOUT /* 表示输入输出参数，指针指向的值会修改，且会读取 */

    typedef enum DmtxStatus_enum
    {
        DmtxStatusEncoding, /* Encoding is currently underway */
        DmtxStatusComplete, /* Encoding is done and everything went well */
        DmtxStatusInvalid,  /* Something bad happened that sometimes happens */
        DmtxStatusFatal     /* Something happened that should never happen */
    } DmtxStatus;

    typedef enum DmtxScheme_enum
    {
        DmtxSchemeAutoFast = -2,
        DmtxSchemeAutoBest = -1,
        DmtxSchemeAscii = 0,
        DmtxSchemeC40,
        DmtxSchemeText,
        DmtxSchemeX12,
        DmtxSchemeEdifact,
        DmtxSchemeBase256
    } DmtxScheme;

    typedef enum DmtxSymbolSize_enum
    {
        DmtxSymbolRectAuto = -3,
        DmtxSymbolSquareAuto = -2,
        DmtxSymbolShapeAuto = -1,

        /* 正方形 */
        DmtxSymbol10x10 = 0,
        DmtxSymbol12x12,
        DmtxSymbol14x14,
        DmtxSymbol16x16,
        DmtxSymbol18x18,
        DmtxSymbol20x20,
        DmtxSymbol22x22,
        DmtxSymbol24x24,
        DmtxSymbol26x26,
        DmtxSymbol32x32,
        DmtxSymbol36x36,
        DmtxSymbol40x40,
        DmtxSymbol44x44,
        DmtxSymbol48x48,
        DmtxSymbol52x52,
        DmtxSymbol64x64,
        DmtxSymbol72x72,
        DmtxSymbol80x80,
        DmtxSymbol88x88,
        DmtxSymbol96x96,
        DmtxSymbol104x104,
        DmtxSymbol120x120,
        DmtxSymbol132x132,
        DmtxSymbol144x144,

        /* 长方形 */
        DmtxSymbol8x18,
        DmtxSymbol8x32,
        DmtxSymbol12x26,
        DmtxSymbol12x36,
        DmtxSymbol16x36,
        DmtxSymbol16x48
    } DmtxSymbolSize;

    typedef enum DmtxDirection_enum
    {
        DmtxDirNone = 0x00,
        DmtxDirUp = 0x01 << 0,
        DmtxDirLeft = 0x01 << 1,
        DmtxDirDown = 0x01 << 2,
        DmtxDirRight = 0x01 << 3,
        DmtxDirHorizontal = DmtxDirLeft | DmtxDirRight,
        DmtxDirVertical = DmtxDirUp | DmtxDirDown,
        DmtxDirRightUp = DmtxDirRight | DmtxDirUp,
        DmtxDirLeftDown = DmtxDirLeft | DmtxDirDown
    } DmtxDirection;

    typedef enum DmtxSymAttribute_enum
    {
        DmtxSymAttribSymbolRows,          /**< 二维码码元总行数（包括L形框和点线）*/
        DmtxSymAttribSymbolCols,          /**< 二维码码元总列数（包括L形框和点线）*/
        DmtxSymAttribDataRegionRows,      /**< 单区块二维码数据区码元行数（不包括L形框和点线）*/
        DmtxSymAttribDataRegionCols,      /**< 单区块二维码数据区码元列数（不包括L形框和点线）*/
        DmtxSymAttribHorizDataRegions,    /**< 水平方向区块个数 */
        DmtxSymAttribVertDataRegions,     /**< 垂直方向区块个数 */
        DmtxSymAttribMappingMatrixRows,   /**< 二维码数据区码元总行数（不包括L形框和点线）*/
        DmtxSymAttribMappingMatrixCols,   /**< 二维码数据区码元总列数（不包括L形框和点线）*/
        DmtxSymAttribInterleavedBlocks,   /**< */
        DmtxSymAttribBlockErrorWords,     /**< */
        DmtxSymAttribBlockMaxCorrectable, /**< */
        DmtxSymAttribSymbolDataWords,     /**< */
        DmtxSymAttribSymbolErrorWords,    /**< */
        DmtxSymAttribSymbolMaxCorrectable /**< */
    } DmtxSymAttribute;

    typedef enum DmtxCornerLoc_enum
    {
        DmtxCorner00 = 0x01 << 0,
        DmtxCorner10 = 0x01 << 1,
        DmtxCorner11 = 0x01 << 2,
        DmtxCorner01 = 0x01 << 3
    } DmtxCornerLoc;

    typedef enum DmtxProperty_enum
    {
        /* Encoding properties */
        DmtxPropScheme = 100, /**<  */
        DmtxPropSizeRequest,  /**<  */
        DmtxPropMarginSize,   /**<  */
        DmtxPropModuleSize,   /**<  */
        DmtxPropFnc1,         /**<  */

        /* Decoding properties */
        DmtxPropEdgeMin = 200, /**<  */
        DmtxPropEdgeMax,       /**<  */
        DmtxPropScanGap,       /**<  */
        DmtxPropSquareDevn,    /**<  */
        DmtxPropSymbolSize,    /**<  */
        DmtxPropEdgeThresh,    /**<  */

        /* 图像属性 @ref DmtxImage */
        DmtxPropWidth = 300,   /**< 图像宽度 */
        DmtxPropHeight,        /**< 图像高度 */
        DmtxPropPixelPacking,  /**< 图像格式类型，像素打包方式 @ref DmtxPackOrder */
        DmtxPropBitsPerPixel,  /**< 每像素所需要的bit数 */
        DmtxPropBytesPerPixel, /**< 每像素所需要的byte数 */
        DmtxPropRowPadBytes,   /**< 每行像素在内存中的填充或对齐字节数 */
        DmtxPropRowSizeBytes,  /**< 每一行（包括填充）在内存中的总字节数 */
        DmtxPropImageFlip,     /**< 图像是否需要翻转，通常用于处理上下颠倒的图像 @ref DmtxFlip */
        DmtxPropChannelCount,  /**< 图像通道数 */

        /* Image modifiers */
        DmtxPropXmin = 400, /**< ROI X坐标最小值(如果未设置则为0) */
        DmtxPropXmax,       /**< ROI X坐标最大值(如果未设置则为图像宽度-1) */
        DmtxPropYmin,       /**< ROI Y坐标最小值(如果未设置则为0) */
        DmtxPropYmax,       /**< ROI Y坐标最大值(如果未设置则为图像高度-1) */
        DmtxPropScale       /**< 图像缩放比例 */
    } DmtxProperty;

    typedef enum DmtxPackOrder_enum
    {
        /* Custom format */
        DmtxPackCustom = 100,
        /* 1 bpp */
        DmtxPack1bppK = 200,
        /* 8 bpp grayscale */
        DmtxPack8bppK = 300,
        /* 16 bpp formats */
        DmtxPack16bppRGB = 400,
        DmtxPack16bppRGBX,
        DmtxPack16bppXRGB,
        DmtxPack16bppBGR,
        DmtxPack16bppBGRX,
        DmtxPack16bppXBGR,
        DmtxPack16bppYCbCr,
        /* 24 bpp formats */
        DmtxPack24bppRGB = 500,
        DmtxPack24bppBGR,
        DmtxPack24bppYCbCr,
        /* 32 bpp formats */
        DmtxPack32bppRGBX = 600,
        DmtxPack32bppXRGB,
        DmtxPack32bppBGRX,
        DmtxPack32bppXBGR,
        DmtxPack32bppCMYK
    } DmtxPackOrder;

    typedef enum DmtxFlip_enum
    {
        DmtxFlipNone = 0x00,
        DmtxFlipX = 0x01 << 0,
        DmtxFlipY = 0x01 << 1
    } DmtxFlip;

    typedef enum DmtxLogLevel_enum
    {
        DmtxLogTrace,
        DmtxLogDebug,
        DmtxLogInfo,
        DmtxLogWarn,
        DmtxLogError,
        DmtxLogFatal
    } DmtxLogLevel;

    /**
     * @brief DmtxMatrix3 类型定义，表示一个3x3的双精度浮点数矩阵
     */
    typedef double DmtxMatrix3[3][3];

    /**
     * @brief 像素坐标
     */
    typedef struct DmtxPixelLoc_struct
    {
        int x;
        int y;
    } DmtxPixelLoc;

    /**
     * @brief 二维向量
     */
    typedef struct DmtxVector2_struct
    {
        double x;
        double y;
    } DmtxVector2;

    /**
     * @brief 向量表示的直线(线段)
     */
    typedef struct DmtxRay2_struct
    {
        double tMin;
        double tMax;
        DmtxVector2 p;
        DmtxVector2 v;
    } DmtxRay2;

    typedef unsigned char DmtxByte;

    /**
     * @struct DmtxByteList
     * @brief DmtxByteList
     * Use signed int for length fields instead of size_t to play nicely with RS
     * arithmetic
     */
    typedef struct DmtxByteList_struct
    {
        int length;
        int capacity;
        DmtxByte *b;
    } DmtxByteList;

    typedef struct DmtxEncodeStream_struct
    {
        int currentScheme;         /* Current encodation scheme */
        int inputNext;             /* Index of next unprocessed input word in queue */
        int outputChainValueCount; /* Count of output values pushed within current scheme chain */
        int outputChainWordCount;  /* Count of output words pushed within current scheme chain */
        char *reason;              /* Reason for status */
        int sizeIdx;               /* Symbol size of completed stream */
        int fnc1;                  /* Character to represent FNC1, or DmtxUndefined */
        DmtxStatus status;
        DmtxByteList *input;
        DmtxByteList *output;
    } DmtxEncodeStream;

    typedef struct DmtxImage_struct
    {
        int width;             /**< 图像的宽度，以像素为单位 */
        int height;            /**< 图像的高度，以像素为单位 */
        int pixelPacking;      /**< 图像格式类型，像素打包方式 @ref DmtxPackOrder */
        int bitsPerPixel;      /**< 每个像素的位数 */
        int bytesPerPixel;     /**< 每个像素的字节数 */
        int rowPadBytes;       /**< 每行像素在内存中的填充或对齐字节数 */
        int rowSizeBytes;      /**< 每一行（包括填充）在内存中的总字节数 */
        int imageFlip;         /**< 图像是否需要翻转，通常用于处理上下颠倒的图像 @ref DmtxFlip */
        int channelCount;      /**< 图像的通道数量，如RGB图像为3，CMYK图像为4 */
        int channelStart[4];   /**< 每个通道在像素数据中的起始位置（位偏移） */
        int bitsPerChannel[4]; /**< 每个通道的位数，描述每个颜色分量的精度 */
        unsigned char *pxl;    /**< 实际的像素数据缓冲区 */
    } DmtxImage;

    /**
     * @brief 图像像素点及其梯度流动方向
     */
    typedef struct DmtxPointFlow_struct
    {
        int plane;        /**< 多通道平面索引 */
        int arrive;       /**< 梯度方向起点 */
        int depart;       /**< 梯度方向终点 */
        int mag;          /**< 梯度幅值 */
        DmtxPixelLoc loc; /**< 像素的坐标 */
    } DmtxPointFlow;

    typedef struct DmtxBestLine_struct
    {
        int angle;           /**< 线条的角度，单位：度 */
        int hOffset;         /**< */
        int mag;             /**< 幅值 */
        int stepBeg;         /**< */
        int stepPos;         /**< 正方向步进位置 */
        int stepNeg;         /**< 负方向步进位置 */
        int distSq;          /**< */
        double devn;         /**< */
        DmtxPixelLoc locBeg; /**< 起始位置点 */
        DmtxPixelLoc locPos; /**< 正方向点（线段端点） */
        DmtxPixelLoc locNeg; /**< 负方向点（线段端点） */
    } DmtxBestLine;

    /**
     * @brief 二维码区域(包围框)
     */
    typedef struct DmtxRegion_struct
    {
        /* Trail blazing values */
        int jumpToPos;           /**< */
        int jumpToNeg;           /**< */
        int stepsTotal;          /**< */
        DmtxPixelLoc finalPos;   /**< */
        DmtxPixelLoc finalNeg;   /**< */
        DmtxPixelLoc boundMin;   /**< */
        DmtxPixelLoc boundMax;   /**< */
        DmtxPointFlow flowBegin; /**< 搜索起点，十字搜索抛出的点 */

        /* Orientation values */
        int polarity;      /**< */
        int stepR;         /**< */
        int stepT;         /**< */
        DmtxPixelLoc locR; /**< remove if stepR works above */
        DmtxPixelLoc locT; /**< remove if stepT works above */

        /* Region fitting values */
        int leftKnown;           /**< known == 1; unknown == 0 */
        int leftAngle;           /**< hough angle of left edge */
        DmtxPixelLoc leftLoc;    /**< known (arbitrary) location on left edge */
        DmtxBestLine leftLine;   /**< */
        int bottomKnown;         /**< known == 1; unknown == 0 */
        int bottomAngle;         /**< hough angle of bottom edge */
        DmtxPixelLoc bottomLoc;  /**< known (arbitrary) location on bottom edge */
        DmtxBestLine bottomLine; /**< */
        int topKnown;            /**< known == 1; unknown == 0 */
        int topAngle;            /**< hough angle of top edge */
        DmtxPixelLoc topLoc;     /**< known (arbitrary) location on top edge */
        int rightKnown;          /**< known == 1; unknown == 0 */
        int rightAngle;          /**< hough angle of right edge */
        DmtxPixelLoc rightLoc;   /**< known (arbitrary) location on right edge */

        /* Region calibration values */
        int onColor;     /**< 代表bit1的颜色值 */
        int offColor;    /**< 代表bit0的颜色值*/
        int sizeIdx;     /**< 二维码类型索引，总共有 DmtxSymbolSquareCount + DmtxSymbolRectCount 种 */
        int symbolRows;  /**< 二维码码元行数（包括L形框和点线） */
        int symbolCols;  /**< 二维码码元列数（包括L形框和点线）*/
        int mappingRows; /**< 二维码数据区码元行数 */
        int mappingCols; /**< 二维码数据区码元列数 */

        /* 变换矩阵 */
        DmtxMatrix3 raw2fit; /**< 3x3 变换矩阵，从图像坐标系到二维码坐标系 */
        DmtxMatrix3 fit2raw; /**< 3x3 变换矩阵，从二维码坐标系到图像坐标系 */
    } DmtxRegion;

    /**
     * @brief DataMatrix编码内容
     */
    typedef struct DmtxMessage_struct
    {
        size_t arraySize;      /**< 二维码数据区码元行数x列数(mappingRows * mappingCols) */
        size_t codeSize;       /**< 编码数据的总大小，包括数据字和纠错字 */
        size_t outputSize;     /**< Size of buffer used to hold decoded data */
        int outputIdx;         /**< Internal index used to store output progress */
        int padCount;          /**< */
        int fnc1;              /**< 表示FNC1或DmtxUndefined的字符 */
        unsigned char *array;  /**< 指向DataMatrix数据区二进制矩阵的指针 */
        unsigned char *code;   /**< 指向码字（数据字和纠错字）的指针 */
        unsigned char *output; /**< 指向二维码码值的指针 */
    } DmtxMessage;

    /**
     * @struct DmtxScanGrid
     * @brief DmtxScanGrid
     */
    typedef struct DmtxScanGrid_struct
    {
        /* set once */
        int minExtent; /* Smallest cross size used in scan */
        int maxExtent; /* Size of bounding grid region (2^N - 1) */
        int xOffset;   /* Offset to obtain image X coordinate */
        int yOffset;   /* Offset to obtain image Y coordinate */
        int xMin;      /**< ROI X坐标最小值(如果未设置则为0) */
        int xMax;      /**< ROI X坐标最大值(如果未设置则为图像宽度-1) */
        int yMin;      /**< ROI Y坐标最小值(如果未设置则为0) */
        int yMax;      /**< ROI Y坐标最大值(如果未设置则为图像高度-1) */

        /* reset for each level */
        int total;      /* Total number of crosses at this size */
        int extent;     /* Length/width of cross in pixels */
        int jumpSize;   /* Distance in pixels between cross centers */
        int pixelTotal; /* Total pixel count within an individual cross path */
        int startPos;   /* X and Y coordinate of first cross center in pattern */

        /* reset for each cross */
        int pixelCount; /* Progress (pixel count) within current cross pattern */
        int xCenter;    /* X center of current cross pattern */
        int yCenter;    /* Y center of current cross pattern */
    } DmtxScanGrid;

    /**
     * @struct DmtxTime
     * @brief DmtxTime
     */
    typedef struct DmtxTime_struct
    {
        time_t sec;
        unsigned long usec;
    } DmtxTime;

    /**
     * @struct DmtxDecode
     * @brief DmtxDecode
     */
    typedef struct DmtxDecode_struct
    {
        /* Options */
        int edgeMin;
        int edgeMax;
        int scanGap;
        int fnc1;
        double squareDevn;
        int sizeIdxExpected;
        int edgeThresh;

        /* Image modifiers */
        int xMin;
        int xMax;
        int yMin;
        int yMax;
        int scale;

        /* Internals */
        /* int             cacheComplete; */
        unsigned char *cache;
        DmtxImage *image;
        DmtxScanGrid grid;
    } DmtxDecode;

    /**
     * @struct DmtxEncode
     * @brief DmtxEncode
     */
    typedef struct DmtxEncode_struct
    {
        int method;
        int scheme;
        int sizeIdxRequest;
        int marginSize;
        int moduleSize;
        int pixelPacking;
        int imageFlip;
        int rowPadBytes;
        int fnc1;
        DmtxMessage *message;
        DmtxImage *image;
        DmtxRegion region;
        DmtxMatrix3 xfrm;  /* XXX still necessary? */
        DmtxMatrix3 rxfrm; /* XXX still necessary? */
    } DmtxEncode;

    /**
     * @struct DmtxChannel
     * @brief DmtxChannel
     */
    typedef struct DmtxChannel_struct
    {
        int encScheme;            /* current encodation scheme */
        int invalid;              /* channel status (invalid if non-zero) */
        unsigned char *inputPtr;  /* pointer to current input character */
        unsigned char *inputStop; /* pointer to position after final input character */
        int encodedLength;        /* encoded length (units of 2/3 bits) */
        int currentLength;        /* current length (units of 2/3 bits) */
        int firstCodeWord;        /* */
        unsigned char encodedWords[1558];
    } DmtxChannel;

    /* Wrap in a struct for fast copies */
    /**
     * @struct DmtxChannelGroup
     * @brief DmtxChannelGroup
     */
    typedef struct DmtxChannelGroup_struct
    {
        DmtxChannel channel[6];
    } DmtxChannelGroup;

    /**
     * @struct DmtxTriplet
     * @brief DmtxTriplet
     */
    typedef struct DmtxTriplet_struct
    {
        unsigned char value[3];
    } DmtxTriplet;

    /**
     * @struct DmtxQuadruplet
     * @brief DmtxQuadruplet
     */
    typedef struct DmtxQuadruplet_struct
    {
        unsigned char value[4];
    } DmtxQuadruplet;

    /* dmtxtime.c */
    extern DmtxTime dmtxTimeNow(void);
    extern DmtxTime dmtxTimeAdd(DmtxTime t, long msec);
    extern int dmtxTimeExceeded(DmtxTime timeout);

    /* dmtxencode.c */
    extern DmtxEncode *dmtxEncodeCreate(void);
    extern DmtxPassFail dmtxEncodeDestroy(DmtxEncode **enc);
    extern DmtxPassFail dmtxEncodeSetProp(DmtxEncode *enc, int prop, int value);
    extern int dmtxEncodeGetProp(DmtxEncode *enc, int prop);
    extern DmtxPassFail dmtxEncodeDataMatrix(DmtxEncode *enc, int inputSize, unsigned char *inputString);
    extern DmtxPassFail dmtxEncodeDataMosaic(DmtxEncode *enc, int inputSize, unsigned char *inputString);

    /* dmtxdecode.c */
    extern DmtxDecode *dmtxDecodeCreate(DmtxImage *img, int scale);
    extern DmtxPassFail dmtxDecodeDestroy(DmtxDecode **dec);
    extern DmtxPassFail dmtxDecodeSetProp(DmtxDecode *dec, int prop, int value);
    extern int dmtxDecodeGetProp(DmtxDecode *dec, int prop);
    extern /*@exposed@*/ unsigned char *dmtxDecodeGetCache(DmtxDecode *dec, int x, int y);
    extern DmtxPassFail dmtxDecodeGetPixelValue(DmtxDecode *dec, int x, int y, int channel, OUT int *value);
    extern DmtxMessage *dmtxDecodeMatrixRegion(DmtxDecode *dec, DmtxRegion *reg, int fix);
    extern DmtxMessage *dmtxDecodePopulatedArray(int sizeIdx, INOUT DmtxMessage *msg, int fix);
    extern DmtxMessage *dmtxDecodeMosaicRegion(DmtxDecode *dec, DmtxRegion *reg, int fix);
    extern unsigned char *dmtxDecodeCreateDiagnostic(DmtxDecode *dec, OUT int *totalBytes, OUT int *headerBytes,
                                                     int style);

    /* dmtxregion.c */
    extern DmtxRegion *dmtxRegionCreate(DmtxRegion *reg);
    extern DmtxPassFail dmtxRegionDestroy(DmtxRegion **reg);
    extern DmtxRegion *dmtxRegionFindNext(DmtxDecode *dec, DmtxTime *timeout);
    extern DmtxRegion *dmtxRegionScanPixel(DmtxDecode *dec, int x, int y);
    extern DmtxPassFail dmtxRegionUpdateCorners(DmtxDecode *dec, DmtxRegion *reg, DmtxVector2 p00, DmtxVector2 p10,
                                                DmtxVector2 p11, DmtxVector2 p01);
    extern DmtxPassFail dmtxRegionUpdateXfrms(DmtxDecode *dec, DmtxRegion *reg);

    /* dmtxmessage.c */
    extern DmtxMessage *dmtxMessageCreate(int sizeIdx, int symbolFormat);
    extern DmtxPassFail dmtxMessageDestroy(DmtxMessage **msg);

    /* dmtximage.c */
    extern DmtxImage *dmtxImageCreate(unsigned char *pxl, int width, int height, int pack);
    extern DmtxPassFail dmtxImageDestroy(DmtxImage **img);
    extern DmtxPassFail dmtxImageSetChannel(DmtxImage *img, int channelStart, int bitsPerChannel);
    extern DmtxPassFail dmtxImageSetProp(DmtxImage *img, int prop, int value);
    extern int dmtxImageGetProp(DmtxImage *img, int prop);
    extern int dmtxImageGetByteOffset(DmtxImage *img, int x, int y);
    extern DmtxPassFail dmtxImageGetPixelValue(DmtxImage *img, int x, int y, int channel, OUT int *value);
    extern DmtxPassFail dmtxImageSetPixelValue(DmtxImage *img, int x, int y, int channel, int value);
    extern DmtxBoolean dmtxImageContainsInt(DmtxImage *img, int margin, int x, int y);
    extern DmtxBoolean dmtxImageContainsFloat(DmtxImage *img, double x, double y);

    /* dmtxvector2.c */
    extern DmtxVector2 *dmtxVector2AddTo(DmtxVector2 *v1, const DmtxVector2 *v2);
    extern DmtxVector2 *dmtxVector2Add(OUT DmtxVector2 *vOut, const DmtxVector2 *v1, const DmtxVector2 *v2);
    extern DmtxVector2 *dmtxVector2SubFrom(DmtxVector2 *v1, const DmtxVector2 *v2);
    extern DmtxVector2 *dmtxVector2Sub(OUT DmtxVector2 *vOut, const DmtxVector2 *v1, const DmtxVector2 *v2);
    extern DmtxVector2 *dmtxVector2ScaleBy(DmtxVector2 *v, double s);
    extern DmtxVector2 *dmtxVector2Scale(OUT DmtxVector2 *vOut, const DmtxVector2 *v, double s);
    extern double dmtxVector2Cross(const DmtxVector2 *v1, const DmtxVector2 *v2);
    extern double dmtxVector2Norm(DmtxVector2 *v);
    extern double dmtxVector2Dot(const DmtxVector2 *v1, const DmtxVector2 *v2);
    extern double dmtxVector2Mag(const DmtxVector2 *v);
    extern double dmtxDistanceFromRay2(const DmtxRay2 *r, const DmtxVector2 *q);
    extern double dmtxDistanceAlongRay2(const DmtxRay2 *r, const DmtxVector2 *q);
    extern DmtxPassFail dmtxRay2Intersect(OUT DmtxVector2 *point, const DmtxRay2 *p0, const DmtxRay2 *p1);
    extern DmtxPassFail dmtxPointAlongRay2(OUT DmtxVector2 *point, const DmtxRay2 *r, double t);

    /* dmtxmatrix3.c */
    extern void dmtxMatrix3Copy(OUT DmtxMatrix3 m0, DmtxMatrix3 m1);
    extern void dmtxMatrix3Identity(OUT DmtxMatrix3 m);
    extern void dmtxMatrix3Translate(OUT DmtxMatrix3 m, double tx, double ty);
    extern void dmtxMatrix3Rotate(OUT DmtxMatrix3 m, double angle);
    extern void dmtxMatrix3Scale(OUT DmtxMatrix3 m, double sx, double sy);
    extern void dmtxMatrix3Shear(OUT DmtxMatrix3 m, double shx, double shy);
    extern void dmtxMatrix3LineSkewTop(OUT DmtxMatrix3 m, double b0, double b1, double sz);
    extern void dmtxMatrix3LineSkewTopInv(OUT DmtxMatrix3 m, double b0, double b1, double sz);
    extern void dmtxMatrix3LineSkewSide(OUT DmtxMatrix3 m, double b0, double b1, double sz);
    extern void dmtxMatrix3LineSkewSideInv(OUT DmtxMatrix3 m, double b0, double b1, double sz);
    extern void dmtxMatrix3Multiply(OUT DmtxMatrix3 mOut, DmtxMatrix3 m0, DmtxMatrix3 m1);
    extern void dmtxMatrix3MultiplyBy(INOUT DmtxMatrix3 m0, DmtxMatrix3 m1);
    extern DmtxPassFail dmtxMatrix3VMultiply(OUT DmtxVector2 *vOut, DmtxVector2 *vIn, DmtxMatrix3 m);
    extern DmtxPassFail dmtxMatrix3VMultiplyBy(INOUT DmtxVector2 *v, DmtxMatrix3 m);
    extern void dmtxMatrix3Print(DmtxMatrix3 m);

    /* dmtxsymbol.c */
    extern int dmtxSymbolModuleStatus(DmtxMessage *message, int sizeIdx, int symbolRow, int symbolCol);
    extern int dmtxGetSymbolAttribute(int attribute, int sizeIdx);
    extern int dmtxGetBlockDataSize(int sizeIdx, int blockIdx);
    extern int getSizeIdxFromSymbolDimension(int rows, int cols);

    /* dmtxbytelist.c */
    extern DmtxByteList dmtxByteListBuild(DmtxByte *storage, int capacity);
    extern void dmtxByteListInit(DmtxByteList *list, int length, DmtxByte value, DmtxPassFail *passFail);
    extern void dmtxByteListClear(DmtxByteList *list);
    extern DmtxBoolean dmtxByteListHasCapacity(DmtxByteList *list);
    extern void dmtxByteListCopy(DmtxByteList *dst, const DmtxByteList *src, DmtxPassFail *passFail);
    extern void dmtxByteListPush(DmtxByteList *list, DmtxByte value, DmtxPassFail *passFail);
    extern DmtxByte dmtxByteListPop(DmtxByteList *list, DmtxPassFail *passFail);
    extern void dmtxByteListPrint(DmtxByteList *list, char *prefix);

    /* dmtxlog.c */
    extern void dmtxLog(int level, const char *file, int line, const char *fmt, ...);
    extern void dmtxLogSetLevel(int level);
    extern void dmtxLogSetQuiet(DmtxBoolean enable);

    /* dmtxcallback.c */
    typedef void (*DmtxCallbackBuildMatrixRegion)(DmtxRegion *region);
    typedef void (*DmtxCallbackBuildMatrix)(DmtxMatrix3 matrix);
    typedef void (*DmtxCallbackPlotPoint)(DmtxPixelLoc loc, float colorHue, int paneNbr, int dispType);
    typedef void (*DmtxCallbackXfrmPlotPoint)(DmtxVector2 point, DmtxMatrix3 xfrm, int paneNbr, int dispType);
    typedef void (*DmtxCallbackPlotModule)(DmtxDecode *info, DmtxRegion *region, int row, int col, float colorHue);
    typedef void (*DmtxCallbackFinal)(DmtxDecode *decode, DmtxRegion *region);

    extern void dmtxCallbackBuildMatrixRegion(DmtxCallbackBuildMatrixRegion cb);
    extern void dmtxCallbackBuildMatrix(DmtxCallbackBuildMatrix cb);
    extern void dmtxCallbackPlotPoint(DmtxCallbackPlotPoint cb);
    extern void dmtxCallbackXfrmPlotPoint(DmtxCallbackXfrmPlotPoint cb);
    extern void dmtxCallbackPlotModule(DmtxCallbackPlotModule cb);
    extern void dmtxCallbackFinal(DmtxCallbackFinal cb);

    extern char *dmtxVersion(void);

#ifdef __cplusplus
}
#endif

#endif
