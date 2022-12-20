#ifndef _FFMPEG_COMPAT_H_
#define _FFMPEG_COMPAT_H_

#ifdef FFMPEG_v42
#define AVCompat_AVInputFormat AVInputFormat
#define AVCompat_AVOutputFormat AVOutputFormat
#define AVCompat_AVCodec AVCodec
#else
#define AVCompat_AVInputFormat const AVInputFormat
#define AVCompat_AVOutputFormat const AVOutputFormat
#define AVCompat_AVCodec const AVCodec
#endif

#endif

