/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2011 Mike Laughton. All rights reserved.
 * Copyright 2012-2016 Vadim A. Misbakh-Soloviov. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact:
 * Vadim A. Misbakh-Soloviov <dmtx@mva.name>
 * Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxencodeedifact.c
 * \brief Edifact encoding rules
 */

#include "dmtx.h"
#include "dmtxstatic.h"

/**
 *
 *
 */
static void encodeNextChunkEdifact(DmtxEncodeStream *stream)
{
    DmtxByte value;

    if (streamInputHasNext(stream)) {
        /* Check for FNC1 character, which needs to be sent in ASCII */
        value = streamInputPeekNext(stream);
        CHKERR;

        if ((value < 32 || value > 94)) {
            streamMarkInvalid(stream, DmtxChannelUnsupportedChar);
            return;
        }

        if (stream->fnc1 != DmtxUndefined && (int)value == stream->fnc1) {
            encodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchExplicit);
            CHKERR;

            streamInputAdvanceNext(stream);
            CHKERR;
            appendValueAscii(stream, DmtxValueFNC1);
            CHKERR;
            return;
        }

        value = streamInputAdvanceNext(stream);
        CHKERR;
        appendValueEdifact(stream, value);
        CHKERR;
    }
}

/**
 *
 *
 */
static void appendValueEdifact(DmtxEncodeStream *stream, DmtxByte value)
{
    DmtxByte edifactValue, previousOutput;

    CHKSCHEME(DmtxSchemeEdifact);

    /*
     *  TODO: KECA -> korean, circles
     *  TODO: UNOX -> ISO-2022-JP
     *  TODO: and so on
     */
    if (value < 31 || value > 94) {
        streamMarkInvalid(stream, DmtxChannelUnsupportedChar);
        return;
    }

    edifactValue = (value & 0x3f) << 2;

    switch (stream->outputChainValueCount % 4) {
        case 0:
            streamOutputChainAppend(stream, edifactValue);
            CHKERR;
            break;
        case 1:
            previousOutput = streamOutputChainRemoveLast(stream);
            CHKERR;
            streamOutputChainAppend(stream, previousOutput | (edifactValue >> 6));
            CHKERR;
            streamOutputChainAppend(stream, edifactValue << 2);
            CHKERR;
            break;
        case 2:
            previousOutput = streamOutputChainRemoveLast(stream);
            CHKERR;
            streamOutputChainAppend(stream, previousOutput | (edifactValue >> 4));
            CHKERR;
            streamOutputChainAppend(stream, edifactValue << 4);
            CHKERR;
            break;
        case 3:
            previousOutput = streamOutputChainRemoveLast(stream);
            CHKERR;
            streamOutputChainAppend(stream, previousOutput | (edifactValue >> 2));
            CHKERR;
            break;
    }

    stream->outputChainValueCount++;
}

/**
 * Complete EDIFACT encoding if it matches a known end-of-symbol condition.
 *
 *   Term  Clean  Symbol  ASCII   Codeword
 *   Cond  Bound  Remain  Remain  Sequence
 *   ----  -----  ------  ------  -----------
 *    (a)      Y       0       0  [none]
 *    (b)      Y       1       0  PAD
 *    (c)      Y       1       1  ASCII
 *    (d)      Y       2       0  PAD PAD
 *    (e)      Y       2       1  ASCII PAD
 *    (f)      Y       2       2  ASCII ASCII
 *             -       -       0  UNLATCH
 *
 * If not matching any of the above, continue without doing anything.
 */
static void completeIfDoneEdifact(DmtxEncodeStream *stream, int sizeIdxRequest)
{
    int i;
    int sizeIdx;
    int symbolRemaining;
    DmtxBoolean cleanBoundary;
    DmtxPassFail passFail;
    DmtxByte outputTmpStorage[3];
    DmtxByteList outputTmp;

    if (stream->status == DmtxStatusComplete) {
        return;
    }

    /*
     * If we just completed a triplet (cleanBoundary), 1 or 2 symbol codewords
     * remain, and our remaining inputs (if any) represented in ASCII would fit
     * in the remaining space, encode them in ASCII with an implicit unlatch.
     */

    cleanBoundary = (stream->outputChainValueCount % 4 == 0) ? DmtxTrue : DmtxFalse;

    if (cleanBoundary == DmtxTrue) {
        /* Encode up to 3 codewords to a temporary stream */
        outputTmp = encodeTmpRemainingInAscii(stream, outputTmpStorage, sizeof(outputTmpStorage), &passFail);

        if (passFail == DmtxFail) {
            streamMarkFatal(stream, DmtxErrorUnknown);
            return;
        }

        if (outputTmp.length < 3) {
            /* Find minimum symbol size for projected length */
            sizeIdx = findSymbolSize(stream->output->length + outputTmp.length, sizeIdxRequest);
            CHKSIZE;

            /* Find remaining capacity over current length */
            symbolRemaining = getRemainingSymbolCapacity(stream->output->length, sizeIdx);
            CHKERR;

            if (symbolRemaining < 3 && outputTmp.length <= symbolRemaining) {
                encodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchImplicit);
                CHKERR;

                for (i = 0; i < outputTmp.length; i++) {
                    appendValueAscii(stream, outputTmp.b[i]);
                    CHKERR;
                }

                /* Register progress since encoding happened outside normal path */
                stream->inputNext = stream->input->length;

                /* Pad remaining if necessary */
                padRemainingInAscii(stream, sizeIdx);
                CHKERR;
                streamMarkComplete(stream, sizeIdx);
                return;
            }
        }
    }

    if (!streamInputHasNext(stream)) {
        sizeIdx = findSymbolSize(stream->output->length, sizeIdxRequest);
        CHKSIZE;
        symbolRemaining = getRemainingSymbolCapacity(stream->output->length, sizeIdx);
        CHKERR;

        /* Explicit unlatch required unless on clean boundary and full symbol */
        if (cleanBoundary == DmtxFalse || symbolRemaining > 0) {
            encodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchExplicit);
            CHKERR;
            sizeIdx = findSymbolSize(stream->output->length, sizeIdxRequest);
            CHKSIZE;
            padRemainingInAscii(stream, sizeIdx);
            CHKERR;
        }

        streamMarkComplete(stream, sizeIdx);
    }
}
