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
 * \file dmtxencodeoptimize.c
 * \brief Logic for optimized (multiple scheme) encoding
 */

#include "dmtx.h"
#include "dmtxstatic.h"

#define DUMPSTREAMS 0

enum SchemeState
{
    AsciiFull,
    AsciiCompactOffset0, /* 0 offset from first regular input value */
    AsciiCompactOffset1,
    C40Offset0, /* 0 offset from first expanded C40 value */
    C40Offset1,
    C40Offset2,
    TextOffset0, /* 0 offset from first expanded Text value */
    TextOffset1,
    TextOffset2,
    X12Offset0, /* 0 offset from first expanded X12 value */
    X12Offset1,
    X12Offset2,
    EdifactOffset0, /* 0 offset from first regular input value */
    EdifactOffset1,
    EdifactOffset2,
    EdifactOffset3,
    Base256,
    SchemeStateCount
};

#if DUMPSTREAMS
static void DumpStreams(DmtxEncodeStream *streamBest)
{
    enum SchemeState state;
    char prefix[32];

    fprintf(stdout, "----------------------------------------\n");
    for (state = 0; state < SchemeStateCount; state++) {
        if (streamBest[state].status == DmtxStatusEncoding || streamBest[state].status == DmtxStatusComplete)
            fprintf(stdout, "\"%c\" ", streamBest[state].input->b[streamBest[state].inputNext - 1]);
        else
            fprintf(stdout, "    ");

        switch (streamBest[state].status) {
            case DmtxStatusEncoding:
                snprintf(prefix, sizeof(prefix), "%2d (%s): ", state, " encode ");
                break;
            case DmtxStatusComplete:
                snprintf(prefix, sizeof(prefix), "%2d (%s): ", state, "complete");
                break;
            case DmtxStatusInvalid:
                snprintf(prefix, sizeof(prefix), "%2d (%s): ", state, "invalid ");
                break;
            case DmtxStatusFatal:
                snprintf(prefix, sizeof(prefix), "%2d (%s): ", state, " fatal  ");
                break;
        }
        dmtxByteListPrint(streamBest[state].output, prefix);
    }
}
#endif

/**
 *
 *
 */
static int encodeOptimizeBest(DmtxByteList *input, DmtxByteList *output, int sizeIdxRequest, int fnc1)
{
    enum SchemeState state;
    int inputNext, c40ValueCount, textValueCount, x12ValueCount;
    int sizeIdx;
    DmtxEncodeStream *winner;
    DmtxPassFail passFail;
    DmtxEncodeStream streamsBest[SchemeStateCount];
    DmtxEncodeStream streamsTemp[SchemeStateCount];
    DmtxByte outputsBestStorage[SchemeStateCount][4096];
    DmtxByte outputsTempStorage[SchemeStateCount][4096];
    DmtxByte ctxTempStorage[4];
    DmtxByteList outputsBest[SchemeStateCount];
    DmtxByteList outputsTemp[SchemeStateCount];
    DmtxByteList ctxTemp = dmtxByteListBuild(ctxTempStorage, sizeof(ctxTempStorage));

    /* Initialize all streams with their own output storage */
    for (state = 0; state < SchemeStateCount; state++) {
        outputsBest[state] = dmtxByteListBuild(outputsBestStorage[state], sizeof(outputsBestStorage[state]));
        outputsTemp[state] = dmtxByteListBuild(outputsTempStorage[state], sizeof(outputsTempStorage[state]));
        streamsBest[state] = streamInit(input, &(outputsBest[state]));
        streamsTemp[state] = streamInit(input, &(outputsTemp[state]));
        streamsBest[state].fnc1 = fnc1;
        streamsTemp[state].fnc1 = fnc1;
    }

    c40ValueCount = textValueCount = x12ValueCount = 0;

    for (inputNext = 0; inputNext < input->length; inputNext++) {
        streamAdvanceFromBest(streamsTemp, streamsBest, AsciiFull, sizeIdxRequest);

        advanceAsciiCompact(streamsTemp, streamsBest, AsciiCompactOffset0, inputNext, sizeIdxRequest);
        advanceAsciiCompact(streamsTemp, streamsBest, AsciiCompactOffset1, inputNext, sizeIdxRequest);

        advanceCTX(streamsTemp, streamsBest, C40Offset0, inputNext, c40ValueCount, sizeIdxRequest);
        advanceCTX(streamsTemp, streamsBest, C40Offset1, inputNext, c40ValueCount, sizeIdxRequest);
        advanceCTX(streamsTemp, streamsBest, C40Offset2, inputNext, c40ValueCount, sizeIdxRequest);

        advanceCTX(streamsTemp, streamsBest, TextOffset0, inputNext, textValueCount, sizeIdxRequest);
        advanceCTX(streamsTemp, streamsBest, TextOffset1, inputNext, textValueCount, sizeIdxRequest);
        advanceCTX(streamsTemp, streamsBest, TextOffset2, inputNext, textValueCount, sizeIdxRequest);

        advanceCTX(streamsTemp, streamsBest, X12Offset0, inputNext, x12ValueCount, sizeIdxRequest);
        advanceCTX(streamsTemp, streamsBest, X12Offset1, inputNext, x12ValueCount, sizeIdxRequest);
        advanceCTX(streamsTemp, streamsBest, X12Offset2, inputNext, x12ValueCount, sizeIdxRequest);

        advanceEdifact(streamsTemp, streamsBest, EdifactOffset0, inputNext, sizeIdxRequest);
        advanceEdifact(streamsTemp, streamsBest, EdifactOffset1, inputNext, sizeIdxRequest);
        advanceEdifact(streamsTemp, streamsBest, EdifactOffset2, inputNext, sizeIdxRequest);
        advanceEdifact(streamsTemp, streamsBest, EdifactOffset3, inputNext, sizeIdxRequest);

        streamAdvanceFromBest(streamsTemp, streamsBest, Base256, sizeIdxRequest);

        /* Overwrite best streams with new results */
        for (state = 0; state < SchemeStateCount; state++) {
            if (streamsBest[state].status != DmtxStatusComplete) {
                streamCopy(&(streamsBest[state]), &(streamsTemp[state]));
            }
        }

        dmtxByteListClear(&ctxTemp);
        pushCTXValues(&ctxTemp, input->b[inputNext], DmtxSchemeC40, &passFail, fnc1);
        c40ValueCount += ((passFail == DmtxPass) ? ctxTemp.length : 1);

        dmtxByteListClear(&ctxTemp);
        pushCTXValues(&ctxTemp, input->b[inputNext], DmtxSchemeText, &passFail, fnc1);
        textValueCount += ((passFail == DmtxPass) ? ctxTemp.length : 1);

        dmtxByteListClear(&ctxTemp);
        pushCTXValues(&ctxTemp, input->b[inputNext], DmtxSchemeX12, &passFail, fnc1);
        x12ValueCount += ((passFail == DmtxPass) ? ctxTemp.length : 1);

#if DUMPSTREAMS
        DumpStreams(streamsBest);
#endif
    }

    /* Choose the overall winner */
    winner = NULL;
    for (state = 0; state < SchemeStateCount; state++) {
        if (streamsBest[state].status == DmtxStatusComplete) {
            if (winner == NULL || streamsBest[state].output->length < winner->output->length) {
                winner = &(streamsBest[state]);
            }
        }
    }

    /* Copy winner to output */
    if (winner == NULL) {
        sizeIdx = DmtxUndefined;
    } else {
        dmtxByteListCopy(output, winner->output, &passFail);
        sizeIdx = (passFail == DmtxPass) ? winner->sizeIdx : DmtxUndefined;
    }

    return sizeIdx;
}

/**
 * It's safe to compare output length because all targetState combinations
 * start on same input and encodes same number of inputs. Only difference
 * is the number of latches/unlatches that are also encoded
 */
static void streamAdvanceFromBest(DmtxEncodeStream *streamsNext, DmtxEncodeStream *streamsBest, int targetState,
                                  int sizeIdxRequest)
{
    enum SchemeState fromState;
    DmtxScheme targetScheme;
    DmtxEncodeOption encodeOption;
    DmtxByte outputTempStorage[4096];
    DmtxByteList outputTemp = dmtxByteListBuild(outputTempStorage, sizeof(outputTempStorage));
    DmtxEncodeStream streamTemp;
    DmtxEncodeStream *targetStream = &(streamsNext[targetState]);

    streamTemp.output = &outputTemp; /* Set directly instead of calling streamInit() */
    targetScheme = getScheme(targetState);

    if (targetState == AsciiFull) {
        encodeOption = DmtxEncodeFull;
    } else if (targetState == AsciiCompactOffset0 || targetState == AsciiCompactOffset1) {
        encodeOption = DmtxEncodeCompact;
    } else {
        encodeOption = DmtxEncodeNormal;
    }

    for (fromState = 0; fromState < SchemeStateCount; fromState++) {
        if (streamsBest[fromState].status != DmtxStatusEncoding ||
            validStateSwitch(fromState, targetState) == DmtxFalse) {
            continue;
        }

        streamCopy(&streamTemp, &(streamsBest[fromState]));
        encodeNextChunk(&streamTemp, targetScheme, encodeOption, sizeIdxRequest);

        if (fromState == 0 ||
            (streamTemp.status != DmtxStatusInvalid && streamTemp.output->length < targetStream->output->length)) {
            streamCopy(targetStream, &streamTemp);
        }
    }
}

/**
 *
 */
static void advanceAsciiCompact(DmtxEncodeStream *streamsNext, DmtxEncodeStream *streamsBest, int targetState,
                                int inputNext, int sizeIdxRequest)
{
    DmtxEncodeStream *currentStream = &(streamsBest[targetState]);
    DmtxEncodeStream *targetStream = &(streamsNext[targetState]);
    DmtxBoolean isStartState;

    switch (targetState) {
        case AsciiCompactOffset0:
            isStartState = (inputNext % 2 == 0) ? DmtxTrue : DmtxFalse;
            break;

        case AsciiCompactOffset1:
            isStartState = (inputNext % 2 == 1) ? DmtxTrue : DmtxFalse;
            break;

        default:
            streamMarkFatal(targetStream, DmtxErrorIllegalParameterValue);
            return;
    }

    if (inputNext < currentStream->inputNext) {
        streamCopy(targetStream, currentStream);
    } else if (isStartState == DmtxTrue) {
        streamAdvanceFromBest(streamsNext, streamsBest, targetState, sizeIdxRequest);
    } else {
        streamCopy(targetStream, currentStream);
        streamMarkInvalid(targetStream, DmtxErrorUnknown);
    }
}

/**
 *
 */
static void advanceCTX(DmtxEncodeStream *streamsNext, DmtxEncodeStream *streamsBest, int targetState, int inputNext,
                       int ctxValueCount, int sizeIdxRequest)
{
    DmtxEncodeStream *currentStream = &(streamsBest[targetState]);
    DmtxEncodeStream *targetStream = &(streamsNext[targetState]);
    DmtxBoolean isStartState;

    /* we won't actually use inputNext here */
    switch (targetState) {
        case C40Offset0:
        case TextOffset0:
        case X12Offset0:
            isStartState = (ctxValueCount % 3 == 0) ? DmtxTrue : DmtxFalse;
            break;

        case C40Offset1:
        case TextOffset1:
        case X12Offset1:
            isStartState = (ctxValueCount % 3 == 1) ? DmtxTrue : DmtxFalse;
            break;

        case C40Offset2:
        case TextOffset2:
        case X12Offset2:
            isStartState = (ctxValueCount % 3 == 2) ? DmtxTrue : DmtxFalse;
            break;

        default:
            streamMarkFatal(targetStream, DmtxErrorIllegalParameterValue);
            return;
    }

    if (inputNext < currentStream->inputNext) {
        streamCopy(targetStream, currentStream);
    } else if (isStartState == DmtxTrue) {
        streamAdvanceFromBest(streamsNext, streamsBest, targetState, sizeIdxRequest);
    } else {
        streamCopy(targetStream, currentStream);
        streamMarkInvalid(targetStream, DmtxErrorUnknown);
    }
}

/**
 *
 */
static void advanceEdifact(DmtxEncodeStream *streamsNext, DmtxEncodeStream *streamsBest, int targetState, int inputNext,
                           int sizeIdxRequest)
{
    DmtxEncodeStream *currentStream = &(streamsBest[targetState]);
    DmtxEncodeStream *targetStream = &(streamsNext[targetState]);
    DmtxBoolean isStartState;

    switch (targetState) {
        case EdifactOffset0:
            isStartState = (inputNext % 4 == 0) ? DmtxTrue : DmtxFalse;
            break;

        case EdifactOffset1:
            isStartState = (inputNext % 4 == 1) ? DmtxTrue : DmtxFalse;
            break;

        case EdifactOffset2:
            isStartState = (inputNext % 4 == 2) ? DmtxTrue : DmtxFalse;
            break;

        case EdifactOffset3:
            isStartState = (inputNext % 4 == 3) ? DmtxTrue : DmtxFalse;
            break;

        default:
            streamMarkFatal(targetStream, DmtxErrorIllegalParameterValue);
            return;
    }

    if (isStartState == DmtxTrue) {
        streamAdvanceFromBest(streamsNext, streamsBest, targetState, sizeIdxRequest);
    } else {
        streamCopy(targetStream, currentStream);
        if (currentStream->status == DmtxStatusEncoding && currentStream->currentScheme == DmtxSchemeEdifact) {
            encodeNextChunk(targetStream, DmtxSchemeEdifact, DmtxEncodeNormal, sizeIdxRequest);
        } else {
            streamMarkInvalid(targetStream, DmtxErrorUnknown);
        }
    }
}

/**
 *
 *
 */
static int getScheme(int state)
{
    DmtxScheme scheme;

    switch (state) {
        case AsciiFull:
        case AsciiCompactOffset0:
        case AsciiCompactOffset1:
            scheme = DmtxSchemeAscii;
            break;
        case C40Offset0:
        case C40Offset1:
        case C40Offset2:
            scheme = DmtxSchemeC40;
            break;
        case TextOffset0:
        case TextOffset1:
        case TextOffset2:
            scheme = DmtxSchemeText;
            break;
        case X12Offset0:
        case X12Offset1:
        case X12Offset2:
            scheme = DmtxSchemeX12;
            break;
        case EdifactOffset0:
        case EdifactOffset1:
        case EdifactOffset2:
        case EdifactOffset3:
            scheme = DmtxSchemeEdifact;
            break;
        case Base256:
            scheme = DmtxSchemeBase256;
            break;
        default:
            scheme = DmtxUndefined;
            break;
    }

    return scheme;
}

/**
 *
 *
 */
static DmtxBoolean validStateSwitch(int fromState, int targetState)
{
    DmtxBoolean validStateSwitch;
    DmtxScheme fromScheme = getScheme(fromState);
    DmtxScheme toScheme = getScheme(targetState);

    if (fromScheme == toScheme && fromState != targetState && fromState != AsciiFull && targetState != AsciiFull) {
        validStateSwitch = DmtxFalse;
    } else {
        validStateSwitch = DmtxTrue;
    }

    return validStateSwitch;
}
