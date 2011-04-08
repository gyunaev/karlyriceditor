#include "ffmpeg_headers.h"

static bool ffmpeg_initialized = false;

void ffmpeg_init_once()
{
	if ( !ffmpeg_initialized )
	{
		avcodec_init();
		avcodec_register_all();
		av_register_all();
	}
}


