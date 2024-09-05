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
 * \file dmtxstatic.h
 * \brief Static header
 */

#ifndef __DMTXSTATIC_H__
#define __DMTXSTATIC_H__

#include <assert.h>
#include <stdio.h>

#include "dmtx.h"

#define DmtxAlmostZero 0.000001
#define DmtxAlmostInfinity -1

#define DmtxValueC40Latch 230
#define DmtxValueTextLatch 239
#define DmtxValueX12Latch 238
#define DmtxValueEdifactLatch 240
#define DmtxValueBase256Latch 231

#define DmtxValueCTXUnlatch 254
#define DmtxValueEdifactUnlatch 31

#define DmtxValueAsciiPad 129
#define DmtxValueAsciiUpperShift 235
#define DmtxValueCTXShift1 0
#define DmtxValueCTXShift2 1
#define DmtxValueCTXShift3 2
#define DmtxValueFNC1 232
#define DmtxValueStructuredAppend 233
#define DmtxValueReaderProgramming 234
#define DmtxValue05Macro 236
#define DmtxValue06Macro 237
#define DmtxValueECI 241

#define DmtxC40TextBasicSet 0
#define DmtxC40TextShift1 1
#define DmtxC40TextShift2 2
#define DmtxC40TextShift3 3

#define DmtxUnlatchExplicit 0
#define DmtxUnlatchImplicit 1

#define DmtxChannelValid 0x00
#define DmtxChannelUnsupportedChar 0x01 << 0
#define DmtxChannelCannotUnlatch 0x01 << 1

#undef min
#define min(X, Y) (((X) < (Y)) ? (X) : (Y))

#undef max
#define max(X, Y) (((X) > (Y)) ? (X) : (Y))

#undef ISDIGIT
#define ISDIGIT(n) (n > 47 && n < 58)

/* Verify stream is using expected scheme */
#define CHKSCHEME(s)                                            \
    {                                                           \
        if (stream->currentScheme != (s)) {                     \
            streamMarkFatal(stream, DmtxErrorUnexpectedScheme); \
            return;                                             \
        }                                                       \
    }

/* CHKERR should follow any call that might alter stream status */
#define CHKERR                                      \
    {                                               \
        if (stream->status != DmtxStatusEncoding) { \
            return;                                 \
        }                                           \
    }

/* CHKSIZE should follows typical calls to findSymbolSize()  */
#define CHKSIZE                                          \
    {                                                    \
        if (sizeIdx == DmtxUndefined) {                  \
            streamMarkInvalid(stream, DmtxErrorUnknown); \
            return;                                      \
        }                                                \
    }

#define DmtxAssert(expr)                 \
    do {                                 \
        assert(expr);                    \
        if (!!(expr)) {                  \
        } else {                         \
            dmtxLogError("%s\n", #expr); \
        }                                \
    } while (0)

typedef enum
{
    DmtxEncodeNormal,  /* Use normal scheme behavior (e.g., ASCII auto) */
    DmtxEncodeCompact, /* Use only compact format within scheme */
    DmtxEncodeFull     /* Use only fully expanded format within scheme */
} DmtxEncodeOption;

typedef enum
{
    DmtxRangeGood,
    DmtxRangeBad,
    DmtxRangeEnd
} DmtxRange;

typedef enum
{
    DmtxEdgeTop = 0x01 << 0,
    DmtxEdgeBottom = 0x01 << 1,
    DmtxEdgeLeft = 0x01 << 2,
    DmtxEdgeRight = 0x01 << 3
} DmtxEdge;

typedef enum
{
    DmtxMaskBit8 = 0x01 << 0,
    DmtxMaskBit7 = 0x01 << 1,
    DmtxMaskBit6 = 0x01 << 2,
    DmtxMaskBit5 = 0x01 << 3,
    DmtxMaskBit4 = 0x01 << 4,
    DmtxMaskBit3 = 0x01 << 5,
    DmtxMaskBit2 = 0x01 << 6,
    DmtxMaskBit1 = 0x01 << 7
} DmtxMaskBit;

/**
 * @struct DmtxFollow
 * @brief DmtxFollow
 */
typedef struct DmtxFollow_struct
{
    unsigned char *ptr;
    unsigned char neighbor;
    int step;
    DmtxPixelLoc loc;
} DmtxFollow;

/**
 * @struct DmtxBresLine
 * @brief DmtxBresLine
 */
typedef struct DmtxBresLine_struct
{
    int xStep;
    int yStep;
    int xDelta;
    int yDelta;
    int steep;
    int xOut;
    int yOut;
    int travel;
    int outward;
    int error;
    DmtxPixelLoc loc;
    DmtxPixelLoc loc0;
    DmtxPixelLoc loc1;
} DmtxBresLine;

typedef struct C40TextState_struct
{
    int shift;
    DmtxBoolean upperShift;
} C40TextState;

typedef struct
{
    va_list ap;
    const char *fmt;
    const char *file;
    struct tm *time;
    void *udata;
    int line;
    int level;
} DmtxLogEvent;

typedef void (*DmtxLogFn)(DmtxLogEvent *ev);
typedef void (*DmtxLogLockFn)(DmtxBoolean lock, void *udata);

/* dmtxregion.c */
static double rightAngleTrueness(DmtxVector2 c0, DmtxVector2 c1, DmtxVector2 c2, double angle);
static DmtxPointFlow matrixRegionSeekEdge(DmtxDecode *dec, DmtxPixelLoc loc0);
static DmtxPassFail matrixRegionOrientation(DmtxDecode *dec, DmtxRegion *reg, DmtxPointFlow flowBegin);
static long distanceSquared(DmtxPixelLoc a, DmtxPixelLoc b);
static int readModuleColor(DmtxDecode *dec, DmtxRegion *reg, int symbolRow, int symbolCol, int sizeIdx, int colorPlane);

static DmtxPassFail matrixRegionFindSize(DmtxDecode *dec, DmtxRegion *reg);
static int countJumpTally(DmtxDecode *dec, DmtxRegion *reg, int xStart, int yStart, DmtxDirection dir);
static DmtxPointFlow getPointFlow(DmtxDecode *dec, int colorPlane, DmtxPixelLoc loc, int arrive);
static DmtxPointFlow findStrongestNeighbor(DmtxDecode *dec, DmtxPointFlow center, int sign);
static DmtxFollow followSeek(DmtxDecode *dec, DmtxRegion *reg, int seek);
static DmtxFollow followSeekLoc(DmtxDecode *dec, DmtxPixelLoc loc);
static DmtxFollow followStep(DmtxDecode *dec, DmtxRegion *reg, DmtxFollow followBeg, int sign);
static DmtxFollow followStep2(DmtxDecode *dec, DmtxFollow followBeg, int sign);
static DmtxPassFail trailBlazeContinuous(DmtxDecode *dec, DmtxRegion *reg, DmtxPointFlow flowBegin, int maxDiagonal);
static int trailBlazeGapped(DmtxDecode *dec, DmtxRegion *reg, DmtxBresLine line, int streamDir);
static int trailClear(DmtxDecode *dec, DmtxRegion *reg, int clearMask);
static DmtxBestLine findBestSolidLine(DmtxDecode *dec, DmtxRegion *reg, int step0, int step1, int streamDir,
                                      int houghAvoid);
static DmtxBestLine findBestSolidLine2(DmtxDecode *dec, DmtxPixelLoc loc0, int tripSteps, int sign, int houghAvoid);
static DmtxPassFail findTravelLimits(DmtxDecode *dec, DmtxRegion *reg, DmtxBestLine *line);
static DmtxPassFail matrixRegionAlignCalibEdge(DmtxDecode *dec, DmtxRegion *reg, int edgeLoc);
static DmtxBresLine bresLineInit(DmtxPixelLoc loc0, DmtxPixelLoc loc1, DmtxPixelLoc locInside);
static DmtxPassFail bresLineGetStep(DmtxBresLine line, DmtxPixelLoc target, int *travel, int *outward);
static DmtxPassFail bresLineStep(DmtxBresLine *line, int travel, int outward);
/*static void WriteDiagnosticImage(DmtxDecode *dec, DmtxRegion *reg, char *imagePath);*/

/* dmtxdecode.c */
static void tallyModuleJumps(DmtxDecode *dec, DmtxRegion *reg, int tally[][24], int xOrigin, int yOrigin, int mapWidth,
                             int mapHeight, DmtxDirection dir);
static DmtxPassFail populateArrayFromMatrix(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);

/* dmtxdecodescheme.c */
static DmtxPassFail decodeDataStream(DmtxMessage *msg, int sizeIdx, unsigned char *outputStart);
static int getEncodationScheme(unsigned char cw);
static void pushOutputWord(DmtxMessage *msg, int value);
static void pushOutputC40TextWord(DmtxMessage *msg, C40TextState *state, int value);
static void pushOutputMacroHeader(DmtxMessage *msg, int macroType);
static void pushOutputMacroTrailer(DmtxMessage *msg);
static unsigned char *decodeSchemeAscii(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd);
static unsigned char *decodeSchemeC40Text(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd,
                                          DmtxScheme encScheme);
static unsigned char *decodeSchemeX12(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd);
static unsigned char *decodeSchemeEdifact(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd);
static unsigned char *decodeSchemeBase256(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd);

/* dmtxencode.c */
static void printPattern(DmtxEncode *encode);
static int encodeDataCodewords(DmtxByteList *input, DmtxByteList *output, int sizeIdxRequest, DmtxScheme scheme,
                               int fnc1);

/* dmtxplacemod.c */
static int modulePlacementEcc200(unsigned char *modules, unsigned char *codewords, int sizeIdx, int moduleOnColor);
static void patternShapeStandard(unsigned char *modules, int mappingRows, int mappingCols, int row, int col,
                                 unsigned char *codeword, int moduleOnColor);
static void patternShapeSpecial1(unsigned char *modules, int mappingRows, int mappingCols, unsigned char *codeword,
                                 int moduleOnColor);
static void patternShapeSpecial2(unsigned char *modules, int mappingRows, int mappingCols, unsigned char *codeword,
                                 int moduleOnColor);
static void patternShapeSpecial3(unsigned char *modules, int mappingRows, int mappingCols, unsigned char *codeword,
                                 int moduleOnColor);
static void patternShapeSpecial4(unsigned char *modules, int mappingRows, int mappingCols, unsigned char *codeword,
                                 int moduleOnColor);
static void placeModule(unsigned char *modules, int mappingRows, int mappingCols, int row, int col,
                        unsigned char *codeword, int mask, int moduleOnColor);

/* dmtxreedsol.c */
static DmtxPassFail rsEncode(DmtxMessage *message, int sizeIdx);
static DmtxPassFail rsDecode(unsigned char *code, int sizeIdx, int fix);
static DmtxPassFail rsGenPoly(DmtxByteList *gen, int errorWordCount);
static DmtxBoolean rsComputeSyndromes(DmtxByteList *syn, const DmtxByteList *rec, int blockErrorWords);
static DmtxBoolean rsFindErrorLocatorPoly(DmtxByteList *elp, const DmtxByteList *syn, int errorWordCount,
                                          int maxCorrectable);
static DmtxBoolean rsFindErrorLocations(DmtxByteList *loc, const DmtxByteList *elp);
static DmtxPassFail rsRepairErrors(DmtxByteList *rec, const DmtxByteList *loc, const DmtxByteList *elp,
                                   const DmtxByteList *syn);

/* dmtxscangrid.c */
static DmtxScanGrid initScanGrid(DmtxDecode *dec);
static int popGridLocation(DmtxScanGrid *grid, /*@out@*/ DmtxPixelLoc *locPtr);
static int getGridCoordinates(DmtxScanGrid *grid, /*@out@*/ DmtxPixelLoc *locPtr);
static void setDerivedFields(DmtxScanGrid *grid);

/* dmtxsymbol.c */
static int findSymbolSize(int dataWords, int sizeIdxRequest);

/* dmtximage.c */
static int getBitsPerPixel(int pack);

/* dmtxencodestream.c */
static DmtxEncodeStream streamInit(DmtxByteList *input, DmtxByteList *output);
static void streamCopy(DmtxEncodeStream *dst, DmtxEncodeStream *src);
static void streamMarkComplete(DmtxEncodeStream *stream, int sizeIdx);
static void streamMarkInvalid(DmtxEncodeStream *stream, int reasonIdx);
static void streamMarkFatal(DmtxEncodeStream *stream, int reasonIdx);
static void streamOutputChainAppend(DmtxEncodeStream *stream, DmtxByte value);
static DmtxByte streamOutputChainRemoveLast(DmtxEncodeStream *stream);
static void streamOutputSet(DmtxEncodeStream *stream, int index, DmtxByte value);
static DmtxBoolean streamInputHasNext(DmtxEncodeStream *stream);
static DmtxByte streamInputPeekNext(DmtxEncodeStream *stream);
static DmtxByte streamInputAdvanceNext(DmtxEncodeStream *stream);
static void streamInputAdvancePrev(DmtxEncodeStream *stream);

/* dmtxencodescheme.c */
static int encodeSingleScheme(DmtxByteList *input, DmtxByteList *output, int sizeIdxRequest, DmtxScheme scheme,
                              int fnc1);
static void encodeNextChunk(DmtxEncodeStream *stream, int scheme, int subScheme, int sizeIdxRequest);
static void encodeChangeScheme(DmtxEncodeStream *stream, DmtxScheme targetScheme, int unlatchType);
static int getRemainingSymbolCapacity(int outputLength, int sizeIdx);

/* dmtxencodeoptimize.c */
static int encodeOptimizeBest(DmtxByteList *input, DmtxByteList *output, int sizeIdxRequest, int fnc1);
static void streamAdvanceFromBest(DmtxEncodeStream *streamNext, DmtxEncodeStream *streamList, int targeteState,
                                  int sizeIdxRequest);
static void advanceAsciiCompact(DmtxEncodeStream *streamNext, DmtxEncodeStream *streamList, int state, int inputNext,
                                int sizeIdxRequest);
static void advanceCTX(DmtxEncodeStream *streamNext, DmtxEncodeStream *streamList, int state, int inputNext,
                       int ctxValueCount, int sizeIdxRequest);
static void advanceEdifact(DmtxEncodeStream *streamNext, DmtxEncodeStream *streamList, int state, int inputNext,
                           int sizeIdxRequest);
static int getScheme(int state);
static DmtxBoolean validStateSwitch(int fromState, int targetState);

/* dmtxencodeascii.c */
static void encodeNextChunkAscii(DmtxEncodeStream *stream, int option);
static void appendValueAscii(DmtxEncodeStream *stream, DmtxByte value);
static void completeIfDoneAscii(DmtxEncodeStream *stream, int sizeIdxRequest);
static void padRemainingInAscii(DmtxEncodeStream *stream, int sizeIdx);
static DmtxByteList encodeTmpRemainingInAscii(DmtxEncodeStream *stream, DmtxByte *storage, int capacity,
                                              DmtxPassFail *passFail);
static DmtxByte randomize253State(DmtxByte cwValue, int cwPosition);

/* dmtxencodec40textx12.c */
static void encodeNextChunkCTX(DmtxEncodeStream *stream, int sizeIdxRequest);
static void appendValuesCTX(DmtxEncodeStream *stream, DmtxByteList *valueList);
static void appendUnlatchCTX(DmtxEncodeStream *stream);
static void completeIfDoneCTX(DmtxEncodeStream *stream, int sizeIdxRequest);
static void completePartialC40Text(DmtxEncodeStream *stream, DmtxByteList *valueList, int sizeIdxRequest);
static void completePartialX12(DmtxEncodeStream *stream, DmtxByteList *valueList, int sizeIdxRequest);
static DmtxBoolean partialX12ChunkRemains(DmtxEncodeStream *stream);
static void pushCTXValues(DmtxByteList *valueList, DmtxByte inputValue, int targetScheme, DmtxPassFail *passFail,
                          int fnc1);
static DmtxBoolean isCTX(int scheme);
static void shiftValueListBy3(DmtxByteList *list, DmtxPassFail *passFail);

/* dmtxencodeedifact.c */
static void encodeNextChunkEdifact(DmtxEncodeStream *stream);
static void appendValueEdifact(DmtxEncodeStream *stream, DmtxByte value);
static void completeIfDoneEdifact(DmtxEncodeStream *stream, int sizeIdxRequest);

/* dmtxencodebase256.c */
static void encodeNextChunkBase256(DmtxEncodeStream *stream);
static void appendValueBase256(DmtxEncodeStream *stream, DmtxByte value);
static void completeIfDoneBase256(DmtxEncodeStream *stream, int sizeIdxRequest);
static void updateBase256ChainHeader(DmtxEncodeStream *stream, int perfectSizeIdx);
static void base256OutputChainInsertFirst(DmtxEncodeStream *stream);
static void base256OutputChainRemoveFirst(DmtxEncodeStream *stream);
static DmtxByte randomize255State(DmtxByte cwValue, int cwPosition);
static unsigned char unRandomize255State(unsigned char value, int idx);

static const int dmtxNeighborNone = 8;
static const int dmtxPatternX[] = {-1, 0, 1, 1, 1, 0, -1, -1};
static const int dmtxPatternY[] = {-1, -1, -1, 0, 1, 1, 1, 0};
static const DmtxPointFlow dmtxBlankEdge = {0, 0, 0, DmtxUndefined, {-1, -1}};

#ifdef DEBUG_CALLBACK
static DmtxCallbackBuildMatrixRegion cbBuildMatrixRegion = NULL;
static DmtxCallbackBuildMatrix cbBuildMatrix = NULL;
static DmtxCallbackPlotPoint cbPlotPoint = NULL;
static DmtxCallbackXfrmPlotPoint cbXfrmPlotPoint = NULL;
static DmtxCallbackFinal cbFinal = NULL;
#endif

/*@ +charint @*/

static int rHvX[] = {
    256,  256,  256,  256,  255,  255,  255,  254,  254,  253,  252,  251,  250,  249,  248,  247,  246,  245,
    243,  242,  241,  239,  237,  236,  234,  232,  230,  228,  226,  224,  222,  219,  217,  215,  212,  210,
    207,  204,  202,  199,  196,  193,  190,  187,  184,  181,  178,  175,  171,  168,  165,  161,  158,  154,
    150,  147,  143,  139,  136,  132,  128,  124,  120,  116,  112,  108,  104,  100,  96,   92,   88,   83,
    79,   75,   71,   66,   62,   58,   53,   49,   44,   40,   36,   31,   27,   22,   18,   13,   9,    4,
    0,    -4,   -9,   -13,  -18,  -22,  -27,  -31,  -36,  -40,  -44,  -49,  -53,  -58,  -62,  -66,  -71,  -75,
    -79,  -83,  -88,  -92,  -96,  -100, -104, -108, -112, -116, -120, -124, -128, -132, -136, -139, -143, -147,
    -150, -154, -158, -161, -165, -168, -171, -175, -178, -181, -184, -187, -190, -193, -196, -199, -202, -204,
    -207, -210, -212, -215, -217, -219, -222, -224, -226, -228, -230, -232, -234, -236, -237, -239, -241, -242,
    -243, -245, -246, -247, -248, -249, -250, -251, -252, -253, -254, -254, -255, -255, -255, -256, -256, -256};

static int rHvY[] = {0,   4,   9,   13,  18,  22,  27,  31,  36,  40,  44,  49,  53,  58,  62,  66,  71,  75,  79,  83,
                     88,  92,  96,  100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 139, 143, 147, 150, 154, 158, 161,
                     165, 168, 171, 175, 178, 181, 184, 187, 190, 193, 196, 199, 202, 204, 207, 210, 212, 215, 217, 219,
                     222, 224, 226, 228, 230, 232, 234, 236, 237, 239, 241, 242, 243, 245, 246, 247, 248, 249, 250, 251,
                     252, 253, 254, 254, 255, 255, 255, 256, 256, 256, 256, 256, 256, 256, 255, 255, 255, 254, 254, 253,
                     252, 251, 250, 249, 248, 247, 246, 245, 243, 242, 241, 239, 237, 236, 234, 232, 230, 228, 226, 224,
                     222, 219, 217, 215, 212, 210, 207, 204, 202, 199, 196, 193, 190, 187, 184, 181, 178, 175, 171, 168,
                     165, 161, 158, 154, 150, 147, 143, 139, 136, 132, 128, 124, 120, 116, 112, 108, 104, 100, 96,  92,
                     88,  83,  79,  75,  71,  66,  62,  58,  53,  49,  44,  40,  36,  31,  27,  22,  18,  13,  9,   4};

/*@ -charint @*/

enum DmtxErrorMessage
{
    DmtxErrorUnknown,
    DmtxErrorUnsupportedCharacter,
    DmtxErrorNotOnByteBoundary,
    DmtxErrorIllegalParameterValue,
    DmtxErrorEmptyList,
    DmtxErrorOutOfBounds,
    DmtxErrorMessageTooLarge,
    DmtxErrorCantCompactNonDigits,
    DmtxErrorUnexpectedScheme,
    DmtxErrorIncompleteValueList
};

static char *dmtxErrorMessage[] = {"Unknown error",
                                   "Unsupported character",
                                   "Not on byte boundary",
                                   "Illegal parameter value",
                                   "Encountered empty list",
                                   "Out of bounds",
                                   "Message too large",
                                   "Can't compact non-digits",
                                   "Encountered unexpected scheme",
                                   "Encountered incomplete value list"};

#endif
