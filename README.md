# Descriptions

### Version
0.1


###############################################################################################
# Compile && Installation source code

#### Get the Dependencies
Copy and paste the whole code box for each step. First install the dependencies:
```sh
$ sudo apt-get update
$ sudo apt-get install -y libcurl4-openssl-dev libv4l-dev 
$ sudo apt-get -y install autoconf automake build-essential libass-dev libfreetype6-dev \
  libsdl1.2-dev libtheora-dev libtool libva-dev libvdpau-dev libvorbis-dev libxcb1-dev \
  libxcb-shm0-dev libxcb-xfixes0-dev pkg-config texinfo zlib1g-dev
```
* Note: Server users can omit the ffplay and x11grab dependencies: libsdl1.2-dev libva-dev libvdpau-dev libxcb1-dev libxcb-shm0-dev libxcb-xfixes0-dev.

Now make a directory for the source files that will be downloaded later in this guide:
```sh
$ mkdir ~/sources_code
```
#### Compilation & Installation
- Install Curl
Run on command line:
```sh
sudo apt-get install libcurl3
```
Otherwise you can compile:
```sh
$ cd ~/sources_code
$ wget https://curl.haxx.se/download/curl-7.49.1.tar.gz
$ tar xzvf curl-7.49.1.tar.gz
$ cd curl-*
$ ./configure
$ make
$ sudo make install
```
- Install Yasm

If your repository offers a yasm package ≥ 1.2.0 then you can install that instead of compiling:
```sh
sudo apt-get install yasm
```
Otherwise you can compile:
```sh
$ cd ~/sources_code
$ wget http://www.tortall.net/projects/yasm/releases/yasm-1.3.0.tar.gz
$ tar xzvf yasm-1.3.0.tar.gz
$ cd yasm-1.3.0
$ ./configure
$ make
$ sudo make install
```
- Install libx264

If your repository offers a libx264-dev package ≥ 0.118 then you can install that instead of compiling:
```sh
$ sudo apt-get install libx264-dev
```
Otherwise you can compile:
```sh
$ cd ~/sources_code
$ wget http://download.videolan.org/pub/x264/snapshots/last_x264.tar.bz2
$ tar xjvf last_x264.tar.bz2
$ cd x264-snapshot*
$ ./configure --enable-static --disable-opencl
$ make
$ sudo make install
```
- Install libfdk-aac

Requires ffmpeg to be configured with --enable-libfdk-aac (and --enable-nonfree if you also included --enable-gpl).
```sh
$ cd ~/sources_code
$ wget -O fdk-aac.tar.gz https://github.com/mstorsjo/fdk-aac/tarball/master
$ tar xzvf fdk-aac.tar.gz
$ cd mstorsjo-fdk-aac*
$ autoreconf -fiv
$ ./configure --disable-shared
$ make
$ sudo make install
```
- Install libmp3lame

If your repository offers a libmp3lame-dev package ≥ 3.98.3 then you can install that instead of compiling:
```sh
$ sudo apt-get install libmp3lame-dev
```
Otherwise you can compile:
```sh
$ sudo apt-get install nasm
$ cd ~/sources_code
$ wget http://downloads.sourceforge.net/project/lame/lame/3.99/lame-3.99.5.tar.gz
$ tar xzvf lame-3.99.5.tar.gz
$ cd lame-3.99.5
$ ./configure --enable-nasm --disable-shared
$ make
$ sudo make install
```
- Install libopus

If your repository offers a libopus-dev package ≥ 1.1 then you can install that instead of compiling:
```sh
$ sudo apt-get install libopus-dev
```
Otherwise you can compile:
```sh
$ cd ~/sources_code
$ wget http://downloads.xiph.org/releases/opus/opus-1.1.2.tar.gz
$ tar xzvf opus-1.1.2.tar.gz
$ cd opus-1.1.2
$ ./configure --disable-shared
$ make
$ sudo make install
```
- Install libvpx

Requires ffmpeg to be configured with --enable-libvpx.
```sh
$ cd ~/sources_code
$ wget http://storage.googleapis.com/downloads.webmproject.org/releases/webm/libvpx-1.5.0.tar.bz2
$ tar xjvf libvpx-1.5.0.tar.bz2
$ cd libvpx-1.5.0
$ ./configure --disable-examples --disable-unit-tests
$ make
$ sudo make install
```
- Install ffmpeg
```sh
cd ~/sources_code
$ wget http://ffmpeg.org/releases/ffmpeg-snapshot.tar.bz2
$ tar xjvf ffmpeg-snapshot.tar.bz2
$ cd ffmpeg
$ ./configure \
  --enable-gpl \
  --enable-libass \
  --enable-libfdk-aac \
  --enable-libfreetype \
  --enable-libmp3lame \
  --enable-libopus \
  --enable-libtheora \
  --enable-libvorbis \
  --enable-libvpx \
  --enable-libx264 \
  --enable-nonfree
$ make
$ sudo make install
```
#### Compile && run source code
- Clone source code from [github - media streaming](https://github.com/phutv-viosoft/VideoStreaming.git)
```sh
cd ~/sources_code
$ git clone https://github.com/phutv-viosoft/VideoStreaming.git
```
- modify some of defined in source
Open main.c file and changes as folow:
```sh
#define MY_CAMERA		"RPCAMERA_00001"      /* Name of your camera */
#define STREAM_FRAME_RATE 	25                    /* 25 images/s */
#define STREAM_PIX_FMT    	AV_PIX_FMT_YUV420P    /* default pix_fmt */
#define WIDTH			320
#define HEIGHT			240
#define SERVER_ADDRESS		"192.168.0.103:8080/" /* Server AddressIP*/
#define DEVICE_NAME		"/dev/video0"         /* Device name of webcame*/
```
- Compile && rung
```sh
$ cd cd ~/sources_code/VideoStreaming
$ make
$ ./rtspServer
```
*************** thank you..! *****************