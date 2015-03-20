/*
 * Copyright (c) 2010 Nicolas George
 * Copyright (c) 2011 Stefano Sabatini
 * Copyright (c) 2014 Andrey Utkin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * API example for demuxing, decoding, scaling, encoding and muxing
 * @example transcoding.c
 */

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libswscale/swscale.h>


static AVFormatContext *ifmt_ctx;
static AVFormatContext *ofmt_ctx;

static int open_input_file(const char *filename, char* ncores, char* codec_name)
{
    int ret;
    unsigned int i;

    ifmt_ctx = NULL;
    if ((ret = avformat_open_input(&ifmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *stream;
        AVCodecContext *codec_ctx;
        stream = ifmt_ctx->streams[i];
        codec_ctx = stream->codec;
        /* Reencode video & audio and remux subtitles etc. */
	//AVCodec *icodec = avcodec_find_decoder_by_name(codec_name);

        if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
                || codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {

            /* Open decoder */
	    if(strcmp(codec_name,"h264") == 0){
		codec_ctx->codec_id = AV_CODEC_ID_H264;
	    }else if(strcmp(codec_name,"h265") == 0){
                codec_ctx->codec_id = AV_CODEC_ID_HEVC;
            }else if(strcmp(codec_name,"orcc264") == 0){
		codec_ctx->codec_id = AV_CODEC_ID_ORCC264;
	    }else if(strcmp(codec_name,"orcc265") == 0){
		codec_ctx->codec_id = AV_CODEC_ID_ORCC265;
	    }else {
		printf("Codec name is: %s \n", codec_name);
		av_log(NULL, AV_LOG_ERROR, "Failed choose codec between h264, orcc264 and orcc265 \n");
                return ret;
	    }

	    codec_ctx->dataflow_ncores = ncores;
            codec_ctx->dataflow_input_fname = filename;
            ret = avcodec_open2(codec_ctx,
                   avcodec_find_decoder(codec_ctx->codec_id), NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
                return ret;
            }
        }
    }

    av_dump_format(ifmt_ctx, 0, filename, 0);
    return 0;
}

static int open_output_file(const char *filename, int width, int height, int bitrate)
{
    AVStream *out_stream;
    AVStream *in_stream;
    AVCodecContext *dec_ctx, *enc_ctx;
    AVCodec *encoder;
    AVDictionary *opts;
    int ret;
    unsigned int i;

    ofmt_ctx = NULL;
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, filename);
    if (!ofmt_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        return AVERROR_UNKNOWN;
    }


    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream) {
            av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
            return AVERROR_UNKNOWN;
        }

        in_stream = ifmt_ctx->streams[i];
        dec_ctx = in_stream->codec;
        enc_ctx = out_stream->codec;

        if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
                || dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            /* in this example, we choose transcoding to same codec */
            //encoder = avcodec_find_encoder(dec_ctx->codec_id);
            encoder = avcodec_find_encoder(AV_CODEC_ID_H264);

	    if (!encoder)
     		exit(1);
 	    //enc_ctx = avcodec_alloc_context3(encoder);

            /* In this example, we transcode to same properties (picture size,
             * sample rate etc.). These properties can be changed for output
             * streams easily using filters */
            if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
                // libx264-medium.ffpreset preset
		enc_ctx->coder_type = 1;  // coder = 1
		enc_ctx->flags|=CODEC_FLAG_LOOP_FILTER;   // flags=+loop (-2147483648)
		enc_ctx->me_cmp|= -1;  // cmp=+chroma, where CHROMA = 1
		//enc_ctx->partitions|=X264_PART_I8X8+X264_PART_I4X4+X264_PART_P8X8+X264_PART_B8X8; // partitions=+parti8x8+parti4x4+partp8x8+partb8x8
		enc_ctx->me_method=ME_HEX;    // me_method=hex (-1)
		enc_ctx->me_subpel_quality = 7;   // subq=7 (-1)
		enc_ctx->me_range = 16;   // me_range=16 (-1)
		enc_ctx->gop_size = 250;  // g=250 (-1)
		enc_ctx->keyint_min = 25; // keyint_min=25 (-1)
		enc_ctx->scenechange_threshold = 40;  // sc_threshold=40
		enc_ctx->i_quant_factor = 0.71; // i_qfactor=0.71 (6)
		enc_ctx->b_frame_strategy = 1;  // b_strategy=1 (-1)
		enc_ctx->qcompress = 0.6; // qcomp=0.6 (-1)
		enc_ctx->qmin = 10;   // qmin=10 (-1)
		enc_ctx->qmax = 51;   // qmax=51  (-1)
		enc_ctx->max_qdiff = 4;   // qdiff=4 (-1)
		enc_ctx->max_b_frames = 3;    // bf=3 (0)
		enc_ctx->refs = 3;    // refs=3 (-1)
		//enc_ctx->directpred = 1;  // directpred=1
		enc_ctx->trellis = 1; // trellis=1 (-1)
		//enc_ctx->flags2|=CODEC_FLAG2_BPYRAMID+CODEC_FLAG2_MIXED_REFS+CODEC_FLAG2_WPRED+CODEC_FLAG2_8X8DCT+CODEC_FLAG2_FASTPSKIP;  // flags2=+bpyramid+mixed_refs+wpred+dct8x8+fastpskip
		//enc_ctx->weighted_p_pred = 2; // wpredp=2

		//libx264-main.ffpreset preset
		//enc_ctx->flags2|=CODEC_FLAG2_8X8DCT;c->flags2^=CODEC_FLAG2_8X8DCT;    // flags2=-dct8x8

                enc_ctx->height = height;
                enc_ctx->width = width;
		enc_ctx->bit_rate = bitrate;
                enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
                /* take first format from list of supported formats */
                enc_ctx->pix_fmt = encoder->pix_fmts[0];
                /* video time_base can be set to whatever is handy and supported by encoder */
                enc_ctx->time_base = dec_ctx->time_base;
            } else {
                enc_ctx->sample_rate = dec_ctx->sample_rate;
                enc_ctx->channel_layout = dec_ctx->channel_layout;
                enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
                /* take first format from list of supported formats */
                enc_ctx->sample_fmt = encoder->sample_fmts[0];
                enc_ctx->time_base = (AVRational){1, enc_ctx->sample_rate};
            }

            /* Third parameter can be used to pass settings to encoder */
            //av_dict_set(&opts, "vprofile", "baseline", 0);
            //av_dict_set(&opts, "preset","ultrafast",0);
            //av_dict_set(&opts, "preset","medium",0);
            av_dict_set(&opts, "b", "2.5M", 0);
            ret = avcodec_open2(enc_ctx, encoder, &opts);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder for stream #%u\n", i);
                return ret;
            }
        } else if (dec_ctx->codec_type == AVMEDIA_TYPE_UNKNOWN) {
            av_log(NULL, AV_LOG_FATAL, "Elementary stream #%d is of unknown type, cannot proceed\n", i);
            return AVERROR_INVALIDDATA;
        } else {
            /* if this stream must be remuxed */
            ret = avcodec_copy_context(ofmt_ctx->streams[i]->codec,
                    ifmt_ctx->streams[i]->codec);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Copying stream context failed\n");
                return ret;
            }
        }

        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            enc_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;

    }
    av_dump_format(ofmt_ctx, 0, filename, 1);

    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", filename);
            return ret;
        }
    }

    /* init muxer, write output file header */
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
        return ret;
    }

    return 0;
}

static int encode_write_frame(AVFrame *scaled_frame, unsigned int stream_index, int *got_frame) {
    int ret;
    int got_frame_local;
    AVPacket enc_pkt;
    int (*enc_func)(AVCodecContext *, AVPacket *, const AVFrame *, int *) =
        (ifmt_ctx->streams[stream_index]->codec->codec_type ==
         AVMEDIA_TYPE_VIDEO) ? avcodec_encode_video2 : avcodec_encode_audio2;

    if (!got_frame)
        got_frame = &got_frame_local;

    av_log(NULL, AV_LOG_INFO, "Encoding frame\n");
    /* encode scaled frame */
    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);
    ret = enc_func(ofmt_ctx->streams[stream_index]->codec, &enc_pkt,
            scaled_frame, got_frame);
    if (ret < 0)
        return ret;
    if (!(*got_frame))
        return 0;

    /* prepare packet for muxing */
    enc_pkt.stream_index = stream_index;
    enc_pkt.dts = av_rescale_q_rnd(enc_pkt.dts,
            ofmt_ctx->streams[stream_index]->codec->time_base,
            ofmt_ctx->streams[stream_index]->time_base,
            AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    enc_pkt.pts = av_rescale_q_rnd(enc_pkt.pts,
            ofmt_ctx->streams[stream_index]->codec->time_base,
            ofmt_ctx->streams[stream_index]->time_base,
            AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    enc_pkt.duration = av_rescale_q(enc_pkt.duration,
            ofmt_ctx->streams[stream_index]->codec->time_base,
            ofmt_ctx->streams[stream_index]->time_base);

    av_log(NULL, AV_LOG_INFO, "Muxing frame\n");
    /* mux encoded frame */
    ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
    return ret;
}

static int flush_encoder(unsigned int stream_index)
{
    int ret;
    int got_frame;

    if (!(ofmt_ctx->streams[stream_index]->codec->codec->capabilities &
                CODEC_CAP_DELAY))
        return 0;

    while (1) {
        av_log(NULL, AV_LOG_INFO, "Flushing stream #%u encoder\n", stream_index);
        ret = encode_write_frame(NULL, stream_index, &got_frame);
        if (ret < 0)
            break;
        if (!got_frame)
            return 0;
    }
    return ret;
}

int main(int argc, char **argv)
{
    int ret,decoded;
    AVPacket packet = { .data = NULL, .size = 0 };
    AVFrame *frame = NULL;
    AVFrame *scaled_frame = NULL; 
    enum AVMediaType type;
    unsigned int stream_index;
    unsigned int i;
    int got_frame;
    char* ncores;
    char* codec_name;
    int width, height, bitrate;
    int (*dec_func)(AVCodecContext *, AVFrame *, int *, const AVPacket *);

    if (argc != 8) {
        av_log(NULL, AV_LOG_ERROR, "Usage: %s <input file> <output file> <width> <height> <bitrate> <ncores> <decoder name>\n", argv[0]);
        return 1;
    }

    width = atoi(argv[3]);
    height= atoi(argv[4]);
    bitrate = atoi(argv[5]);
    ncores = argv[6];
    codec_name = argv[7];

    av_register_all();
    avfilter_register_all();
    avcodec_register_all();

    if ((ret = open_input_file(argv[1],ncores, codec_name)) < 0)
        goto end;
    if ((ret = open_output_file(argv[2], width, height, bitrate)) < 0)
        goto end;

    scaled_frame = avcodec_alloc_frame();
    int num_bytes = avpicture_get_size(AV_PIX_FMT_YUV420P, width, height);
    uint8_t* scaled_frame_buffer = (uint8_t *)av_malloc(num_bytes*sizeof(uint8_t));
    avpicture_fill((AVPicture*)scaled_frame, scaled_frame_buffer,AV_PIX_FMT_YUV420P, width, height);
 
    /* read all packets */
    while (1) {
        if ((ret = av_read_frame(ifmt_ctx, &packet)) < 0)
            break;
        stream_index = packet.stream_index;
        type = ifmt_ctx->streams[packet.stream_index]->codec->codec_type;
        av_log(NULL, AV_LOG_DEBUG, "Demuxer gave frame of stream_index %u\n",
                stream_index);

            av_log(NULL, AV_LOG_DEBUG, "Going to reencode&filter the frame\n");

	    frame = av_frame_alloc();
            if (!frame) {
                ret = AVERROR(ENOMEM);
                break;
            }
            packet.dts = av_rescale_q_rnd(packet.dts,
                   ifmt_ctx->streams[stream_index]->time_base,
                   ifmt_ctx->streams[stream_index]->codec->time_base,
                   AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
            packet.pts = av_rescale_q_rnd(packet.pts,
                   ifmt_ctx->streams[stream_index]->time_base,
                   ifmt_ctx->streams[stream_index]->codec->time_base,
                   AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);


            do{
	    	dec_func = (type == AVMEDIA_TYPE_VIDEO) ? avcodec_decode_video2 :
                	avcodec_decode_audio4;

            	ret = dec_func(ifmt_ctx->streams[stream_index]->codec, frame,
                    	&got_frame, &packet);
                decoded = ret;
            	if (ret < 0) {
                	av_log(NULL, AV_LOG_ERROR, "Decoding failed\n");
                	break;
            	}

            	if (got_frame) {
                	frame->pts = av_frame_get_best_effort_timestamp(frame);
			ff_scale_image (scaled_frame->data, scaled_frame->linesize,width, height, AV_PIX_FMT_YUV420P, frame->data, frame->linesize, ifmt_ctx->streams[stream_index]->codec->width, ifmt_ctx->streams[stream_index]->codec->height, ifmt_ctx->streams[stream_index]->codec->pix_fmt, NULL);
                        ret = encode_write_frame(scaled_frame, stream_index, NULL); 

                	if (ret < 0)
                   		goto end; 
            	} 
		packet.data += decoded;
	   	packet.size -= decoded;
                //printf("*** pkt.size=%d  and ret=%d ***\n", packet.size, ret);
	   }while(packet.size > 0);

	   av_frame_free(&frame);        
        av_free_packet(&packet);
    }

    /* flush filters and encoders */
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        /* flush encoder */
        ret = flush_encoder(i);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Flushing encoder failed\n");
            goto end;
        }
    }

    av_write_trailer(ofmt_ctx);
    av_frame_free(&scaled_frame);	
end:
    av_free_packet(&packet);
    av_frame_free(&frame);
    av_frame_free(&scaled_frame);
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        avcodec_close(ifmt_ctx->streams[i]->codec);
        if (ofmt_ctx && ofmt_ctx->nb_streams > i && ofmt_ctx->streams[i] && ofmt_ctx->streams[i]->codec)
            avcodec_close(ofmt_ctx->streams[i]->codec);
    }
    avformat_close_input(&ifmt_ctx);
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);

    if (ret < 0)
        av_log(NULL, AV_LOG_ERROR, "Error occurred: %s\n", av_err2str(ret));

    return ret ? 1 : 0;
}
