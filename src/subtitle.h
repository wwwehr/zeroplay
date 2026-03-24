#ifndef SUBTITLE_H
#define SUBTITLE_H

#include <stdint.h>
#include <pthread.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include "queue.h"

#define SUBTITLE_MAX_CUES  1024
#define SUBTITLE_MAX_TEXT  512

/* ------------------------------------------------------------------ */
/* A single decoded subtitle cue                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    int64_t  start_us;                 /* display start, microseconds  */
    int64_t  end_us;                   /* display end,   microseconds  */
    char     text[SUBTITLE_MAX_TEXT];  /* plain UTF-8, \n-separated    */
} SubtitleCue;

/* ------------------------------------------------------------------ */
/* SubtitleContext                                                       */
/*                                                                      */
/* Two usage modes:                                                     */
/*   1. Embedded stream — demux pushes packets into subtitle_queue,    */
/*      subtitle_run() thread decodes them as they arrive.             */
/*   2. External file   — subtitle_open_file() decodes all cues        */
/*      eagerly at open time; no thread or queue needed.               */
/* ------------------------------------------------------------------ */

typedef struct {
    AVCodecContext  *codec_ctx;
    AVRational       time_base;

    /* Decoded cue store — written by decode thread, read by main     */
    SubtitleCue      cues[SUBTITLE_MAX_CUES];
    int              cue_count;
    pthread_mutex_t  cue_mutex;

    /* Input queue (embedded mode only; NULL for external files)      */
    Queue           *subtitle_queue;
} SubtitleContext;

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

/*
 * Open a subtitle decoder for an embedded AVStream.
 * subtitle_queue receives raw packets from the demux thread.
 * Call subtitle_run() in a dedicated thread to process them.
 */
int  subtitle_open(SubtitleContext *ctx, AVStream *stream,
                   Queue *subtitle_queue);

/*
 * Open and eagerly decode an external subtitle file (SRT, ASS, etc.).
 * All cues are decoded synchronously — no thread needed.
 * base_us offsets all timestamps (pass 0 for normal playback).
 */
int  subtitle_open_file(SubtitleContext *ctx, const char *path,
                        int64_t base_us);

/* Decode thread entry point (embedded mode only) */
void subtitle_run(SubtitleContext *ctx);

/* Clear all decoded cues (call on seek) */
void subtitle_flush(SubtitleContext *ctx);

void subtitle_close(SubtitleContext *ctx);

/*
 * Return active cue text at pts_us, or NULL if no cue is active.
 * The returned pointer is valid until the next subtitle_flush/close.
 * Thread-safe.
 */
const char *subtitle_get_active(SubtitleContext *ctx, int64_t pts_us);

#endif /* SUBTITLE_H */
