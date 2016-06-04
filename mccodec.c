#ifndef _MCCODEC_C_
#define _MCCODEC_C_
#include "mccodec.h"
#include <stdlib.h>

AVFrame* avframe_create_new(AVCodecContext* c,uint8_t *frame_buf)
{
    AVFrame *frame;
    
    //frame = avcodec_alloc_frame();
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;
    frame->pts = 0;
    av_image_alloc(frame->data, frame->linesize, c->width, c->height,
                         c->pix_fmt, 32);
    return frame;
}

AVCodecContext* encodec_create_new(enum AVCodecID id, unsigned int width, unsigned int hight, unsigned char fps)
{
    AVCodecContext *c= NULL;
    AVCodec *codec;
    
    codec = avcodec_find_encoder(id);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }
    c = avcodec_alloc_context3(codec);
    
    /* put sample parameters */
    c->bit_rate = 800000;
    /* resolution must be a multiple of two */
    c->width = width;
    c->height = hight;
    /* frames per second */
    c->time_base= (AVRational){1,fps};
    c->gop_size = 20; /* emit one intra frame every ten frames */
    c->max_b_frames=2;
    c->pix_fmt = AV_PIX_FMT_YUV420P;
      
    if (id == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);
    
    /* open it */
    if (avcodec_open2(c, codec,NULL) < 0) {
        fprintf(stderr, "could not open codec\n");
        exit(1);
    }
    
    return c;
}

AVCodecContext* decodec_create_new(enum AVCodecID id, unsigned int width, unsigned int hight, unsigned char fps)
{
    AVCodecContext *c= NULL;
    AVCodec *codec;
    
    codec = avcodec_find_decoder(id);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }
    c = avcodec_alloc_context3(codec);
    
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = width;
    c->height = hight;
    /* frames per second */
    c->time_base= (AVRational){1,fps};
    c->gop_size = 10; /* emit one intra frame every ten frames */
    c->max_b_frames=1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;
    
    if(codec->capabilities&CODEC_CAP_TRUNCATED)
        c->flags|= CODEC_FLAG_TRUNCATED; /* we do not send complete frames */
        
    if (id == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);
    
    /* open it */
    if (avcodec_open2(c, codec,NULL) < 0) {
        fprintf(stderr, "could not open codec\n");
        exit(1);
    }
    
    return c;
}

void write_data(FILE* f, AVPacket *pkt, char end)
{
    uint8_t outbuf[4];
    if(end == 0){
	printf("encoding frame (size=%5d)\n", pkt->size);
	fwrite(pkt->data, 1, pkt->size, f);
    }
    else if(end == 1){
	/* add sequence end code to have a real mpeg file */
	outbuf[0] = 0x00;
	outbuf[1] = 0x00;
	outbuf[2] = 0x01;
	outbuf[3] = 0xb7;
	fwrite(outbuf, 1, 4, f);
	fclose(f);
	free(outbuf);
    }
}

void decode_init()
{
	av_init_packet(&avpkt);
	output_size = 100000;
	output_buffer = malloc(output_size);
	picture_buf = malloc((width_picture*height_picture*3)/2); 		/* size for YUV 420 */
	
	en_codec = encodec_create_new(AV_CODEC_ID_MPEG1VIDEO,width_picture,height_picture,24);
	picture = avframe_create_new(en_codec,picture_buf);	
// 	pFrame = avcodec_alloc_frame();
	got_picture = 0;   
	
	swsContext = sws_getContext(width_picture, height_picture, AV_PIX_FMT_RGB24,
				    width_picture, height_picture, en_codec->pix_fmt, 
				    SWS_FAST_BILINEAR, NULL, NULL, NULL);
	avFrameRGB.linesize[0] = width_picture * 3;
	avFrameRGB.data[0]     = (uint8_t*)malloc( avFrameRGB.linesize[0] * width_picture );
}

#endif