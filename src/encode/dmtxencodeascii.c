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
 * \file dmtxencodeascii.c
 * \brief ASCII encoding rules
 */

#include <assert.h>

#include "dmtx.h"
#include "dmtxstatic.h"

/**
 * \brief Simple single scheme encoding uses "Normal"
 * The optimizer needs to track "Expanded" and "Compact" streams separately, so they
 * are called explicitly.
 *
 *   Normal:   Automatically collapses 2 consecutive digits into one codeword
 *   Expanded: Uses a whole codeword to represent a digit (never collapses)
 *   Compact:  Collapses 2 digits into a single codeword or marks the stream
 *             invalid if either values are not digits
 *
 * \param stream
 * \param option [Expanded|Compact|Normal]
 */
static void encodeNextChunkAscii(DmtxEncodeStream *stream, int option)
{
    DmtxByte v0, v1;
    DmtxBoolean compactDigits;

    if (streamInputHasNext(stream)) {
        v0 = streamInputAdvanceNext(stream);
        CHKERR;

        if ((option == DmtxEncodeCompact || option == DmtxEncodeNormal) && streamInputHasNext(stream)) {
            v1 = streamInputPeekNext(stream);
            CHKERR;

            /* Check for FNC1 character */
            if (stream->fnc1 != DmtxUndefined && (int)v1 == stream->fnc1) {
                v1 = 0;
                compactDigits = DmtxFalse;
            } else {
                compactDigits = (ISDIGIT(v0) && ISDIGIT(v1)) ? DmtxTrue : DmtxFalse;
            }
        } else /* option == DmtxEncodeFull */
        {
            v1 = 0;
            compactDigits = DmtxFalse;
        }

        if (compactDigits == DmtxTrue) {
            /* Two adjacent digit chars: Make peek progress official and encode */
            streamInputAdvanceNext(stream);
            CHKERR;
            appendValueAscii(stream, 10 * (v0 - '0') + (v1 - '0') + 130);
            CHKERR;
        } else if (option == DmtxEncodeCompact) {
            /* Can't compact non-digits */
            streamMarkInvalid(stream, DmtxErrorCantCompactNonDigits);
        } else {
            /* Encode single ASCII value */
            if (stream->fnc1 != DmtxUndefined && (int)v0 == stream->fnc1) {
                /* FNC1 */
                appendValueAscii(stream, DmtxValueFNC1);
                CHKERR;
            } else if (v0 < 128) {
                /* Regular ASCII */
                appendValueAscii(stream, v0 + 1);
                CHKERR;
            } else {
                /* Extended ASCII */
                appendValueAscii(stream, DmtxValueAsciiUpperShift);
                CHKERR;
                appendValueAscii(stream, v0 - 127);
                CHKERR;
            }
        }
    }
}

/**
 * this code is separated from encodeNextChunkAscii() because it needs to be
 * called directly elsewhere
 */
static void appendValueAscii(DmtxEncodeStream *stream, DmtxByte value)
{
    CHKSCHEME(DmtxSchemeAscii);

    streamOutputChainAppend(stream, value);
    CHKERR;
    stream->outputChainValueCount++;
}

/**
 *
 *
 */
static void completeIfDoneAscii(DmtxEncodeStream *stream, int sizeIdxRequest)
{
    int sizeIdx;

    if (stream->status == DmtxStatusComplete) {
        return;
    }

    if (!streamInputHasNext(stream)) {
        sizeIdx = findSymbolSize(stream->output->length, sizeIdxRequest);
        CHKSIZE;
        padRemainingInAscii(stream, sizeIdx);
        CHKERR;
        streamMarkComplete(stream, sizeIdx);
    }
}

/**
 * Can we just receive a length to pad here? I don't like receiving
 * sizeIdxRequest (or sizeIdx) this late in the game
 */
static void padRemainingInAscii(DmtxEncodeStream *stream, int sizeIdx)
{
    int symbolRemaining;
    DmtxByte padValue;

    CHKSCHEME(DmtxSchemeAscii);
    CHKSIZE;

    symbolRemaining = getRemainingSymbolCapacity(stream->output->length, sizeIdx);

    /* First pad character is not randomized */
    if (symbolRemaining > 0) {
        padValue = DmtxValueAsciiPad;
        streamOutputChainAppend(stream, padValue);
        CHKERR;
        symbolRemaining--;
    }

    /* All remaining pad characters are randomized based on character position */
    while (symbolRemaining > 0) {
        padValue = randomize253State(DmtxValueAsciiPad, stream->output->length + 1);
        streamOutputChainAppend(stream, padValue);
        CHKERR;
        symbolRemaining--;
    }
}

/**
 * consider receiving instantiated DmtxByteList instead of the output components
 */
static DmtxByteList encodeTmpRemainingInAscii(DmtxEncodeStream *stream, DmtxByte *storage, int capacity,
                                              DmtxPassFail *passFail)
{
    DmtxEncodeStream streamAscii;
    DmtxByteList output = dmtxByteListBuild(storage, capacity);

    /* Create temporary copy of stream that writes to storage */
    streamAscii = *stream;
    streamAscii.currentScheme = DmtxSchemeAscii;
    streamAscii.outputChainValueCount = 0;
    streamAscii.outputChainWordCount = 0;
    streamAscii.reason = NULL;
    streamAscii.sizeIdx = DmtxUndefined;
    streamAscii.status = DmtxStatusEncoding;
    streamAscii.output = &output;

    while (dmtxByteListHasCapacity(streamAscii.output)) {
        if (streamInputHasNext(&streamAscii)) {
            encodeNextChunkAscii(&streamAscii, DmtxEncodeNormal); /* No CHKERR */
        } else {
            break;
        }
    }

    /*
     * We stopped encoding before attempting to write beyond output boundary so
     * any stream errors are truly unexpected. The passFail status indicates
     * whether output.length can be trusted by the calling function.
     */

    if (streamAscii.status == DmtxStatusInvalid || streamAscii.status == DmtxStatusFatal) {
        *passFail = DmtxFail;
    } else {
        *passFail = DmtxPass;
    }

    return output;
}

/**
 * \brief Randomize 253 state
 * \param cwValue codewordValue
 * \param cwPosition codewordPosition
 * \return Randomized value
 */
static DmtxByte randomize253State(DmtxByte cwValue, int cwPosition)
{
    int pseudoRandom, tmp;

    pseudoRandom = ((149 * cwPosition) % 253) + 1;
    tmp = cwValue + pseudoRandom;
    if (tmp > 254) {
        tmp -= 254;
    }

    DmtxAssert(tmp >= 0 && tmp < 256);

    return (DmtxByte)tmp;
}
