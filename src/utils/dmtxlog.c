/*
 * Copyright (c) 2020 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dmtx.h"
#include "dmtxstatic.h"

#define MAX_CALLBACKS 32

#if _WIN32
#    define DmtxPrint fprintf_s
#else
#    define DmtxPrint fprintf
#endif

typedef struct
{
    DmtxLogFn fn;
    void *udata;
    int level;
} Callback;

static struct
{
    void *udata;
    DmtxLogLockFn lock;
    int level;
    unsigned int quiet;
    Callback callbacks[MAX_CALLBACKS];
} l;

static const char *levelStrings[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

#ifdef LOG_USE_COLOR
static const char *level_colors[] = {"\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"};
#endif

static void stdoutCallback(DmtxLogEvent *ev)
{
    char buf[16];
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
#ifdef LOG_USE_COLOR
    DmtxPrint(ev->udata, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ", buf, level_colors[ev->level],
              level_strings[ev->level], ev->file, ev->line);
#else
    DmtxPrint(ev->udata, "%s %-5s %s:%d: ", buf, levelStrings[ev->level], ev->file, ev->line);
#endif
    vfprintf(ev->udata, ev->fmt, ev->ap);
    DmtxPrint(ev->udata, "\n");
    fflush(ev->udata);
}

static void fileCallback(DmtxLogEvent *ev)
{
    char buf[64];
    buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';
    DmtxPrint(ev->udata, "%s %-5s %s:%d: ", buf, levelStrings[ev->level], ev->file, ev->line);
    vfprintf(ev->udata, ev->fmt, ev->ap);
    DmtxPrint(ev->udata, "\n");
    fflush(ev->udata);
}

static void lock(void)
{
    if (l.lock) {
        l.lock(1, l.udata);
    }
}

static void unlock(void)
{
    if (l.lock) {
        l.lock(0, l.udata);
    }
}

const char *dmtxLogLevelString(int level)
{
    return levelStrings[level];
}

void dmtxLogSetLock(DmtxLogLockFn fn, void *udata)
{
    l.lock = fn;
    l.udata = udata;
}

extern void dmtxLogSetLevel(int level)
{
    l.level = level;
}

extern void dmtxLogSetQuiet(DmtxBoolean enable)
{
    l.quiet = enable;
}

int dmtxLogAddCallback(DmtxLogFn fn, void *udata, int level)
{
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (!l.callbacks[i].fn) {
            l.callbacks[i] = (Callback){fn, udata, level};
            return 0;
        }
    }
    return -1;
}

int dmtxLogAddFp(FILE *fp, int level)
{
    return dmtxLogAddCallback(fileCallback, fp, level);
}

static void initEvent(DmtxLogEvent *ev, void *udata)
{
    if (ev->time == NULL) {
        ev->time = malloc(sizeof(struct tm));
    }

    time_t t = time(NULL);
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(ev->time, &t);
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
    localtime_r(&t, ev->time);
#else
#    error "Unsupported platform"
#endif
    ev->udata = udata;
}

extern void dmtxLog(int level, const char *file, int line, const char *fmt, ...)
{
    DmtxLogEvent ev = {.fmt = fmt, .file = file, .line = line, .level = level, .time = malloc(sizeof(struct tm))};

    lock();

    if (!l.quiet && level >= l.level) {
        initEvent(&ev, stderr);
        va_start(ev.ap, fmt);
        stdoutCallback(&ev);
        va_end(ev.ap);
    }

    for (int i = 0; i < MAX_CALLBACKS && l.callbacks[i].fn; i++) {
        Callback *cb = &l.callbacks[i];
        if (level >= cb->level) {
            initEvent(&ev, cb->udata);
            va_start(ev.ap, fmt);
            cb->fn(&ev);
            va_end(ev.ap);
        }
    }

    free(ev.time);
    unlock();
}