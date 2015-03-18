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

int sent = 0;
int to_send = 0;
orcc264_param_t *cmd;
AVPacket *tmp_avpkt; 

////////////////////////////////////////////////////////////////////////////////
// Instance
extern actor_t AVCSource;

////////////////////////////////////////////////////////////////////////////////
// Output FIFOs
extern fifo_u8_t *AVCSource_O;                                                  

////////////////////////////////////////////////////////////////////////////////
// Output Fifo control variables                                                
static unsigned int index_O;
static unsigned int numFree_O;                                                  
#define NUM_READERS_O 1                                                         
#define SIZE_O SIZE
#define tokens_O AVCSource_O->contents                                          

////////////////////////////////////////////////////////////////////////////////
// Successors
extern actor_t AVCDecoder_Syn_Parser_Algo_Synp;

////////////////////////////////////////////////////////////////////////////////
// Token functions

static void write_O() {
        index_O = AVCSource_O->write_ind;
        numFree_O = index_O + fifo_u8_get_room(AVCSource_O, NUM_READERS_O);
}

static void write_end_O() {
        AVCSource_O->write_ind = index_O;
}

////////////////////////////////////////////////////////////////////////////////
// Functions/procedures
extern void source_init();

////////////////////////////////////////////////////////////////////////////////
// Actions
static i32 isSchedulable_s() {
	i32 result;
	result = sent < to_send;
	return result;
}

static void s() {

	// Compute aligned port indexes
	i32 index_aligned_O = index_O % SIZE_O;
	tokens_O[(index_O + (0)) % SIZE_O] = tmp_avpkt->data[sent];
	sent += 1;
	// Update ports indexes
	//printf("sent token %d of %d\n", index_O, to_send);
	index_O += 1;

}

////////////////////////////////////////////////////////////////////////////////
// Initializes

static void AVCSource_initialize(schedinfo_t *si) {
	int i = 0;
	write_O();
	source_init();
finished:
	write_end_O();
	return;
}

////////////////////////////////////////////////////////////////////////////////
// Action scheduler
static void AVCSource_scheduler(schedinfo_t *si) {
	int i = 0;
	si->ports = 0;

	write_O();

	while (1) {
		if (isSchedulable_s()) {
			int stop = 0;
			if (1 > SIZE_O - index_O + AVCSource_O->read_inds[0]) {
				stop = 1;
			}
			if (stop != 0) {
				si->num_firings = i;
				si->reason = full;
				goto finished;
			}
			s();
			i++;
		} else {
			si->num_firings = i;
			si->reason = starved;
			//cnt = 0;
			goto finished;
		}
	}

finished:

	write_end_O();
}


////////////////////////////////////////////////////////////////////////////////
// Instance
extern actor_t AVCDisplay;

////////////////////////////////////////////////////////////////////////////////
// Input FIFOs
extern fifo_u8_t *AVCDisplay_B;
extern fifo_i16_t *AVCDisplay_WIDTH;
extern fifo_i16_t *AVCDisplay_HEIGHT;

////////////////////////////////////////////////////////////////////////////////
// Input Fifo control variables
static unsigned int index_B;
static unsigned int numTokens_B;
#define SIZE_B SIZE
#define tokens_B AVCDisplay_B->contents

extern connection_t connection_AVCDisplay_B;
#define rate_B connection_AVCDisplay_B.rate

static unsigned int index_WIDTH;
static unsigned int numTokens_WIDTH;
#define SIZE_WIDTH SIZE
#define tokens_WIDTH AVCDisplay_WIDTH->contents

extern connection_t connection_AVCDisplay_WIDTH;
#define rate_WIDTH connection_AVCDisplay_WIDTH.rate

static unsigned int index_HEIGHT;
static unsigned int numTokens_HEIGHT;
#define SIZE_HEIGHT SIZE
#define tokens_HEIGHT AVCDisplay_HEIGHT->contents

extern connection_t connection_AVCDisplay_HEIGHT;
#define rate_HEIGHT connection_AVCDisplay_HEIGHT.rate

////////////////////////////////////////////////////////////////////////////////
// Predecessors
extern actor_t AVCMerger;
extern actor_t AVCDecoder_Syn_Parser_Algo_Synp;
extern actor_t AVCDecoder_Syn_Parser_Algo_Synp;


////////////////////////////////////////////////////////////////////////////////
// State variables of the actor
#define Display_MB_SIZE_LUMA 16
#define Display_MB_SIZE_CHROMA 8
#define Display_DISP_ENABLE 1
static u8 pictureBufferY[8388608];
static u8 pictureBufferU[2097152];
static u8 pictureBufferV[2097152];
static i32 pictureSizeInMb;
static i32 nbBlockGot;
static i32 nbFrameDecoded;
static u8 chromaComponent;
static i16 pictureWidthLuma = 4096;
static i16 pictureHeightLuma = 2048;
static u16 xIdxLuma;
static u32 yOffLuma;
static u16 xIdxChroma;
static u32 yOffChroma;
static i32 isTerminated = 0;

////////////////////////////////////////////////////////////////////////////////
// Initial FSM state of the actor
enum states {
	my_state_GetChroma1Block,
	my_state_GetChroma2Block,
	my_state_GetLumaBlock,
	my_state_GetPictureSize
};

static char *stateNames[] = {
	"GetChroma1Block",
	"GetChroma2Block",
	"GetLumaBlock",
	"GetPictureSize"
};

static enum states _FSM_state;



////////////////////////////////////////////////////////////////////////////////
// Token functions
static void read_B() {
	index_B = AVCDisplay_B->read_inds[0];
	numTokens_B = index_B + fifo_u8_get_num_tokens(AVCDisplay_B, 0);
}

static void read_end_B() {
	AVCDisplay_B->read_inds[0] = index_B;
}
static void read_WIDTH() {
	index_WIDTH = AVCDisplay_WIDTH->read_inds[0];
	numTokens_WIDTH = index_WIDTH + fifo_i16_get_num_tokens(AVCDisplay_WIDTH, 0);
}

static void read_end_WIDTH() {
	AVCDisplay_WIDTH->read_inds[0] = index_WIDTH;
}
static void read_HEIGHT() {
	index_HEIGHT = AVCDisplay_HEIGHT->read_inds[0];
	numTokens_HEIGHT = index_HEIGHT + fifo_i16_get_num_tokens(AVCDisplay_HEIGHT, 0);
}

static void read_end_HEIGHT() {
	AVCDisplay_HEIGHT->read_inds[0] = index_HEIGHT;
}


////////////////////////////////////////////////////////////////////////////////
// Functions/procedures
extern i32 source_isMaxLoopsReached();
extern i32 displayYUV_getNbFrames();
extern void source_exit(i32 exitCode);
extern u8 displayYUV_getFlags();
extern void fpsPrintNewPicDecoded();
extern void displayYUV_displayPicture(u8 pictureBufferY[8388608], u8 pictureBufferU[8388608], u8 pictureBufferV[8388608], i16 pictureWidth, i16 pictureSize);
extern void compareYUV_comparePicture(u8 pictureBufferY[8388608], u8 pictureBufferU[8388608], u8 pictureBufferV[8388608], i16 pictureWidth, i16 pictureSize);
extern void displayYUV_init();
extern void compareYUV_init();
extern void fpsPrintInit();


////////////////////////////////////////////////////////////////////////////////
// Actions
static i32 isSchedulable_untagged_0() {
	i32 result;
	i32 local_isTerminated;
	i32 tmp_source_isMaxLoopsReached;
	i32 local_nbFrameDecoded;
	i32 tmp_displayYUV_getNbFrames;

	local_isTerminated = isTerminated;
	tmp_source_isMaxLoopsReached = source_isMaxLoopsReached();
	local_nbFrameDecoded = nbFrameDecoded;
	tmp_displayYUV_getNbFrames = displayYUV_getNbFrames();
	result = !local_isTerminated && (tmp_source_isMaxLoopsReached || local_nbFrameDecoded == tmp_displayYUV_getNbFrames);
	return result;
}

static void untagged_0() {


	isTerminated = 1;
	source_exit(0);

	// Update ports indexes

}
static i32 isSchedulable_getPictureSize() {
	i32 result;

	result = 1;
	return result;
}

static void getPictureSize() {

	i16 WidthValue;
	i16 HeightValue;

	WidthValue = tokens_WIDTH[(index_WIDTH + (0)) % SIZE_WIDTH];
	HeightValue = tokens_HEIGHT[(index_HEIGHT + (0)) % SIZE_HEIGHT];
	pictureWidthLuma = WidthValue * 16;
	pictureHeightLuma = HeightValue * 16;
	pictureSizeInMb = WidthValue * HeightValue;
	nbBlockGot = 0;
	xIdxLuma = 0;
	xIdxChroma = 0;
	yOffLuma = 0;
	yOffChroma = 0;

	// Update ports indexes
	index_WIDTH += 1;
	index_HEIGHT += 1;

	rate_WIDTH += 1;
	rate_HEIGHT += 1;
}
static i32 isSchedulable_getPixValue_launch_Luma() {
	i32 result;
	i32 local_nbBlockGot;
	i32 local_pictureSizeInMb;

	local_nbBlockGot = nbBlockGot;
	local_pictureSizeInMb = pictureSizeInMb;
	result = local_nbBlockGot < local_pictureSizeInMb;
	return result;
}

static void getPixValue_launch_Luma() {

	u32 local_yOffLuma;
	u32 yOff;
	u16 idx;
	i32 local_nbBlockGot;
	i32 y;
	i32 x;
	u16 local_xIdxLuma;
	u8 tmp_B;
	i16 local_pictureWidthLuma;
	u32 local_MB_SIZE_LUMA;

	local_yOffLuma = yOffLuma;
	yOff = local_yOffLuma;
	idx = 0;
	local_nbBlockGot = nbBlockGot;
	nbBlockGot = local_nbBlockGot + 1;
	y = 0;
	while (y <= 15) {
		local_xIdxLuma = xIdxLuma;
		x = local_xIdxLuma;
		local_xIdxLuma = xIdxLuma;
		while (x <= local_xIdxLuma + 15) {
			tmp_B = tokens_B[(index_B + (idx)) % SIZE_B];
			pictureBufferY[yOff + x] = tmp_B;
			idx = idx + 1;
			x = x + 1;
		}
		local_pictureWidthLuma = pictureWidthLuma;
		yOff = yOff + local_pictureWidthLuma;
		y = y + 1;
	}
	local_xIdxLuma = xIdxLuma;
	local_MB_SIZE_LUMA = Display_MB_SIZE_LUMA;
	xIdxLuma = local_xIdxLuma + local_MB_SIZE_LUMA;
	chromaComponent = 0;

	// Update ports indexes
	index_B += 256;
	read_end_B();

	rate_B += 256;
}
static void getPixValue_launch_Luma_aligned() {

	u32 local_yOffLuma;
	u32 yOff;
	u16 idx;
	i32 local_nbBlockGot;
	i32 y;
	i32 x;
	u16 local_xIdxLuma;
	u8 tmp_B;
	i16 local_pictureWidthLuma;
	u32 local_MB_SIZE_LUMA;

	local_yOffLuma = yOffLuma;
	yOff = local_yOffLuma;
	idx = 0;
	local_nbBlockGot = nbBlockGot;
	nbBlockGot = local_nbBlockGot + 1;
	y = 0;
	while (y <= 15) {
		local_xIdxLuma = xIdxLuma;
		x = local_xIdxLuma;
		local_xIdxLuma = xIdxLuma;
		while (x <= local_xIdxLuma + 15) {
			tmp_B = tokens_B[(index_B % SIZE_B) + (idx)];
			pictureBufferY[yOff + x] = tmp_B;
			idx = idx + 1;
			x = x + 1;
		}
		local_pictureWidthLuma = pictureWidthLuma;
		yOff = yOff + local_pictureWidthLuma;
		y = y + 1;
	}
	local_xIdxLuma = xIdxLuma;
	local_MB_SIZE_LUMA = Display_MB_SIZE_LUMA;
	xIdxLuma = local_xIdxLuma + local_MB_SIZE_LUMA;
	chromaComponent = 0;

	// Update ports indexes
	index_B += 256;
	read_end_B();

	rate_B += 256;
}
static i32 isSchedulable_getPixValue_launch_Chroma() {
	i32 result;

	result = 1;
	return result;
}

static void getPixValue_launch_Chroma() {

	u32 local_yOffChroma;
	u32 yOff;
	u8 idx;
	i32 y;
	i32 x;
	u16 local_xIdxChroma;
	u8 local_chromaComponent;
	u8 tmp_B;
	u8 tmp_B0;
	i16 local_pictureWidthLuma;
	u32 local_MB_SIZE_CHROMA;
	u16 local_xIdxLuma;
	u32 local_yOffLuma;
	u32 local_MB_SIZE_LUMA;

	local_yOffChroma = yOffChroma;
	yOff = local_yOffChroma;
	idx = 0;
	y = 0;
	while (y <= 7) {
		local_xIdxChroma = xIdxChroma;
		x = local_xIdxChroma;
		local_xIdxChroma = xIdxChroma;
		while (x <= local_xIdxChroma + 7) {
			local_chromaComponent = chromaComponent;
			if (local_chromaComponent == 0) {
				tmp_B = tokens_B[(index_B + (idx)) % SIZE_B];
				pictureBufferU[yOff + x] = tmp_B;
			} else {
				tmp_B0 = tokens_B[(index_B + (idx)) % SIZE_B];
				pictureBufferV[yOff + x] = tmp_B0;
			}
			idx = idx + 1;
			x = x + 1;
		}
		local_pictureWidthLuma = pictureWidthLuma;
		yOff = yOff + local_pictureWidthLuma / 2;
		y = y + 1;
	}
	local_chromaComponent = chromaComponent;
	if (local_chromaComponent != 0) {
		local_xIdxChroma = xIdxChroma;
		local_MB_SIZE_CHROMA = Display_MB_SIZE_CHROMA;
		xIdxChroma = local_xIdxChroma + local_MB_SIZE_CHROMA;
		local_xIdxLuma = xIdxLuma;
		local_pictureWidthLuma = pictureWidthLuma;
		if (local_xIdxLuma == local_pictureWidthLuma) {
			xIdxLuma = 0;
			xIdxChroma = 0;
			local_yOffLuma = yOffLuma;
			local_MB_SIZE_LUMA = Display_MB_SIZE_LUMA;
			local_pictureWidthLuma = pictureWidthLuma;
			yOffLuma = local_yOffLuma + local_MB_SIZE_LUMA * local_pictureWidthLuma;
			local_yOffChroma = yOffChroma;
			local_MB_SIZE_CHROMA = Display_MB_SIZE_CHROMA;
			local_pictureWidthLuma = pictureWidthLuma;
			yOffChroma = local_yOffChroma + local_MB_SIZE_CHROMA * local_pictureWidthLuma / 2;
		}
	}
	chromaComponent = 1;

	// Update ports indexes
	index_B += 64;
	read_end_B();

	rate_B += 64;
}
static void getPixValue_launch_Chroma_aligned() {

	u32 local_yOffChroma;
	u32 yOff;
	u8 idx;
	i32 y;
	i32 x;
	u16 local_xIdxChroma;
	u8 local_chromaComponent;
	u8 tmp_B;
	u8 tmp_B0;
	i16 local_pictureWidthLuma;
	u32 local_MB_SIZE_CHROMA;
	u16 local_xIdxLuma;
	u32 local_yOffLuma;
	u32 local_MB_SIZE_LUMA;

	local_yOffChroma = yOffChroma;
	yOff = local_yOffChroma;
	idx = 0;
	y = 0;
	while (y <= 7) {
		local_xIdxChroma = xIdxChroma;
		x = local_xIdxChroma;
		local_xIdxChroma = xIdxChroma;
		while (x <= local_xIdxChroma + 7) {
			local_chromaComponent = chromaComponent;
			if (local_chromaComponent == 0) {
				tmp_B = tokens_B[(index_B % SIZE_B) + (idx)];
				pictureBufferU[yOff + x] = tmp_B;
			} else {
				tmp_B0 = tokens_B[(index_B % SIZE_B) + (idx)];
				pictureBufferV[yOff + x] = tmp_B0;
			}
			idx = idx + 1;
			x = x + 1;
		}
		local_pictureWidthLuma = pictureWidthLuma;
		yOff = yOff + local_pictureWidthLuma / 2;
		y = y + 1;
	}
	local_chromaComponent = chromaComponent;
	if (local_chromaComponent != 0) {
		local_xIdxChroma = xIdxChroma;
		local_MB_SIZE_CHROMA = Display_MB_SIZE_CHROMA;
		xIdxChroma = local_xIdxChroma + local_MB_SIZE_CHROMA;
		local_xIdxLuma = xIdxLuma;
		local_pictureWidthLuma = pictureWidthLuma;
		if (local_xIdxLuma == local_pictureWidthLuma) {
			xIdxLuma = 0;
			xIdxChroma = 0;
			local_yOffLuma = yOffLuma;
			local_MB_SIZE_LUMA = Display_MB_SIZE_LUMA;
			local_pictureWidthLuma = pictureWidthLuma;
			yOffLuma = local_yOffLuma + local_MB_SIZE_LUMA * local_pictureWidthLuma;
			local_yOffChroma = yOffChroma;
			local_MB_SIZE_CHROMA = Display_MB_SIZE_CHROMA;
			local_pictureWidthLuma = pictureWidthLuma;
			yOffChroma = local_yOffChroma + local_MB_SIZE_CHROMA * local_pictureWidthLuma / 2;
		}
	}
	chromaComponent = 1;

	// Update ports indexes
	index_B += 64;
	read_end_B();

	rate_B += 64;
}
static i32 isSchedulable_displayPicture() {
	i32 result;
	i32 local_nbBlockGot;
	i32 local_pictureSizeInMb;
	u8 tmp_displayYUV_getFlags;
	u8 local_DISP_ENABLE;

	local_nbBlockGot = nbBlockGot;
	local_pictureSizeInMb = pictureSizeInMb;
	tmp_displayYUV_getFlags = displayYUV_getFlags();
	local_DISP_ENABLE = Display_DISP_ENABLE;
	result = local_nbBlockGot >= local_pictureSizeInMb && (tmp_displayYUV_getFlags & local_DISP_ENABLE) != 0;
	return result;
}

static void displayPicture() {

	i16 local_pictureWidthLuma;
	i16 local_pictureHeightLuma;
	i32 local_nbFrameDecoded;

	fpsPrintNewPicDecoded();
	local_pictureWidthLuma = pictureWidthLuma;
	local_pictureHeightLuma = pictureHeightLuma;
	displayYUV_displayPicture(pictureBufferY, pictureBufferU, pictureBufferV, local_pictureWidthLuma, local_pictureHeightLuma);
	local_pictureWidthLuma = pictureWidthLuma;
	local_pictureHeightLuma = pictureHeightLuma;
	compareYUV_comparePicture(pictureBufferY, pictureBufferU, pictureBufferV, local_pictureWidthLuma, local_pictureHeightLuma);
	local_nbFrameDecoded = nbFrameDecoded;
	nbFrameDecoded = local_nbFrameDecoded + 1;

	// Update ports indexes

}
static i32 isSchedulable_displayDisable() {
	i32 result;
	i32 local_nbBlockGot;
	i32 local_pictureSizeInMb;

	local_nbBlockGot = nbBlockGot;
	local_pictureSizeInMb = pictureSizeInMb;
	result = local_nbBlockGot >= local_pictureSizeInMb;
	return result;
}

static void displayDisable() {

	i16 local_pictureWidthLuma;
	i16 local_pictureHeightLuma;
	i32 local_nbFrameDecoded;

	fpsPrintNewPicDecoded();
	local_pictureWidthLuma = pictureWidthLuma;
	local_pictureHeightLuma = pictureHeightLuma;
	compareYUV_comparePicture(pictureBufferY, pictureBufferU, pictureBufferV, local_pictureWidthLuma, local_pictureHeightLuma);
	local_nbFrameDecoded = nbFrameDecoded;
	nbFrameDecoded = local_nbFrameDecoded + 1;

	// Update ports indexes

}

////////////////////////////////////////////////////////////////////////////////
// Initializes
static i32 isSchedulable_untagged_1() {
	i32 result;

	result = 1;
	return result;
}

static void untagged_1() {

	u8 tmp_displayYUV_getFlags;
	u8 local_DISP_ENABLE;

	tmp_displayYUV_getFlags = displayYUV_getFlags();
	local_DISP_ENABLE = Display_DISP_ENABLE;
	if ((tmp_displayYUV_getFlags & local_DISP_ENABLE) != 0) {
		displayYUV_init();
	}
	compareYUV_init();
	fpsPrintInit();
	nbFrameDecoded = 0;

	// Update ports indexes

}

static void AVCDisplay_initialize(schedinfo_t *si) {
	int i = 0;
	/* Set initial state to current FSM state */
	_FSM_state = my_state_GetPictureSize;
	if(isSchedulable_untagged_1()) {
		untagged_1();
	}
finished:
	return;
}

////////////////////////////////////////////////////////////////////////////////
// Action scheduler
static void AVCDisplay_outside_FSM_scheduler(schedinfo_t *si) {
	int i = 0;
	while (1) {
		if (isSchedulable_untagged_0()) {
			untagged_0();
			i++;
		} else {
			si->num_firings = i;
			si->reason = starved;
			goto finished;
		}
	}
finished:
	// no read_end/write_end here!
	return;
}

static void AVCDisplay_scheduler(schedinfo_t *si) {
	int i = 0;

	read_B();
	read_WIDTH();
	read_HEIGHT();

	// jump to FSM state
	switch (_FSM_state) {
	case my_state_GetChroma1Block:
		goto l_GetChroma1Block;
	case my_state_GetChroma2Block:
		goto l_GetChroma2Block;
	case my_state_GetLumaBlock:
		goto l_GetLumaBlock;
	case my_state_GetPictureSize:
		goto l_GetPictureSize;
	default:
		printf("unknown state in AVCDisplay.c : %s\n", stateNames[_FSM_state]);
		exit(1);
	}

	// FSM transitions
l_GetChroma1Block:
	AVCDisplay_outside_FSM_scheduler(si);
	i += si->num_firings;
	if (numTokens_B - index_B >= 64 && isSchedulable_getPixValue_launch_Chroma()) {
		{
			int isAligned = 1;
			isAligned &= ((index_B % SIZE_B) < ((index_B + 64) % SIZE_B));
			if (isAligned) {
				getPixValue_launch_Chroma_aligned();
			} else {
				getPixValue_launch_Chroma();
			}
		}
		i++;
		goto l_GetChroma2Block;
	} else {
		si->num_firings = i;
		si->reason = starved;
		_FSM_state = my_state_GetChroma1Block;
		goto finished;
	}
l_GetChroma2Block:
	AVCDisplay_outside_FSM_scheduler(si);
	i += si->num_firings;
	if (numTokens_B - index_B >= 64 && isSchedulable_getPixValue_launch_Chroma()) {
		{
			int isAligned = 1;
			isAligned &= ((index_B % SIZE_B) < ((index_B + 64) % SIZE_B));
			if (isAligned) {
				getPixValue_launch_Chroma_aligned();
			} else {
				getPixValue_launch_Chroma();
			}
		}
		i++;
		goto l_GetLumaBlock;
	} else {
		si->num_firings = i;
		si->reason = starved;
		_FSM_state = my_state_GetChroma2Block;
		goto finished;
	}
l_GetLumaBlock:
	AVCDisplay_outside_FSM_scheduler(si);
	i += si->num_firings;
	if (numTokens_B - index_B >= 256 && isSchedulable_getPixValue_launch_Luma()) {
		{
			int isAligned = 1;
			isAligned &= ((index_B % SIZE_B) < ((index_B + 256) % SIZE_B));
			if (isAligned) {
				getPixValue_launch_Luma_aligned();
			} else {
				getPixValue_launch_Luma();
			}
		}
		i++;
		goto l_GetChroma1Block;
	} else if (isSchedulable_displayPicture()) {
		displayPicture();
		i++;
		goto l_GetPictureSize;
	} else if (isSchedulable_displayDisable()) {
		displayDisable();
		i++;
		goto l_GetPictureSize;
	} else {
		si->num_firings = i;
		si->reason = starved;
		_FSM_state = my_state_GetLumaBlock;
		goto finished;
	}
l_GetPictureSize:
	AVCDisplay_outside_FSM_scheduler(si);
	i += si->num_firings;
	if (numTokens_WIDTH - index_WIDTH >= 1 && numTokens_HEIGHT - index_HEIGHT >= 1 && isSchedulable_getPictureSize()) {
		getPictureSize();
		i++;
		goto l_GetLumaBlock;
	} else {
		si->num_firings = i;
		si->reason = starved;
		_FSM_state = my_state_GetPictureSize;
		goto finished;
	}
finished:
	read_end_B();
	read_end_WIDTH();
	read_end_HEIGHT();
}


static av_cold int liborcc264_decode_init(AVCodecContext *avctx)
{
    liborcc264Context *q = avctx->priv_data;
    //orcc264_decoder_init(h);
    q->frames_decoded = 0;
    cmd = (orcc264_param_t*) malloc(sizeof(orcc264_param_t));
    av_log(avctx, AV_LOG_ERROR, "decoder is still being tested!\n");

    //some arguments; defaults dataflow decoder. later we need a set function! 
    char* argv[8];
    argv[0] = "./fforcc";
    argv[1] = "-i";
    argv[2] = "place_holder_only.h264";  //input stream
    argv[3] = "-n";
    argv[4] = "-l";
    argv[5] = "1";
    argv[6] = "-c";
    argv[7] = "4"; //ncores
    int argc = 8;
    cmd->argv = argv;
    cmd->argc = argc;

    opt = set_default_options();
    orcc_thread_create(q->launch_thread, orcc264_start_actors, *cmd, q->launch_thread_id);

    //either we need to sleep till initilization or check by pooling.
    while(opt->display_flags==NULL);
    while(displayYUV_getFlags()!=0);


    q->sink_sched_info = (schedinfo_t*) malloc(sizeof(schedinfo_t));
    (q->sink_sched_info)->num_firings = 0;
    (q->sink_sched_info)->reason = 0;
    (q->sink_sched_info)->ports = 0;
    q->frames_decoded = 0;
    AVCDisplay_initialize(q->sink_sched_info);

    q->source_sched_info = (schedinfo_t*) malloc(sizeof(schedinfo_t));
    (q->source_sched_info)->num_firings = 0;
    (q->source_sched_info)->reason = 0;
    (q->source_sched_info)->ports = 0;
    AVCSource_initialize(q->source_sched_info);

    return 0;
}

static int liborcc264_decode_frame(AVCodecContext *avctx, void *data, int *got_frame, AVPacket *avpkt)
{
    liborcc264Context *q = avctx->priv_data;
    //liborcc264Picture *pic = NULL;
    //liborcc264Packet *pkt = NULL;
    //orcc264_decoder_decode(h, pic, got_frame, pkt);
    int buf_size       = avpkt->size;
    int buf_index      = 0;
    tmp_avpkt = avpkt;
    to_send = avpkt->size;
    sent = 0;
    AVCSource_scheduler(q->source_sched_info);
    //nbFrameDecoded
    q->frames_decoded = nbFrameDecoded;

    // printf("**** inside decode_frame ***** 2 \n");

    AVCDisplay_scheduler(q->sink_sched_info);
    //Do nothing :)

    //printf("**** inside decode_frame ***** 3 \n");
    //printf("**** inside decode_frame ***** 4 --- %d, %d\n", nbFrameDecoded,  q->frames_decoded);

    if(nbFrameDecoded > q->frames_decoded){
	 // printf("**** inside decode_frame ***** 4 --- %d, %d\n", nbFrameDecoded,  q->frames_decoded);
         // av_log(avctx, AV_LOG_ERROR, "Packet size is %d !\n", avpkt->size);
         // Initialize the AVFrame
         AVFrame* frame = data;
         frame->width = pictureWidthLuma;
         frame->height = pictureHeightLuma;
         frame->format = AV_PIX_FMT_YUV420P;

         // Initialize frame->linesize
         avpicture_fill((AVPicture*)frame, NULL, frame->format, frame->width, frame->height);

         // Set frame->data pointers manually
         frame->data[0] =  pictureBufferY;
         frame->data[1] =  pictureBufferU;
         frame->data[2] =  pictureBufferV;
         *got_frame = 1;

    }else{
    	*got_frame = 0;
    }

    avpkt->size -= sent;
    avpkt->data += sent;

    // printf("**** decoded frame %d, gotframe = %d and packet size = %d and sent = %d *****\n", q->frames_decoded++, *got_frame, to_send, sent);
    // Return the amount of bytes consumed if everything was ok
    return sent;
}

static av_cold int liborcc264_decode_end(AVCodecContext *avctx)
{
    liborcc264Context *q = avctx->priv_data;
    //orcc264_decoder_end(h);
    free(q->source_sched_info);
    free(q->sink_sched_info);
    free(cmd);
    untagged_0();
    orcc_thread_join(q->launch_thread);
    //fclose(ofile);
    //Return 0 if everything is ok, -1 if not
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
