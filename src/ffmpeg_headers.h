#ifndef FFMPEG_HEADERS_H
#define FFMPEG_HEADERS_H

#include <sys/types.h>


#define UINT64_C(c) c ## ULL
#define INT64_C(c) c ## LL

extern "C"
{

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

};

void ffmpeg_init_once();

#endif // FFMPEG_HEADERS_H
