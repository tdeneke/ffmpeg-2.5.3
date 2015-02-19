/*
 * liborcc265 decoder
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
#define ORCC265_API_IMPORTS 1
#endif

#include <orcc265.h>

#include "libavutil/internal.h"
#include "libavutil/common.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "avcodec.h"
#include "internal.h"

static av_cold int liborcc265_decode_init(AVCodecContext *avctx)
{
    liborcc265Context *h = avctx->priv_data;
    orcc265_decoder_init(h);
    return 0;
}

static int liborcc265_decode_frame(AVCodecContext *avctx, void *data, int *got_frame, AVPacket *avpkt)
{
    liborcc265Context *h = avctx->priv_data;
    liborcc265Picture *pic = NULL;
    liborcc265Packet *pkt = NULL;
    orcc265_decoder_decode(h, pic, got_frame,pkt);
    return 0;
}

static av_cold int liborcc265_decode_end(AVCodecContext *avctx)
{
    liborcc265Context *h = avctx->priv_data;
    orcc265_decoder_end(h);
    return 0;
}

AVCodec ff_liborcc265_decoder = {
    .name                  = "liborcc265",
    .long_name             = NULL_IF_CONFIG_SMALL("ORCC / H.265 / HEVC"),
    .type                  = AVMEDIA_TYPE_VIDEO,
    .id                    = AV_CODEC_ID_ORCC265,
    .priv_data_size        = sizeof(liborcc265Context),
    .init                  = liborcc265_decode_init,
    .close                 = liborcc265_decode_end,
    .decode                = liborcc265_decode_frame,
    /*.capabilities          = CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 |
                             CODEC_CAP_DELAY | CODEC_CAP_SLICE_THREADS |
                             CODEC_CAP_FRAME_THREADS,
    .flush                 = flush_dpb,
    .init_thread_copy      = ONLY_IF_THREADS_ENABLED(decode_init_thread_copy),
    .update_thread_context = ONLY_IF_THREADS_ENABLED(ff_h265_update_thread_context),
    .profiles              = NULL_IF_CONFIG_SMALL(profiles),
    .priv_class            = &h264_class,*/
};
