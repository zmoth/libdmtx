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
 * \file dmtx.c
 * \brief Main libdmtx source file
 */

#include "dmtx.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "config.h"
#include "dmtxstatic.h"

/**
 * Use `#include` to merge the individual `.c` source files into a single combined
 * file during preprocessing. This allows the project to be organized in files
 * of like-functionality while still keeping a clean namespace. Specifically,
 * internal functions can be static without losing the ability to access them
 * "externally" from the other source files in this list.
 */

#include "decode/dmtxdecode.c"
#include "decode/dmtxdecodescheme.c"
#include "dmtxcallback.c"
#include "dmtxmessage.c"
#include "dmtxplacemod.c"
#include "dmtxreedsol.c"
#include "dmtxregion.c"
#include "dmtxscangrid.c"
#include "dmtxsymbol.c"
#include "encode/dmtxencode.c"
#include "encode/dmtxencodeascii.c"
#include "encode/dmtxencodebase256.c"
#include "encode/dmtxencodec40textx12.c"
#include "encode/dmtxencodeedifact.c"
#include "encode/dmtxencodeoptimize.c"
#include "encode/dmtxencodescheme.c"
#include "encode/dmtxencodestream.c"
#include "utils/dmtxbytelist.c"
#include "utils/dmtximage.c"
#include "utils/dmtxlog.c"
#include "utils/dmtxmatrix3.c"
#include "utils/dmtxtime.c"
#include "utils/dmtxvector2.c"

extern char *dmtxVersion(void)
{
    return PACKAGE_VERSION;
}
