#include "libavcodec/avcodec.h"
// #include "libavcodec/sw"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libavutil/opt.h"
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "libavutil/imgutils.h"
#include "mcv4l2.h"

void errno_exit(const char *s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
//#ifdef WITH_V4L2_LIB
	fprintf(stderr, "%s\n",
			v4lconvert_get_error_message(v4lconvert_data));
//#endif
	exit(EXIT_FAILURE);
}

int xioctl(int fd, int request, void *arg)
{
	int r;

	do {
		r = ioctl(fd, request, arg);
	} while (r < 0 && EINTR == errno);
	return r;
}

int process_image(unsigned char *p, int len)
{
#ifdef WITH_V4L2_LIB
  if (v4lconvert_convert(v4lconvert_data,
			  &src_fmt,
			  &fmt,
			  p, len,
			  avFrameRGB.data[0], fmt.fmt.pix.sizeimage) < 0)
      return 0;
#endif
  return 1;
}

int read_frame(void)
{
    struct v4l2_buffer buf;
    int ret=0;
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (xioctl(fd, VIDIOC_DQBUF, &buf) < 0)
    {
      return 0;
    }
    ret = process_image(buffers[buf.index].start, buf.bytesused);

    if (xioctl(fd, VIDIOC_QBUF, &buf) < 0)
	    errno_exit("VIDIOC_QBUF");
    return ret;
}

void stop_capturing(void)
{
	enum v4l2_buf_type type;

	switch (io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;
	case V4L2_MEMORY_MMAP:
	case V4L2_MEMORY_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (xioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
			errno_exit("VIDIOC_STREAMOFF");
		break;
	}
}

void start_capturing(void)
{
	int i;
	enum v4l2_buf_type type;

	switch(io) {
	case V4L2_MEMORY_MMAP:
		printf("mmap method\n");
		for (i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

			if (xioctl(fd, VIDIOC_QBUF, &buf) < 0)
				errno_exit("VIDIOC_QBUF");
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (xioctl(fd, VIDIOC_STREAMON, &type) < 0)
			errno_exit("VIDIOC_STREAMON");
		break;
	case V4L2_MEMORY_USERPTR:
	case IO_METHOD_READ:
		break;
	}
}

void uninit_device(void)
{
	int i;

	switch (io) {
	case V4L2_MEMORY_MMAP:
		for (i = 0; i < n_buffers; ++i)
			if (-1 ==
			    munmap(buffers[i].start, buffers[i].length))
				errno_exit("munmap");
		break;
	case V4L2_MEMORY_USERPTR:
	case IO_METHOD_READ:
		break;
	}
	free(buffers);
}

void init_read(unsigned int buffer_size)
{
	buffers = calloc(1, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	buffers[0].length = buffer_size;
	buffers[0].start = malloc(buffer_size);

	if (!buffers[0].start) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
}

void init_mmap(void)
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (xioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
				"memory mapping\n", videoDevice);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s\n",
			videoDevice);
		exit(EXIT_FAILURE);
	}

	buffers = calloc(req.count, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		if (xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
			errno_exit("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap(NULL /* start anywhere */ ,
						buf.length,
						PROT_READ | PROT_WRITE
						/* required */ ,
						MAP_SHARED
						/* recommended */ ,
						fd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit("mmap");
	}
}

void init_userp(unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;
	unsigned int page_size;

	page_size = getpagesize();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

	CLEAR(req);

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (xioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
				"user pointer i/o\n", videoDevice);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	buffers = calloc(4, sizeof(*buffers));
	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
		buffers[n_buffers].length = buffer_size;
		buffers[n_buffers].start = memalign( /* boundary */ page_size,
						    buffer_size);

		if (!buffers[n_buffers].start) {
			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
		}
	}
}

void init_device(int w, int h)
{
	struct v4l2_capability cap;
	int ret;
	int sizeimage;

	avFrameRGB.linesize[0] = w * 3;
	avFrameRGB.data[0]     = (uint8_t*)malloc( avFrameRGB.linesize[0] * w );
	swsContext = sws_getContext(w, h, AV_PIX_FMT_RGB24,
					    w, h, AV_PIX_FMT_YUV420P,
					    SWS_FAST_BILINEAR, NULL, NULL, NULL);
	picture = av_frame_alloc();
	picture->format = AV_PIX_FMT_YUV420P;
	picture->width = w;
	picture->height = h;
	picture->pts = 0;
	av_image_alloc(picture->data, picture->linesize, w, h,
                         AV_PIX_FMT_YUV420P, 32);
    
	if (xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device\n",
				videoDevice);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device\n",
			videoDevice);
		exit(EXIT_FAILURE);
	}

	switch (io) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			fprintf(stderr, "%s does not support read i/o\n",
				videoDevice);
			exit(EXIT_FAILURE);
		}
		break;
	case V4L2_MEMORY_MMAP:
		break;
	case V4L2_MEMORY_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf(stderr,
				"%s does not support streaming i/o\n",
				videoDevice);
			exit(EXIT_FAILURE);
		}
		break;
	}

	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = w;
	fmt.fmt.pix.height = h;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
#ifdef WITH_V4L2_LIB
	v4lconvert_data = v4lconvert_create(fd);
	if (v4lconvert_data == NULL)
		errno_exit("v4lconvert_create");
	if (v4lconvert_try_format(v4lconvert_data, &fmt, &src_fmt) != 0)
		errno_exit("v4lconvert_try_format");
	ret = xioctl(fd, VIDIOC_S_FMT, &src_fmt);
	sizeimage = src_fmt.fmt.pix.sizeimage;
	dst_buf = malloc(fmt.fmt.pix.sizeimage);
	printf("raw pixfmt: %c%c%c%c %dx%d\n",
		src_fmt.fmt.pix.pixelformat & 0xff,
	       (src_fmt.fmt.pix.pixelformat >> 8) & 0xff,
	       (src_fmt.fmt.pix.pixelformat >> 16) & 0xff,
	       (src_fmt.fmt.pix.pixelformat >> 24) & 0xff,
		src_fmt.fmt.pix.width, src_fmt.fmt.pix.height);
#else
	ret = xioctl(fd, VIDIOC_S_FMT, &fmt);
	sizeimage = fmt.fmt.pix.sizeimage;
#endif

	if (ret < 0)
		errno_exit("VIDIOC_S_FMT");
//
//      /* Note VIDIOC_S_FMT may change width and height. */
//
	printf("pixfmt: %c%c%c%c %dx%d\n",
		fmt.fmt.pix.pixelformat & 0xff,
	       (fmt.fmt.pix.pixelformat >> 8) & 0xff,
	       (fmt.fmt.pix.pixelformat >> 16) & 0xff,
	       (fmt.fmt.pix.pixelformat >> 24) & 0xff,
		fmt.fmt.pix.width, fmt.fmt.pix.height);

	switch (io) {
	case IO_METHOD_READ:
		init_read(sizeimage);
		break;
	case V4L2_MEMORY_MMAP:
		init_mmap();
		break;
	case V4L2_MEMORY_USERPTR:
		init_userp(sizeimage);
		break;
	}
}

void close_device(void)
{
	sws_freeContext(swsContext);
	close(fd);
}

int open_device(void)
{
	struct stat st;

	if (stat(videoDevice, &st) < 0) {
		fprintf(stderr, "Cannot identify '%s': %d, %s\n",
			videoDevice, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device\n", videoDevice);
		exit(EXIT_FAILURE);
	}

	fd = open(videoDevice, O_RDWR /* required */  | O_NONBLOCK, 0);
	if (fd < 0) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n",
			videoDevice, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	return fd;
}