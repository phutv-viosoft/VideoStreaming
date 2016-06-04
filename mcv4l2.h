#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>		/* getopt_long() */
#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "stdint.h"
#include <asm/types.h>		/* for videodev2.h */
#include <linux/videodev2.h>
#include "libv4lconvert.h"

#define WITH_V4L2_LIB	1
#define SVV_VERSION 		"0.1"
#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define IO_METHOD_READ 7	/* !! must be != V4L2_MEMORY_MMAP / USERPTR */
#define NFRAMES 30
#define io 1

FILE *_file;
struct v4lconvert_data *v4lconvert_data;
struct v4l2_format src_fmt;	/* raw format */
unsigned char *dst_buf;
AVFrame *picture;
AVFrame avFrameRGB;
struct SwsContext *swsContext;

struct v4l2_format fmt;		/* gtk pormat */
char *videoDevice;// = "/dev/video0";
int fd;
struct buffer *buffers;
int n_buffers;
int grab, info, display_time;
struct buffer {
void *start;
size_t length;
};
struct timeval cur_time;
unsigned char *data_send;

void errno_exit(const char *s);
int xioctl(int fd, int request, void *arg);
int process_image(unsigned char *p, int len);
int read_frame(void);
int get_frame();
void stop_capturing(void);
void start_capturing(void);
void uninit_device(void);
void init_read(unsigned int buffer_size);
void init_mmap(void);
void init_userp(unsigned int buffer_size);
void init_device(int w, int h);
void close_device(void);
int open_device(void);