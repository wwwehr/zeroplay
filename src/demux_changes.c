/* ================================================================== */
/* CHANGES TO src/demux.h                                               */
/* ================================================================== */

/* In the DemuxContext struct, add after audio_stream_idx: */

    int              subtitle_stream_idx;  /* -1 if none */
    Queue           *subtitle_queue;       /* set by caller if subtitles wanted */

/* Add to demux.h public API (after demux_next_chapter): */

/* Returns 1 if stream has an embedded subtitle track */
int demux_has_subtitles(DemuxContext *ctx);


/* ================================================================== */
/* CHANGES TO src/demux.c                                               */
/* ================================================================== */

/* 1. In demux_open(), initialise the new field near the top: */

    ctx->subtitle_stream_idx = -1;

/* 2. In the stream-selection loop (where video/audio are picked),
      add subtitle detection. After the existing audio selection: */

        if (par->codec_type == AVMEDIA_TYPE_SUBTITLE &&
            ctx->subtitle_stream_idx == -1)
            ctx->subtitle_stream_idx = (int)i;

/* 3. In the AVDISCARD_ALL loop at the end of demux_open(),
      also keep the subtitle stream (update the condition): */

        if ((int)i != ctx->video_stream_idx  &&
            (int)i != ctx->audio_stream_idx  &&
            (int)i != ctx->subtitle_stream_idx)
            ctx->fmt_ctx->streams[i]->discard = AVDISCARD_ALL;

/* 4. In demux_run(), in the packet routing block, add after the
      existing audio routing (the `else if` for audio packets): */

        } else if (pkt->stream_index == ctx->subtitle_stream_idx &&
                   ctx->subtitle_queue) {
            queue_push(ctx->subtitle_queue, pkt);
            pkt = NULL;   /* ownership transferred */
        }

/* 5. Add the new helper function anywhere in demux.c: */

int demux_has_subtitles(DemuxContext *ctx)
{
    return ctx->subtitle_stream_idx >= 0;
}

/* ================================================================== */
/* NOTES                                                                */
/* ================================================================== */

/*
 * ctx->subtitle_queue is set by the caller (main.c) before starting
 * the demux thread — same pattern as video_queue and audio_queue.
 * If subtitle_queue is NULL, subtitle packets are dropped (no-op).
 *
 * The subtitle stream index is printed in the existing demux debug
 * log so you can verify detection without --verbose.
 */
