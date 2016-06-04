/*
 ============================================================================
 Name        : VideoStreaming.c
 Author      : PhuTran
 Version     :
 Copyright   : Lab411
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include <stdio.h>
#include <time.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <curl/curl.h>
// #include <opencv/highgui.h>
// #include<opencv/cv.h>
#include "mcv4l2.h"

#define MY_CAMERA		"RPCAMERA_00001"
#define STREAM_FRAME_RATE 	25 /* 25 images/s */
#define STREAM_PIX_FMT    	AV_PIX_FMT_YUV420P /* default pix_fmt */
#define WIDTH			320
#define HEIGHT			240
#define SERVER_ADDRESS		"192.168.0.103:8080/"
#define DEVICE_NAME		"/dev/video0"

typedef enum {
    HTTP_CLIENT_REQUEST,
    HTTP_CLIENT_MESSAGE, 
    HTTP_CLIENT_DATA,
    HTTP_CLIENT_ERROR
} status;

static int got_packet;
static CURL *curl;
static uint64_t frame_count = 0;
static AVCodecContext *video_codec;
static status current_status;
//IplImage* IPframe;
/* video output */

static AVCodecContext *avcodec_creat_new(int CODEC_ID)
{
    AVCodec *codec;
    AVCodecContext *c= NULL;
    
    codec = avcodec_find_encoder(CODEC_ID);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    c->bit_rate = 400000;
    c->width = WIDTH;
    c->height = HEIGHT;
    c->time_base = (AVRational){1,STREAM_FRAME_RATE};
    c->gop_size = 10;
    c->max_b_frames = 1;
    c->pix_fmt = STREAM_PIX_FMT;

    if (CODEC_ID == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);
    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }
    
    return c;
}

// static void fill_yuv_image(int width, int height)
// {
//   while(read_frame() == 0){}
//   sws_scale(swsContext, avFrameRGB.data, avFrameRGB.linesize, 0, height, picture->data, picture->linesize);
// 	if(avframe_to_iplimage(picture, IPframe)){
// 		cvNamedWindow("window",CV_WINDOW_AUTOSIZE);
// 		cvShowImage("window", IPframe);
// 		cvWaitKey(1);
// 	}
// }

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
  AVPacket pkt = { 0 };  
  av_init_packet(&pkt);
  
  switch(current_status) {
    case HTTP_CLIENT_REQUEST:
      sleep(5);
      size = 22;
      memcpy(ptr, "REQUEST" "@" MY_CAMERA, size);
    case HTTP_CLIENT_MESSAGE:
    case HTTP_CLIENT_ERROR:
      break;
    case HTTP_CLIENT_DATA:
      /*sleep and wait for read frame completed*/
      usleep(100000);
      while(read_frame() == 0){}
      /*Convert data picture from RGB to YUV420P*/
      sws_scale(swsContext, avFrameRGB.data, avFrameRGB.linesize, 0, video_codec->height, picture->data, picture->linesize);
      picture->pts = frame_count;
      /*encode video frame*/
      if (avcodec_encode_video2(video_codec, &pkt, picture, &got_packet) < 0) {
	fprintf(stderr, "Error encoding video frame:");
	exit(1);
      }    
      size = pkt.size;
      if(size > 0) {
	memcpy(ptr, pkt.data, size);
      } else if(size == 0){
	size = 5;
	memcpy(ptr, "error", size);
      }    
      frame_count++;  
      break;
    default:
      break;
  }
  return size;
}

static size_t write_callback( void *ptr, size_t size, size_t nmemb, void *stream)
{ 
  printf("DATA received = %s\n",(char *)ptr);
  if(strstr((char *)ptr, "REQUEST@" "200 OK")) {
    curl_easy_pause(curl, CURLPAUSE_SEND);
    current_status = HTTP_CLIENT_ERROR;
  }
  else if(strstr((char *)ptr, "DATA@" MY_CAMERA)) {
    curl_easy_pause(curl, CURLPAUSE_CONT);
    current_status = HTTP_CLIENT_DATA;
  }
  else if(strstr((char *)ptr, "PAUSE@" MY_CAMERA)) {
    curl_easy_pause(curl, CURLPAUSE_SEND);
    current_status = HTTP_CLIENT_ERROR;
  }
  return size*nmemb;
}

static void init_main()
{
  videoDevice = DEVICE_NAME;  
  open_device();
  init_device(WIDTH, HEIGHT);
  start_capturing();
  avcodec_register_all();
  av_register_all();
  avformat_network_init();
  video_codec = avcodec_creat_new(AV_CODEC_ID_MPEG1VIDEO);
}

static void terminal_main()
{
  curl_global_cleanup();
  stop_capturing();
  uninit_device();
  close_device();
}

int main(int argc, char **argv)
{      
  
  CURLcode res;
  char *url = SERVER_ADDRESS;
  current_status = HTTP_CLIENT_REQUEST;
  init_main();
  curl_global_init(CURL_GLOBAL_ALL);
  /* get a curl handle */
  curl = curl_easy_init();
  if(curl) {    
    /* enable uploading */
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    /* HTTP PUT please */
    curl_easy_setopt(curl, CURLOPT_PUT, 1L);
    /* we want to use our own read function */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));    
    /* always cleanup */
    curl_easy_cleanup(curl);
  }
  terminal_main();
  
  return 0;
}