/* ================================================================== */
/* CHANGES TO src/main.c — Phase 1 subtitle wiring                     */
/* ================================================================== */

/* ------------------------------------------------------------------ */
/* 1. Add include near top                                              */
/* ------------------------------------------------------------------ */

#include "subtitle.h"

/* ------------------------------------------------------------------ */
/* 2. Add --sub option to Options struct and parse_args                 */
/* ------------------------------------------------------------------ */

/* In Options struct, add: */
    const char *sub_path;   /* --sub or auto-detected .srt */

/* In long_opts[] array, add: */
    { "sub",  required_argument, NULL, 's' },

/* In getopt switch, add: */
    case 's': opt->sub_path = optarg; break;

/* In print_usage(), add under options: */
    "  --sub path               external subtitle file (.srt)\n"

/* ------------------------------------------------------------------ */
/* 3. Add to PlayerContext struct                                       */
/* ------------------------------------------------------------------ */

    SubtitleContext  sub;
    Queue            sub_queue;
    pthread_t        stid;
    int              sub_active;      /* 1 if subtitle context is open  */
    int              sub_embedded;    /* 1 if using embedded stream      */
    const char      *last_sub_text;   /* for change detection in log     */

/* ------------------------------------------------------------------ */
/* 4. Add subtitle thread trampoline near the other thread functions    */
/* ------------------------------------------------------------------ */

static void *subtitle_thread(void *arg)
{
    PlayerContext *p = ((ThreadArg *)arg)->p; free(arg);
    subtitle_run(&p->sub); return NULL;
}

/* ------------------------------------------------------------------ */
/* 5. Helper: auto-detect .srt alongside video file                    */
/* ------------------------------------------------------------------ */

/*
 * Given /path/to/video.mp4, writes /path/to/video.srt into out.
 * Returns 1 if the file exists and is readable, 0 otherwise.
 */
static int find_srt_alongside(const char *video_path,
                               char *out, int out_size)
{
    strncpy(out, video_path, out_size - 5);
    out[out_size - 5] = '\0';

    /* Strip existing extension */
    char *dot = strrchr(out, '.');
    if (dot) *dot = '\0';

    strncat(out, ".srt", out_size - strlen(out) - 1);
    return access(out, R_OK) == 0;
}

/* ------------------------------------------------------------------ */
/* 6. In player_open_video(), after pipeline is set up, load subtitles */
/* ------------------------------------------------------------------ */

/*
 * Add this block at the END of player_open_video(), just before
 * "return 0;":
 */

    /* ---- Subtitles ---- */
    p->sub_active   = 0;
    p->sub_embedded = 0;
    p->last_sub_text = NULL;

    /* External file takes priority over embedded stream */
    const char *sub_path = opt->sub_path;
    char auto_srt[512] = "";

    if (!sub_path && find_srt_alongside(filename, auto_srt, sizeof(auto_srt))) {
        sub_path = auto_srt;
        fprintf(stderr, "subtitle: auto-detected '%s'\n", sub_path);
    }

    if (sub_path) {
        /* External file — eager decode, no thread needed */
        if (subtitle_open_file(&p->sub, sub_path, 0) == 0)
            p->sub_active = 1;
    } else if (!p->no_audio && demux_has_subtitles(&p->demux)) {
        /* Embedded stream — route through queue + decode thread */
        AVStream *sub_stream =
            p->demux.fmt_ctx->streams[p->demux.subtitle_stream_idx];
        queue_init(&p->sub_queue);
        p->demux.subtitle_queue = &p->sub_queue;
        if (subtitle_open(&p->sub, sub_stream, &p->sub_queue) == 0) {
            p->sub_active   = 1;
            p->sub_embedded = 1;
        } else {
            queue_destroy(&p->sub_queue);
            p->demux.subtitle_queue = NULL;
        }
    }

/* ------------------------------------------------------------------ */
/* 7. In player_threads_start(), start subtitle thread if embedded     */
/* ------------------------------------------------------------------ */

/*
 * Add after the audio thread launch:
 */
    if (p->sub_active && p->sub_embedded) {
        ThreadArg *sa = malloc(sizeof(*sa)); sa->p = p;
        pthread_create(&p->stid, NULL, subtitle_thread, sa);
    }

/* ------------------------------------------------------------------ */
/* 8. In player_threads_stop(), join subtitle thread if running        */
/* ------------------------------------------------------------------ */

/*
 * Add after the audio thread join:
 */
    if (p->sub_active && p->sub_embedded)
        pthread_join(p->stid, NULL);

/* ------------------------------------------------------------------ */
/* 9. In player_close_pipeline(), close subtitle context               */
/* ------------------------------------------------------------------ */

/*
 * Add after audio_close():
 */
    if (p->sub_active) {
        subtitle_close(&p->sub);
        if (p->sub_embedded) {
            queue_destroy(&p->sub_queue);
            p->demux.subtitle_queue = NULL;
        }
        p->sub_active   = 0;
        p->sub_embedded = 0;
    }

/* ------------------------------------------------------------------ */
/* 10. In player_seek(), flush subtitles                               */
/* ------------------------------------------------------------------ */

/*
 * Add after audio_flush():
 */
    if (p->sub_active)
        subtitle_flush(&p->sub);

/* ------------------------------------------------------------------ */
/* 11. In the main frame presentation loop (standalone mode),          */
/*     log active subtitle to stderr when it changes.                  */
/*                                                                      */
/* Add this block just after "p->current_pts = frame->pts_us;" in     */
/* the video presentation section:                                     */
/* ------------------------------------------------------------------ */

            if (p->sub_active) {
                const char *sub = subtitle_get_active(&p->sub,
                                                       p->current_pts);
                if (sub != p->last_sub_text) {
                    if (sub)
                        fprintf(stderr, "subtitle: [%.2fs] %s\n",
                                p->current_pts / 1e6, sub);
                    p->last_sub_text = sub;
                }
            }

/*
 * Do the same in run_ws_mode() at the same point in the frame loop.
 */

/* ================================================================== */
/* MAKEFILE CHANGES                                                     */
/* ================================================================== */

/*
 * Add after the existing CFLAGS/LIBS lines, before the WS block:
 */

# Subtitles via FreeType2 (auto-detected — enabled if libfreetype2 is installed)
FREETYPE_CFLAGS := $(shell pkg-config --cflags freetype2 2>/dev/null)
FREETYPE_LIBS   := $(shell pkg-config --libs   freetype2 2>/dev/null)
ifneq ($(FREETYPE_CFLAGS),)
    CFLAGS += -DHAVE_FREETYPE $(FREETYPE_CFLAGS)
    LIBS   += $(FREETYPE_LIBS)
    $(info subtitle: FreeType2 found — subtitle rendering enabled)
else
    $(info subtitle: FreeType2 not found — install libfreetype6-dev to enable rendering)
endif

/*
 * Add subtitle.c to the SRCS list:
 */

SRCS = ... \
       $(SRCDIR)/subtitle.c \
       ...
