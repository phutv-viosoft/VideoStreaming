CC = gcc

object_main = rtspServer

libs = $(shell pkg-config --cflags --libs libv4l2 libavutil libswscale libavcodec libavformat libv4lconvert libcurl)
all:
	$(CC) -Wall -O2 main.c mcv4l2.c -o $(object_main) $(libs)
clean:
	-rm *~ $(object_main)
