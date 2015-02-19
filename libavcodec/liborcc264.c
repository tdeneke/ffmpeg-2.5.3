/*
 * liborcc264 decoder
 *
 * Copyright (c) 2013-2014 Tewodros Deneke
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#if defined(_MSC_VER)
#define ORCC264_API_IMPORTS 1
#endif

#include <orcc264.h>

#include "libavutil/internal.h"
#include "libavutil/common.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "avcodec.h"
#include "internal.h"

static av_cold int liborcc264_decode_init(AVCodecContext *avctx)
{
    liborcc264Context *h = avctx->priv_data;
    orcc264_decoder_init(h);
    return 0;
}

static int liborcc264_decode_frame(AVCodecContext *avctx, void *data, int *got_frame, AVPacket *avpkt)
{
    liborcc264Context *h = avctx->priv_data;
    liborcc264Picture *pic = NULL;
    liborcc264Packet *pkt = NULL;
    orcc264_decoder_decode(h, pic, got_frame, pkt);
    return 0;
}

static av_cold int liborcc264_decode_end(AVCodecContext *avctx)
{
    liborcc264Context *h = avctx->priv_data;
    orcc_decoder_end(h);
    return 0;
}

AVCodec ff_liborcc264_decoder = {
    .name                  = "liborcc264",
    .long_name             = NULL_IF_CONFIG_SMALL("ORCC / H.264"),
    .type                  = AVMEDIA_TYPE_VIDEO,
    .id                    = AV_CODEC_ID_ORCC264,
    .priv_data_size        = sizeof(liborcc264Context),
    .init                  = liborcc264_decode_init,
    .close                 = liborcc264_decode_end,
    .decode                = liborcc264_decode_frame,
    /*.capabilities          = CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 |
                             CODEC_CAP_DELAY | CODEC_CAP_SLICE_THREADS |
                             CODEC_CAP_FRAME_THREADS,
    .flush                 = flush_dpb,
    .init_thread_copy      = ONLY_IF_THREADS_ENABLED(decode_init_thread_copy),
    .update_thread_context = ONLY_IF_THREADS_ENABLED(ff_h264_update_thread_context),
    .profiles              = NULL_IF_CONFIG_SMALL(profiles),
    .priv_class            = &h264_class,*/
};
