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
 * \file dmtxdecodescheme.c
 */
#include <assert.h>
#include <limits.h>

#include "dmtx.h"
#include "dmtxstatic.h"

/**
 * \brief  Translate encoded data stream into final output
 * \param  msg
 * \param  sizeIdx
 * \param  outputStart
 * \return void
 */
extern DmtxPassFail decodeDataStream(DmtxMessage *msg, int sizeIdx, unsigned char *outputStart)
{
    // printf("libdmtx::decodeDataStream()\n");
    // int oned = sqrt(msg->arraySize);
    // for (int i=0; i<msg->arraySize; i++){
    //    printf(" %c.", msg->array[i]);
    //    if (i%oned==oned-1){
    //       printf("\n");
    //    }
    // }

    DmtxBoolean macro = DmtxFalse;
    DmtxScheme encScheme;
    unsigned char *ptr, *dataEnd;

    msg->output = (outputStart == NULL) ? msg->output : outputStart;
    msg->outputIdx = 0;

    ptr = msg->code;
    dataEnd = ptr + dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx);

    /* Print macro header if first codeword triggers it */
    if (*ptr == DmtxValue05Macro || *ptr == DmtxValue06Macro) {
        pushOutputMacroHeader(msg, *ptr);
        macro = DmtxTrue;
    }

    while (ptr < dataEnd) {
        encScheme = getEncodationScheme(*ptr);
        if (encScheme != DmtxSchemeAscii) {
            ptr++;
        }

        switch (encScheme) {
            case DmtxSchemeAscii:
                ptr = decodeSchemeAscii(msg, ptr, dataEnd);
                break;
            case DmtxSchemeC40:
            case DmtxSchemeText:
                ptr = decodeSchemeC40Text(msg, ptr, dataEnd, encScheme);
                break;
            case DmtxSchemeX12:
                ptr = decodeSchemeX12(msg, ptr, dataEnd);
                break;
            case DmtxSchemeEdifact:
                ptr = decodeSchemeEdifact(msg, ptr, dataEnd);
                break;
            case DmtxSchemeBase256:
                ptr = decodeSchemeBase256(msg, ptr, dataEnd);
                break;
            default:
                /* error */
                break;
        }

        if (ptr == NULL) {
            return DmtxFail;
        }
    }

    /* Print macro trailer if required */
    if (macro == DmtxTrue) {
        pushOutputMacroTrailer(msg);
    }

    return DmtxPass;
}

/**
 * \brief  Determine next encodation scheme
 * \param  encScheme
 * \param  cw
 * \return Pointer to next undecoded codeword
 */
static int getEncodationScheme(unsigned char cw)
{
    DmtxScheme encScheme;

    switch (cw) {
        case DmtxValueC40Latch:
            encScheme = DmtxSchemeC40;
            break;
        case DmtxValueTextLatch:
            encScheme = DmtxSchemeText;
            break;
        case DmtxValueX12Latch:
            encScheme = DmtxSchemeX12;
            break;
        case DmtxValueEdifactLatch:
            encScheme = DmtxSchemeEdifact;
            break;
        case DmtxValueBase256Latch:
            encScheme = DmtxSchemeBase256;
            break;
        default:
            encScheme = DmtxSchemeAscii;
            break;
    }

    return encScheme;
}

/**
 *
 *
 */
static void pushOutputWord(DmtxMessage *msg, int value)
{
    DmtxAssert(value >= 0 && value < 256);

    msg->output[msg->outputIdx++] = (unsigned char)value;
}

/**
 *
 *
 */
static DmtxBoolean validOutputWord(int value)
{
    return (value >= 0 && value < 256) ? DmtxTrue : DmtxFalse;
}

/**
 *
 *
 */
static void pushOutputC40TextWord(DmtxMessage *msg, C40TextState *state, int value)
{
    DmtxAssert(value >= 0 && value < 256);

    msg->output[msg->outputIdx] = (unsigned char)value;

    if (state->upperShift == DmtxTrue) {
        DmtxAssert(value < 128);
        msg->output[msg->outputIdx] += 128;
    }

    msg->outputIdx++;

    state->shift = DmtxC40TextBasicSet;
    state->upperShift = DmtxFalse;
}

/**
 *
 *
 */
static void pushOutputMacroHeader(DmtxMessage *msg, int macroType)
{
    pushOutputWord(msg, '[');
    pushOutputWord(msg, ')');
    pushOutputWord(msg, '>');
    pushOutputWord(msg, 30); /* ASCII RS */
    pushOutputWord(msg, '0');

    DmtxAssert(macroType == DmtxValue05Macro || macroType == DmtxValue06Macro);
    if (macroType == DmtxValue05Macro) {
        pushOutputWord(msg, '5');
    } else {
        pushOutputWord(msg, '6');
    }

    pushOutputWord(msg, 29); /* ASCII GS */
}

/**
 *
 *
 */
static void pushOutputMacroTrailer(DmtxMessage *msg)
{
    pushOutputWord(msg, 30); /* ASCII RS */
    pushOutputWord(msg, 4);  /* ASCII EOT */
}

/**
 * \brief  Decode stream assuming standard ASCII encodation
 * \param  msg
 * \param  ptr
 * \param  dataEnd
 * \return Pointer to next undecoded codeword
 *         NULL if an error was detected in the stream
 */
static unsigned char *decodeSchemeAscii(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd)
{
    int upperShift = DmtxFalse;

    while (ptr < dataEnd) {
        int codeword = (int)(*ptr);

        if (getEncodationScheme(*ptr) != DmtxSchemeAscii) {
            return ptr;
        }
        ptr++;

        if (upperShift == DmtxTrue) {
            int pushword = codeword + 127;
            if (validOutputWord(pushword) != DmtxTrue) {
                return NULL;
            }
            pushOutputWord(msg, pushword);
            upperShift = DmtxFalse;
        } else if (codeword == DmtxValueAsciiUpperShift) {
            upperShift = DmtxTrue;
        } else if (codeword == DmtxValueAsciiPad) {
            DmtxAssert(dataEnd >= ptr);
            DmtxAssert(dataEnd - ptr <= INT_MAX);
            msg->padCount = (int)(dataEnd - ptr);
            return dataEnd;
        } else if (codeword == 0 || codeword >= 242) {
            return ptr;
        } else if (codeword <= 128) {
            pushOutputWord(msg, codeword - 1);
        } else if (codeword <= 229) {
            int digits = codeword - 130;
            pushOutputWord(msg, digits / 10 + '0');
            pushOutputWord(msg, digits - (digits / 10) * 10 + '0');
        } else if (codeword == DmtxValueFNC1) {
            if (msg->fnc1 != DmtxUndefined) {
                int pushword = msg->fnc1;
                if (validOutputWord(pushword) != DmtxTrue) {
                    return NULL;
                }
                pushOutputWord(msg, pushword);
            }
        }
    }

    return ptr;
}

/**
 * \brief  Decode stream assuming C40 or Text encodation
 * \param  msg
 * \param  ptr
 * \param  dataEnd
 * \param  encScheme
 * \return Pointer to next undecoded codeword
 */
static unsigned char *decodeSchemeC40Text(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd,
                                          DmtxScheme encScheme)
{
    int i;
    int packed;
    int c40Values[3];
    C40TextState state;

    state.shift = DmtxC40TextBasicSet;
    state.upperShift = DmtxFalse;

    DmtxAssert(encScheme == DmtxSchemeC40 || encScheme == DmtxSchemeText);

    /* Unlatch is implied if only one codeword remains */
    if (dataEnd - ptr < 2) {
        return ptr;
    }

    while (ptr < dataEnd) {
        /* FIXME Also check that ptr+1 is safe to access */
        packed = (*ptr << 8) | *(ptr + 1);
        c40Values[0] = ((packed - 1) / 1600);
        c40Values[1] = ((packed - 1) / 40) % 40;
        c40Values[2] = (packed - 1) % 40;
        ptr += 2;

        for (i = 0; i < 3; i++) {
            if (state.shift == DmtxC40TextBasicSet) { /* Basic set */
                if (c40Values[i] <= 2) {
                    state.shift = c40Values[i] + 1;
                } else if (c40Values[i] == 3) {
                    pushOutputC40TextWord(msg, &state, ' ');
                } else if (c40Values[i] <= 13) {
                    pushOutputC40TextWord(msg, &state, c40Values[i] - 13 + '9'); /* 0-9 */
                } else if (c40Values[i] <= 39) {
                    if (encScheme == DmtxSchemeC40) {
                        pushOutputC40TextWord(msg, &state, c40Values[i] - 39 + 'Z'); /* A-Z */
                    } else if (encScheme == DmtxSchemeText) {
                        pushOutputC40TextWord(msg, &state, c40Values[i] - 39 + 'z'); /* a-z */
                    }
                }
            } else if (state.shift == DmtxC40TextShift1) {        /* Shift 1 set */
                pushOutputC40TextWord(msg, &state, c40Values[i]); /* ASCII 0 - 31 */
            } else if (state.shift == DmtxC40TextShift2) {        /* Shift 2 set */
                if (c40Values[i] <= 14) {
                    pushOutputC40TextWord(msg, &state, c40Values[i] + 33); /* ASCII 33 - 47 */
                } else if (c40Values[i] <= 21) {
                    pushOutputC40TextWord(msg, &state, c40Values[i] + 43); /* ASCII 58 - 64 */
                } else if (c40Values[i] <= 26) {
                    pushOutputC40TextWord(msg, &state, c40Values[i] + 69); /* ASCII 91 - 95 */
                } else if (c40Values[i] == 27) {
                    if (msg->fnc1 != DmtxUndefined) {
                        pushOutputC40TextWord(msg, &state, msg->fnc1);
                    }
                } else if (c40Values[i] == 30) {
                    state.upperShift = DmtxTrue;
                    state.shift = DmtxC40TextBasicSet;
                }
            } else if (state.shift == DmtxC40TextShift3) { /* Shift 3 set */
                if (encScheme == DmtxSchemeC40) {
                    pushOutputC40TextWord(msg, &state, c40Values[i] + 96);
                } else if (encScheme == DmtxSchemeText) {
                    if (c40Values[i] == 0) {
                        pushOutputC40TextWord(msg, &state, c40Values[i] + 96);
                    } else if (c40Values[i] <= 26) {
                        pushOutputC40TextWord(msg, &state, c40Values[i] - 26 + 'Z'); /* A-Z */
                    } else {
                        pushOutputC40TextWord(msg, &state, c40Values[i] - 31 + 127); /* { | } ~ DEL */
                    }
                }
            }
        }

        /* Unlatch if codeword 254 follows 2 codewords in C40/Text encodation */
        if (*ptr == DmtxValueCTXUnlatch) {
            return ptr + 1;
        }

        /* Unlatch is implied if only one codeword remains */
        if (dataEnd - ptr < 2) {
            return ptr;
        }
    }

    return ptr;
}

/**
 * \brief  Decode stream assuming X12 encodation
 * \param  msg
 * \param  ptr
 * \param  dataEnd
 * \return Pointer to next undecoded codeword
 */
static unsigned char *decodeSchemeX12(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd)
{
    int i;
    int packed;
    int x12Values[3];

    /* Unlatch is implied if only one codeword remains */
    if (dataEnd - ptr < 2) {
        return ptr;
    }

    while (ptr < dataEnd) {
        /* FIXME Also check that ptr+1 is safe to access */
        packed = (*ptr << 8) | *(ptr + 1);
        x12Values[0] = ((packed - 1) / 1600);
        x12Values[1] = ((packed - 1) / 40) % 40;
        x12Values[2] = (packed - 1) % 40;
        ptr += 2;

        for (i = 0; i < 3; i++) {
            if (x12Values[i] == 0) {
                pushOutputWord(msg, 13);
            } else if (x12Values[i] == 1) {
                pushOutputWord(msg, 42);
            } else if (x12Values[i] == 2) {
                pushOutputWord(msg, 62);
            } else if (x12Values[i] == 3) {
                pushOutputWord(msg, 32);
            } else if (x12Values[i] <= 13) {
                pushOutputWord(msg, x12Values[i] + 44);
            } else if (x12Values[i] <= 90) {
                pushOutputWord(msg, x12Values[i] + 51);
            }
        }

        /* Unlatch if codeword 254 follows 2 codewords in C40/Text encodation */
        if (*ptr == DmtxValueCTXUnlatch) {
            return ptr + 1;
        }

        /* Unlatch is implied if only one codeword remains */
        if (dataEnd - ptr < 2) {
            return ptr;
        }
    }

    return ptr;
}

/**
 * \brief  Decode stream assuming EDIFACT encodation
 * \param  msg
 * \param  ptr
 * \param  dataEnd
 * \return Pointer to next undecoded codeword
 */
static unsigned char *decodeSchemeEdifact(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd)
{
    int i;
    unsigned char unpacked[4];

    /* Unlatch is implied if fewer than 3 codewords remain */
    if (dataEnd - ptr < 3) {
        return ptr;
    }

    while (ptr < dataEnd) {
        /* FIXME Also check that ptr+2 is safe to access -- shouldn't be a
           problem because I'm guessing you can guarantee there will always
           be at least 3 error codewords */
        unpacked[0] = (*ptr & 0xfc) >> 2;
        unpacked[1] = (*ptr & 0x03) << 4 | (*(ptr + 1) & 0xf0) >> 4;
        unpacked[2] = (*(ptr + 1) & 0x0f) << 2 | (*(ptr + 2) & 0xc0) >> 6;
        unpacked[3] = *(ptr + 2) & 0x3f;

        for (i = 0; i < 4; i++) {
            /* Advance input ptr (4th value comes from already-read 3rd byte) */
            if (i < 3) {
                ptr++;
            }

            /* Test for unlatch condition */
            if (unpacked[i] == DmtxValueEdifactUnlatch) {
                DmtxAssert(msg->output[msg->outputIdx] == 0); /* XXX dirty why? */
                return ptr;
            }

            pushOutputWord(msg, unpacked[i] ^ (((unpacked[i] & 0x20) ^ 0x20) << 1));
        }

        /* Unlatch is implied if fewer than 3 codewords remain */
        if (dataEnd - ptr < 3) {
            return ptr;
        }
    }

    return ptr;

    /* XXX the following version should be safer, but requires testing before replacing the old version
       int bits = 0;
       int bitCount = 0;
       int value;

       while(ptr < dataEnd) {

          if(bitCount < 6) {
             bits = (bits << 8) | *(ptr++);
             bitCount += 8;
          }

          value = bits >> (bitCount - 6);
          bits -= (value << (bitCount - 6));
          bitCount -= 6;

          if(value == 0x1f) {
             DmtxAssert(bits == 0); // should be padded with zero-value bits
             return ptr;
          }
          pushOutputWord(msg, value ^ (((value & 0x20) ^ 0x20) << 1));

          // Unlatch implied if just completed triplet and 1 or 2 words are left
          if(bitCount == 0 && dataEnd - ptr - 1 > 0 && dataEnd - ptr - 1 < 3)
             return ptr;
       }

       DmtxAssert(bits == 0); // should be padded with zero-value bits
       DmtxAssert(bitCount == 0); // should be padded with zero-value bits
       return ptr;
    */
}

/**
 * \brief  Decode stream assuming Base 256 encodation
 * \param  msg
 * \param  ptr
 * \param  dataEnd
 * \return Pointer to next undecoded codeword,
 *         NULL if an error was detected in the stream
 */
static unsigned char *decodeSchemeBase256(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd)
{
    int d0, d1;
    int idx;
    unsigned char *ptrEnd;

    /* Find positional index used for unrandomizing */
    DmtxAssert(ptr + 1 >= msg->code);
    DmtxAssert(ptr + 1 - msg->code <= INT_MAX);
    idx = (int)(ptr + 1 - msg->code);

    d0 = unRandomize255State(*(ptr++), idx++);
    if (d0 == 0) {
        ptrEnd = dataEnd;
    } else if (d0 <= 249) {
        ptrEnd = ptr + d0;
    } else {
        d1 = unRandomize255State(*(ptr++), idx++);
        ptrEnd = ptr + (d0 - 249) * 250 + d1;
    }

    if (ptrEnd > dataEnd) {
        return NULL;
    }

    while (ptr < ptrEnd) {
        pushOutputWord(msg, unRandomize255State(*(ptr++), idx++));
    }

    return ptr;
}
