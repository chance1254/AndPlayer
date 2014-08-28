/*
 * ffmpegUtils.h
 *
 *  Created on: 28 рту. 2014 у.
 *      Author: 1
 */

#ifndef FFMPEGUTILS_H_
#define FFMPEGUTILS_H_
#include "libavformat/avformat.h"
void opt_codec(int *pstream_copy, char **pcodec_name,
                      int codec_type, const char *arg);
int opt_bitrate(const char *opt, const char *arg);
void new_video_stream(AVFormatContext *oc);

typedef struct AVStreamMap {
    int file_index;
    int stream_index;
    int sync_file_index;
    int sync_stream_index;
} AVStreamMap;

int av_exit(int ret);
AVFormatContext *opt_output_file(const char *filename);

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
        void (*func_arg)(const char *); //FIXME passing error code as int return would be nicer then exit() in the func
        int *int_arg;
        char **str_arg;
        float *float_arg;
        int (*func2_arg)(const char *, const char *);
        int64_t *int64_arg;
    } u;
    const char *help;
    const char *argname;
} OptionDef;

int opt_default(const char *opt, const char *arg);
int opt_preset(const char *opt, const char *arg);
int opt_bsf(const char *opt, const char *arg);
void opt_video_standard(const char *arg);
void opt_video_channel(const char *arg);
void opt_subtitle_tag(const char *arg);
void opt_new_subtitle_stream(void);
void opt_subtitle_codec(const char *arg);
void opt_audio_sample_fmt(const char *arg);
void opt_new_audio_stream(void);
void opt_audio_tag(const char *arg);
void opt_audio_codec(const char *arg);
int opt_audio_channels(const char *opt, const char *arg);
int opt_audio_rate(const char *opt, const char *arg);
void opt_new_video_stream(void);
void opt_video_tag(const char *arg);
void opt_top_field_first(const char *arg);
void opt_inter_matrix(const char *arg);
void opt_intra_matrix(const char *arg);
void opt_vstats_file (const char *arg);
void opt_vstats (void);
void opt_pass(const char *pass_str);
int opt_me_threshold(const char *opt, const char *arg);
void opt_video_codec(const char *arg);
void opt_video_rc_override_string(const char *arg);
void opt_qscale(const char *arg);
void opt_pad(const char *arg);
void opt_frame_crop_right(const char *arg);
void opt_frame_crop_left(const char *arg);
void opt_frame_crop_bottom(const char *arg);
void opt_frame_crop_top(const char *arg);
void opt_frame_pix_fmt(const char *arg);
void opt_frame_aspect_ratio(const char *arg);
void opt_frame_size(const char *arg);
int opt_frame_rate(const char *opt, const char *arg);
int opt_thread_count(const char *opt, const char *arg);
void opt_format(const char *arg);
AVFormatContext *opt_input_file(const char *filename);
void opt_map(const char *arg);
void opt_map_meta_data(const char *arg);

typedef struct AVMetaDataMap {
    int out_file;
    int in_file;
} AVMetaDataMap;

typedef struct {
	char *key;
	char *value;
} AVMetadataTag;

int opt_recording_time(const char *opt, const char *arg);
int opt_start_time(const char *opt, const char *arg);
int opt_input_ts_offset(const char *opt, const char *arg);
void opt_input_ts_scale(const char *arg);
int opt_rec_timestamp(const char *opt, const char *arg);
int opt_metadata(const char *opt, const char *arg);
int opt_timelimit(const char *opt, const char *arg);
int opt_verbose(const char *opt, const char *arg);
void opt_target(const char *arg);
int64_t parse_time_or_die(const char *context, const char *timestr, int is_duration);
double parse_number_or_die(const char *context, const char *numstr, int type, double min, double max);
#endif /* FFMPEGUTILS_H_ */
