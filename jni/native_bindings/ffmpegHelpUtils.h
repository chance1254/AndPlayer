#ifndef ffmpegHelpUtils_h
#define ffmpegHelpUtils_h
#include "libavformat/avformat.h"
typedef struct {
    const char *name;
    int flags;
#define HAS_ARG    0x0001
#define OPT_BOOL   0x0002
#define OPT_EXPERT 0x0004
#define OPT_STRING 0x0008
#define OPT_VIDEO  0x0010
#define OPT_AUDIO  0x0020
#define OPT_GRAB   0x0040
#define OPT_INT    0x0080
#define OPT_FLOAT  0x0100
#define OPT_SUBTITLE 0x0200
#define OPT_FUNC2  0x0400
#define OPT_INT64  0x0800
#define OPT_EXIT   0x1000
     union {
        void (*func_arg)(const char *);
        int *int_arg;
        char **str_arg;
        float *float_arg;
        int (*func2_arg)(const char *, const char *);
        int64_t *int64_arg;
    } u;
    const char *help;
    const char *argname;
} OptionDef;

typedef struct {
    AVRational time_base;
    int sample_rate;
    int channels;
    int width;
    int height;
    enum PixelFormat pix_fmt;
    int channel;
    const char *standard;
    unsigned int mpeg2ts_raw:1;
    unsigned int mpeg2ts_compute_pcr:1;
    unsigned int initial_pause:1;
    unsigned int prealloced_context:1;
    #if LIBAVFORMAT_VERSION_INT < (53<<16)
    enum AVCodecID video_codec_id;
    enum AVCodecID audio_codec_id;
    #endif
} AVFormatParameters;

int av_exit(int result);
AVFormatContext *opt_output_file(const char *filename);
void parse_options(int argc, char **argv, const OptionDef *options,
                   void (* parse_arg_function)(const char*));
void opt_codec(int *pstream_copy, char **pcodec_name,
                      int codec_type, const char *arg);
void new_video_stream(AVFormatContext *oc);
void set_context_opts(void *ctx, void *opts_ctx, int flags);
AVFormatContext *opt_input_file(const char *filename);
int opt_bitrate(const char *opt, const char *arg);
int opt_default(const char *opt, const char *arg);
#endif