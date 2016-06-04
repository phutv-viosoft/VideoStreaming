#ifndef _MCCODEC_H_
#define _MCCODEC_H_
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libavcodec/avcodec.h"
// #include "libavcodec/sw"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libavutil/opt.h"
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "libavutil/imgutils.h"

#define INBUF_SIZE 		4096
#define AUDIO_INBUF_SIZE 	20480
#define AUDIO_REFILL_THRESH 	4096

//Dinh nghia bien toan cuc
AVCodecContext *en_codec;
int out_size;
int output_size;
uint8_t *output_buffer;
AVFrame *picture, *picture_buf;
AVPacket avpkt;
AVFrame *de_picture,*pFrame;
int got_picture,de_len;

AVFrame avFrameRGB;
struct SwsContext *swsContext;
//Dinh nghia ham
void write_data(FILE *f, AVPacket *pkt, char end);
AVCodecContext *encodec_create_new(enum AVCodecID id, unsigned int width, unsigned int hight, unsigned char fps);
AVCodecContext *decodec_create_new(enum AVCodecID id, unsigned int width, unsigned int hight, unsigned char fps);
AVFrame *avframe_create_new(AVCodecContext *c, uint8_t *frame_buf);
void decode_init();

#endif