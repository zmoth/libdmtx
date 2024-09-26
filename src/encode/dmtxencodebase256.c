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
 * \file dmtxencodebase256.c
 * \brief Base 256 encoding rules
 */

#include <assert.h>

#include "dmtx.h"
#include "dmtxstatic.h"

/**
 *
 *
 */
static void encodeNextChunkBase256(DmtxEncodeStream *stream)
{
    DmtxByte value;

    if (streamInputHasNext(stream)) {
        /* Check for FNC1 character, which needs to be sent in ASCII */
        value = streamInputPeekNext(stream);
        CHKERR;
        if (stream->fnc1 != DmtxUndefined && (int)value == stream->fnc1) {
            encodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchExplicit);

            streamInputAdvanceNext(stream);
            CHKERR;
            appendValueAscii(stream, DmtxValueFNC1);
            CHKERR;
            return;
        }

        value = streamInputAdvanceNext(stream);
        CHKERR;
        appendValueBase256(stream, value);
        CHKERR;
    }
}

/**
 *
 *
 */
static void appendValueBase256(DmtxEncodeStream *stream, DmtxByte value)
{
    CHKSCHEME(DmtxSchemeBase256);

    streamOutputChainAppend(stream, randomize255State(value, stream->output->length + 1));
    CHKERR;
    stream->outputChainValueCount++;

    updateBase256ChainHeader(stream, DmtxUndefined);
    CHKERR;
}

/**
 * check remaining symbol capacity and remaining codewords
 * if the chain can finish perfectly at the end of symbol data words there is a
 * special one-byte length header value that can be used (i think ... read the
 * spec again before commiting to anything)
 */
static void completeIfDoneBase256(DmtxEncodeStream *stream, int sizeIdxRequest)
{
    int sizeIdx;
    int headerByteCount, outputLength, symbolRemaining;

    if (stream->status == DmtxStatusComplete) {
        return;
    }

    if (!streamInputHasNext(stream)) {
        headerByteCount = stream->outputChainWordCount - stream->outputChainValueCount;
        DmtxAssert(headerByteCount == 1 || headerByteCount == 2);

        /* Check for special case where every last symbol word is used */
        if (headerByteCount == 2) {
            /* Find symbol size as if headerByteCount was only 1 */
            outputLength = stream->output->length - 1;
            sizeIdx = findSymbolSize(outputLength, sizeIdxRequest); /* No CHKSIZE */
            if (sizeIdx != DmtxUndefined) {
                symbolRemaining = getRemainingSymbolCapacity(outputLength, sizeIdx);

                if (symbolRemaining == 0) {
                    /* Perfect fit -- complete encoding */
                    updateBase256ChainHeader(stream, sizeIdx);
                    CHKERR;
                    streamMarkComplete(stream, sizeIdx);
                    return;
                }
            }
        }

        /* Normal case */
        sizeIdx = findSymbolSize(stream->output->length, sizeIdxRequest);
        CHKSIZE;
        encodeChangeScheme(stream, DmtxSchemeAscii, DmtxUnlatchImplicit);
        padRemainingInAscii(stream, sizeIdx);
        streamMarkComplete(stream, sizeIdx);
    }
}

/**
 *
 *
 */
static void updateBase256ChainHeader(DmtxEncodeStream *stream, int perfectSizeIdx)
{
    int headerIndex;
    int outputLength;
    int headerByteCount;
    int symbolDataWords;
    DmtxBoolean perfectFit;
    DmtxByte headerValue0;
    DmtxByte headerValue1;

    outputLength = stream->outputChainValueCount;
    headerIndex = stream->output->length - stream->outputChainWordCount;
    headerByteCount = stream->outputChainWordCount - stream->outputChainValueCount;
    perfectFit = (perfectSizeIdx == DmtxUndefined) ? DmtxFalse : DmtxTrue;

    /*
     * If requested perfect fit verify symbol capacity against final length
     */

    if (perfectFit) {
        symbolDataWords = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, perfectSizeIdx);
        if (symbolDataWords != stream->output->length - 1) {
            streamMarkFatal(stream, DmtxErrorUnknown);
            return;
        }
    }

    /*
     * Adjust header to hold correct number of bytes, not worrying about the
     * values held there until below. Note: Header bytes are not considered
     * scheme "values" so we can insert or remove them without updating the
     * outputChainValueCount.
     */

    if (headerByteCount == 0 && stream->outputChainWordCount == 0) {
        /* No output words written yet -- insert single header byte */
        streamOutputChainAppend(stream, 0);
        CHKERR;
        headerByteCount++;
    } else if (!perfectFit && headerByteCount == 1 && outputLength > 249) {
        /* Beyond 249 bytes requires a second header byte */
        base256OutputChainInsertFirst(stream);
        CHKERR;
        headerByteCount++;
    } else if (perfectFit && headerByteCount == 2) {
        /* Encoding to exact end of symbol only requires single byte */
        base256OutputChainRemoveFirst(stream);
        CHKERR;
        headerByteCount--;
    }

    /*
     * Encode header byte(s) with current length
     */

    if (!perfectFit && headerByteCount == 1 && outputLength <= 249) {
        /* Normal condition for chain length < 250 bytes */
        headerValue0 = randomize255State(outputLength, headerIndex + 1);
        streamOutputSet(stream, headerIndex, headerValue0);
        CHKERR;
    } else if (!perfectFit && headerByteCount == 2 && outputLength > 249) {
        /* Normal condition for chain length >= 250 bytes */
        headerValue0 = randomize255State(outputLength / 250 + 249, headerIndex + 1);
        streamOutputSet(stream, headerIndex, headerValue0);
        CHKERR;

        headerValue1 = randomize255State(outputLength % 250, headerIndex + 2);
        streamOutputSet(stream, headerIndex + 1, headerValue1);
        CHKERR;
    } else if (perfectFit && headerByteCount == 1) {
        /* Special condition when Base 256 stays in effect to end of symbol */
        headerValue0 = randomize255State(0, headerIndex + 1); /* XXX replace magic value 0? */
        streamOutputSet(stream, headerIndex, headerValue0);
        CHKERR;
    } else {
        streamMarkFatal(stream, DmtxErrorUnknown);
        return;
    }
}

/**
 * insert element at beginning of chain, shifting all following elements forward by one
 * used for binary length changes
 */
static void base256OutputChainInsertFirst(DmtxEncodeStream *stream)
{
    DmtxByte value;
    DmtxPassFail passFail;
    int i, chainStart;

    chainStart = stream->output->length - stream->outputChainWordCount;
    dmtxByteListPush(stream->output, 0, &passFail);
    if (passFail == DmtxPass) {
        for (i = stream->output->length - 1; i > chainStart; i--) {
            value = unRandomize255State(stream->output->b[i - 1], i);
            stream->output->b[i] = randomize255State(value, i + 1);
        }

        stream->outputChainWordCount++;
    } else {
        streamMarkFatal(stream, DmtxErrorUnknown);
    }
}

/**
 * remove first element from chain, shifting all following elements back by one
 * used for binary length changes end condition
 */
static void base256OutputChainRemoveFirst(DmtxEncodeStream *stream)
{
    DmtxByte value;
    DmtxPassFail passFail;
    int i, chainStart;

    chainStart = stream->output->length - stream->outputChainWordCount;

    for (i = chainStart; i < stream->output->length - 1; i++) {
        value = unRandomize255State(stream->output->b[i + 1], i + 2);
        stream->output->b[i] = randomize255State(value, i + 1);
    }

    dmtxByteListPop(stream->output, &passFail);
    if (passFail == DmtxPass) {
        stream->outputChainWordCount--;
    } else {
        streamMarkFatal(stream, DmtxErrorUnknown);
    }
}

/**
 * \brief Randomize 255 state
 * \param value
 * \param position
 * \return Randomized value
 */
static DmtxByte randomize255State(DmtxByte value, int position)
{
    int pseudoRandom, tmp;

    pseudoRandom = ((149 * position) % 255) + 1;
    tmp = value + pseudoRandom;

    return (tmp <= 255) ? tmp : tmp - 256;
}

/**
 * \brief Unrandomize 255 state
 * \param value
 * \param idx
 * \return Unrandomized value
 */
static unsigned char unRandomize255State(unsigned char value, int idx)
{
    int pseudoRandom;
    int tmp;

    pseudoRandom = ((149 * idx) % 255) + 1;
    tmp = value - pseudoRandom;
    if (tmp < 0) {
        tmp += 256;
    }

    DmtxAssert(tmp >= 0 && tmp < 256);

    return (unsigned char)tmp;
}
