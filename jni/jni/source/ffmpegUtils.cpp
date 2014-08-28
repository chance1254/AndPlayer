#define TAG "ffmpegUtils"
#include "../include/ffmpegUtils.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/mem.h"
#include <android/log.h>
#define MAX_FILES 2
#define MAX_STREAMS 4

static const char *last_asked_format = NULL;
static AVFormatContext *input_files[MAX_FILES];
static int64_t input_files_ts_offset[MAX_FILES];
static double input_files_ts_scale[MAX_FILES][MAX_STREAMS];
static AVCodec *input_codecs[MAX_FILES * MAX_STREAMS];
static int nb_input_files = 0;
static int nb_icodecs;

static AVFormatContext *output_files[MAX_FILES];
static AVCodec *output_codecs[MAX_FILES * MAX_STREAMS];
static int nb_output_files = 0;
static int nb_ocodecs;

static AVStreamMap stream_maps[MAX_FILES * MAX_STREAMS];
static int nb_stream_maps;

static AVMetaDataMap meta_data_maps[MAX_FILES];
static int nb_meta_data_maps;

static int frame_width = 0;
static int frame_height = 0;
static float frame_aspect_ratio = 0;
static enum PixelFormat frame_pix_fmt = PIX_FMT_NONE;
static enum SampleFormat audio_sample_fmt = AV_SAMPLE_FMT_NONE;
static int frame_topBand = 0;
static AVMetadataTag *metadata;
static int frame_bottomBand = 0;
static int frame_leftBand = 0;
static int frame_rightBand = 0;
static int max_frames[4] = { INT_MAX, INT_MAX, INT_MAX, INT_MAX };
static AVRational frame_rate;
static float video_qscale = 0;
static uint16_t *intra_matrix = NULL;
static uint16_t *inter_matrix = NULL;
static const char *video_rc_override_string = NULL;
static int video_disable = 0;
static int video_discard = 0;
static char *video_codec_name = NULL;
static unsigned int video_codec_tag = 0;
static char *video_language = NULL;
static int same_quality = 0;
static int do_deinterlace = 0;
static int top_field_first = -1;
static int me_threshold = 0;
static int intra_dc_precision = 8;
static int loop_input = 0;
static int loop_output = AVFMT_NOOUTPUTLOOP;
static int qp_hist = 0;
#if CONFIG_AVFILTER
static char *vfilters = NULL;
AVFilterGraph *graph = NULL;
#endif

static int intra_only = 0;
static int audio_sample_rate = 44100;
static int64_t channel_layout = 0;
#define QSCALE_NONE -99999
static float audio_qscale = QSCALE_NONE;
static int audio_disable = 0;
static int audio_channels = 1;
static char *audio_codec_name = NULL;
static unsigned int audio_codec_tag = 0;
static char *audio_language = NULL;

static int subtitle_disable = 0;
static char *subtitle_codec_name = NULL;
static char *subtitle_language = NULL;
static unsigned int subtitle_codec_tag = 0;

static float mux_preload = 0.5;
static float mux_max_delay = 0.7;

static int64_t recording_time = INT64_MAX;
static int64_t start_time = 0;
static int64_t rec_timestamp = 0;
static int64_t input_ts_offset = 0;
static int file_overwrite = 0;
static int metadata_count;
static AVMetadataTag *metadata;
static int do_benchmark = 0;
static int do_hex_dump = 0;
static int do_pkt_dump = 0;
static int do_psnr = 0;
static int do_pass = 0;
static char *pass_logfilename_prefix = NULL;
static int audio_stream_copy = 0;
static int video_stream_copy = 0;
static int subtitle_stream_copy = 0;
static int video_sync_method = -1;
static int audio_sync_method = 0;
static float audio_drift_threshold = 0.1;
static int copy_ts = 0;
static int opt_shortest = 0;
static int video_global_header = 0;
static char *vstats_filename;
static FILE *vstats_file;
static int opt_programid = 0;
static int copy_initial_nonkeyframes = 0;

static int rate_emu = 0;

static int video_channel = 0;
static char *video_standard;

static int audio_volume = 256;

static int exit_on_error = 0;
static int using_stdin = 0;
static int verbose = 1;
static int thread_count = 1;
static int q_pressed = 0;
static int64_t video_size = 0;
static int64_t audio_size = 0;
static int64_t extra_size = 0;
static int nb_frames_dup = 0;
static int nb_frames_drop = 0;
static int input_sync;
static uint64_t limit_filesize = 0;
static int force_fps = 0;

static int pgmyuv_compatibility_hack = 0;
static float dts_delta_threshold = 10;

static unsigned int sws_flags = SWS_BICUBIC;

static int64_t timer_start;

static uint8_t *audio_buf;
static uint8_t *audio_out;
unsigned int allocated_audio_out_size, allocated_audio_buf_size;

static short *samples;

static AVBitStreamFilterContext *video_bitstream_filters = NULL;
static AVBitStreamFilterContext *audio_bitstream_filters = NULL;
static AVBitStreamFilterContext *subtitle_bitstream_filters = NULL;
static AVBitStreamFilterContext *bitstream_filters[MAX_FILES][MAX_STREAMS];

void opt_codec(int *pstream_copy, char **pcodec_name, int codec_type,
		const char *arg) {
	av_freep(pcodec_name);
	if (!strcmp(arg, "copy")) {
		*pstream_copy = 1;
	} else {
		*pcodec_name = av_strdup(arg);
	}
}

int opt_bitrate(const char *opt, const char *arg) {
	int codec_type = opt[0] == 'a' ? AVMEDIA_TYPE_AUDIO : AVMEDIA_TYPE_VIDEO;

	opt_default(opt, arg);

	if (av_get_int(avcodec_opts[codec_type], "b", NULL) < 1000)
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"WARNING: The bitrate parameter is set too low. It takes bits/s as argument, not kbits/s\n");

	return 0;
}

void new_video_stream(AVFormatContext *oc) {
	AVStream *st;
	AVCodecContext *video_enc;
	enum AVCodecID codecID;
	st = av_new_stream(oc, oc->nb_streams);
	if (!st) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Could not alloc stream\n");
		av_exit(1);
	}
}

int av_exit(int ret) {
	int i;

	/* close files */
	for (i = 0; i < nb_output_files; i++) {
		/* maybe av_close_output_file ??? */
		AVFormatContext *s = output_files[i];
		int j;
		if (!(s->oformat->flags & AVFMT_NOFILE) && s->pb)
			url_fclose(s->pb);
		for (j = 0; j < s->nb_streams; j++) {
			av_metadata_free(&s->streams[j]->metadata);
			av_free(s->streams[j]->codec);
			av_free(s->streams[j]);
		}
		for (j = 0; j < s->nb_programs; j++) {
			av_metadata_free(&s->programs[j]->metadata);
		}
		for (j = 0; j < s->nb_chapters; j++) {
			av_metadata_free(&s->chapters[j]->metadata);
		}
		av_metadata_free(&s->metadata);
		av_free(s);
	}
	for (i = 0; i < nb_input_files; i++)
		av_close_input_file(input_files[i]);

	av_free(intra_matrix);
	av_free(inter_matrix);

	if (vstats_file)
		fclose(vstats_file);
	av_free(vstats_filename);

	av_free (opt_names);

	av_free(video_codec_name);
	av_free(audio_codec_name);
	av_free(subtitle_codec_name);

	av_free(video_standard);

#if CONFIG_POWERPC_PERF
	void powerpc_display_perf_report(void);
	powerpc_display_perf_report();
#endif /* CONFIG_POWERPC_PERF */

	for (i = 0; i < AVMEDIA_TYPE_NB; i++)
		av_free (avcodec_opts[i]);
	av_free (avformat_opts);
	av_free (sws_opts);
	av_free(audio_buf);
	av_free(audio_out);
	allocated_audio_buf_size = allocated_audio_out_size = 0;
	av_free(samples);

#if CONFIG_AVFILTER
	avfilter_uninit();
#endif

	if (received_sigterm) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"Received signal %d: terminating.\n", (int) received_sigterm);
		exit(255);
	}

	exit(ret); /* not all OS-es handle main() return value */
	return ret;
}

int64_t parse_time_or_die(const char *context, const char *timestr,
		int is_duration) {
	int64_t us = parse_date(timestr, is_duration);
	if (us == INT64_MIN) {
		fprintf(stderr, "Invalid %s specification for %s: %s\n",
				is_duration ? "duration" : "date", context, timestr);
		exit(1);
	}
	return us;
}

AVFormatContext *opt_output_file(const char *filename) {
	AVFormatContext *oc;
	int err, use_video, use_audio, use_subtitle;
	int input_has_video, input_has_audio, input_has_subtitle;
	AVFormatParameters params, *ap = &params;
	AVOutputFormat *file_oformat;

	if (!strcmp(filename, "-"))
		filename = "pipe:";

	oc = avformat_alloc_context();
	if (!oc) {
		print_error(filename, AVERROR(ENOMEM));
		av_exit(1);
	}

	if (last_asked_format) {
		file_oformat = av_guess_format(last_asked_format, NULL, NULL);
		if (!file_oformat) {
			__android_log_print(ANDROID_LOG_ERROR, TAG,
					"Requested output format '%s' is not a suitable output format\n",
					last_asked_format);
			av_exit(1);
		}
		last_asked_format = NULL;
	} else {
		file_oformat = av_guess_format(NULL, filename, NULL);
		if (!file_oformat) {
			__android_log_print(ANDROID_LOG_ERROR, TAG,
					"Unable to find a suitable output format for '%s'\n",
					filename);
			av_exit(1);
		}
	}

	oc->oformat = file_oformat;
	av_strlcpy(oc->filename, filename, sizeof(oc->filename));

	if (!strcmp(file_oformat->name, "ffm")
			&& av_strstart(filename, "http:", NULL)) {
		/* special case for files sent to ffserver: we get the stream
		 parameters from ffserver */
		int err = read_ffserver_streams(oc, filename);
		if (err < 0) {
			print_error(filename, err);
			av_exit(1);
		}
	} else {
		use_video = file_oformat->video_codec != CODEC_ID_NONE
				|| video_stream_copy || video_codec_name;
		use_audio = file_oformat->audio_codec != CODEC_ID_NONE
				|| audio_stream_copy || audio_codec_name;
		use_subtitle = file_oformat->subtitle_codec != CODEC_ID_NONE
				|| subtitle_stream_copy || subtitle_codec_name;

		/* disable if no corresponding type found and at least one
		 input file */
		if (nb_input_files > 0) {
			check_audio_video_sub_inputs(&input_has_video, &input_has_audio,
					&input_has_subtitle);
			if (!input_has_video)
				use_video = 0;
			if (!input_has_audio)
				use_audio = 0;
			if (!input_has_subtitle)
				use_subtitle = 0;
		}

		/* manual disable */
		if (audio_disable) {
			use_audio = 0;
		}
		if (video_disable) {
			use_video = 0;
		}
		if (subtitle_disable) {
			use_subtitle = 0;
		}

		if (use_video) {
			new_video_stream(oc);
		}

		if (use_audio) {
			new_audio_stream(oc);
		}

		if (use_subtitle) {
			new_subtitle_stream(oc);
		}

		oc->timestamp = rec_timestamp;

		for (; metadata_count > 0; metadata_count--) {
			av_metadata_set2(&oc->metadata, metadata[metadata_count - 1].key,
					metadata[metadata_count - 1].value, 0);
		}
		av_metadata_conv(oc, oc->oformat->metadata_conv, NULL);
	}

	output_files[nb_output_files++] = oc;

	/* check filename in case of an image number is expected */
	if (oc->oformat->flags & AVFMT_NEEDNUMBER) {
		if (!av_filename_number_test(oc->filename)) {
			print_error(oc->filename, AVERROR_NUMEXPECTED);
			av_exit(1);
		}
	}

	if (!(oc->oformat->flags & AVFMT_NOFILE)) {
		/* test if it already exists to avoid loosing precious files */
		if (!file_overwrite
				&& (strchr(filename, ':') == NULL || filename[1] == ':'
						|| av_strstart(filename, "file:", NULL))) {
			if (url_exist(filename)) {
				if (!using_stdin) {
					__android_log_print(ANDROID_LOG_ERROR, TAG,
							"File '%s' already exists. Overwrite ? [y/N] ",
							filename);
					fflush(stderr);
					if (!read_yesno()) {
						__android_log_print(ANDROID_LOG_ERROR, TAG,
								"Not overwriting - exiting\n");
						av_exit(1);
					}
				} else {
					__android_log_print(ANDROID_LOG_ERROR, TAG,
							"File '%s' already exists. Exiting.\n", filename);
					av_exit(1);
				}
			}
		}

		/* open the file */
		if ((err = url_fopen(&oc->pb, filename, URL_WRONLY)) < 0) {
			print_error(filename, err);
			av_exit(1);
		}
	}

	memset(ap, 0, sizeof(*ap));
	if (av_set_parameters(oc, ap) < 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"%s: Invalid encoding parameters\n", oc->filename);
		av_exit(1);
	}

	oc->preload = (int) (mux_preload * AV_TIME_BASE);
	oc->max_delay = (int) (mux_max_delay * AV_TIME_BASE);
	oc->loop_output = loop_output;
	oc->flags |= AVFMT_FLAG_NONBLOCK;

	set_context_opts(oc, avformat_opts, AV_OPT_FLAG_ENCODING_PARAM);
	return oc;
}

int opt_default(const char *opt, const char *arg) {
	int type;
	int ret = 0;
	const AVOption *o = NULL;
	int opt_types[] = { AV_OPT_FLAG_VIDEO_PARAM, AV_OPT_FLAG_AUDIO_PARAM, 0,
			AV_OPT_FLAG_SUBTITLE_PARAM, 0 };

	for (type = 0; type < AVMEDIA_TYPE_NB && ret >= 0; type++) {
		const AVOption *o2 = av_find_opt(avcodec_opts[0], opt, NULL,
				opt_types[type], opt_types[type]);
		if (o2)
			ret = av_set_string3(avcodec_opts[type], opt, arg, 1, &o);
	}
	if (!o)
		ret = av_set_string3(avformat_opts, opt, arg, 1, &o);
	if (!o && sws_opts)
		ret = av_set_string3(sws_opts, opt, arg, 1, &o);
	if (!o) {
		if (opt[0] == 'a')
			ret = av_set_string3(avcodec_opts[AVMEDIA_TYPE_AUDIO], opt + 1, arg,
					1, &o);
		else if (opt[0] == 'v')
			ret = av_set_string3(avcodec_opts[AVMEDIA_TYPE_VIDEO], opt + 1, arg,
					1, &o);
		else if (opt[0] == 's')
			ret = av_set_string3(avcodec_opts[AVMEDIA_TYPE_SUBTITLE], opt + 1,
					arg, 1, &o);
	}
	if (o && ret < 0) {
		fprintf(stderr, "Invalid value '%s' for option '%s'\n", arg, opt);
		exit(1);
	}
	if (!o) {
		fprintf(stderr, "Unrecognized option '%s'\n", opt);
		exit(1);
	}

	//    av_log(NULL, AV_LOG_ERROR, "%s:%s: %f 0x%0X\n", opt, arg, av_get_double(avcodec_opts, opt, NULL), (int)av_get_int(avcodec_opts, opt, NULL));

	//FIXME we should always use avcodec_opts, ... for storing options so there will not be any need to keep track of what i set over this
	opt_names = av_realloc(opt_names, sizeof(void*) * (opt_name_count + 1));
	opt_names[opt_name_count++] = o->name;

	if (avcodec_opts[0]->debug || avformat_opts->debug)
		av_log_set_level(AV_LOG_DEBUG);
	return 0;
}

int opt_preset(const char *opt, const char *arg) {
	FILE *f = NULL;
	char filename[1000], tmp[1000], tmp2[1000], line[1000];
	int i;
	const char *base[3] = { getenv("FFMPEG_DATADIR"), getenv("HOME"),
			FFMPEG_DATADIR, };

	if (*opt != 'f') {
		for (i = 0; i < 3 && !f; i++) {
			if (!base[i])
				continue;
			snprintf(filename, sizeof(filename), "%s%s/%s.ffpreset", base[i],
					i != 1 ? "" : "/.ffmpeg", arg);
			f = fopen(filename, "r");
			if (!f) {
				char *codec_name =
						*opt == 'v' ? video_codec_name :
						*opt == 'a' ? audio_codec_name : subtitle_codec_name;
				snprintf(filename, sizeof(filename), "%s%s/%s-%s.ffpreset",
						base[i], i != 1 ? "" : "/.ffmpeg", codec_name, arg);
				f = fopen(filename, "r");
			}
		}
	} else {
		av_strlcpy(filename, arg, sizeof(filename));
		f = fopen(filename, "r");
	}

	if (!f) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"File for preset '%s' not found\n", arg);
		av_exit(1);
	}

	while (!feof(f)) {
		int e = fscanf(f, "%999[^\n]\n", line) - 1;
		if (line[0] == '#' && !e)
			continue;
		e |= sscanf(line, "%999[^=]=%999[^\n]\n", tmp, tmp2) - 2;
		if (e) {
			__android_log_print(ANDROID_LOG_ERROR, TAG,
					"%s: Invalid syntax: '%s'\n", filename, line);
			av_exit(1);
		}
		if (!strcmp(tmp, "acodec")) {
			opt_audio_codec(tmp2);
		} else if (!strcmp(tmp, "vcodec")) {
			opt_video_codec(tmp2);
		} else if (!strcmp(tmp, "scodec")) {
			opt_subtitle_codec(tmp2);
		} else if (opt_default(tmp, tmp2) < 0) {
			__android_log_print(ANDROID_LOG_ERROR, TAG,
					"%s: Invalid option or argument: '%s', parsed as '%s' = '%s'\n",
					filename, line, tmp, tmp2);
			av_exit(1);
		}
	}

	fclose(f);

	return 0;
}

int opt_bsf(const char *opt, const char *arg) {
	AVBitStreamFilterContext *bsfc = av_bitstream_filter_init(arg); //FIXME split name and args for filter at '='
	AVBitStreamFilterContext **bsfp;

	if (!bsfc) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"Unknown bitstream filter %s\n", arg);
		av_exit(1);
	}

	bsfp = *opt == 'v' ? &video_bitstream_filters :
			*opt == 'a' ?
					&audio_bitstream_filters : &subtitle_bitstream_filters;
	while (*bsfp)
		bsfp = &(*bsfp)->next;

	*bsfp = bsfc;

	return 0;
}

void opt_video_standard(const char *arg) {
	video_standard = av_strdup(arg);
}

void opt_video_channel(const char *arg) {
	video_channel = strtol(arg, NULL, 0);
}

void opt_subtitle_tag(const char *arg) {
	char *tail;
	subtitle_codec_tag = strtol(arg, &tail, 0);

	if (!tail || *tail)
		subtitle_codec_tag = arg[0] + (arg[1] << 8) + (arg[2] << 16)
				+ (arg[3] << 24);
}

void opt_new_subtitle_stream(void) {
	AVFormatContext *oc;
	if (nb_output_files <= 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"At least one output file must be specified\n");
		av_exit(1);
	}
	oc = output_files[nb_output_files - 1];
	new_subtitle_stream(oc);
}

void opt_subtitle_codec(const char *arg) {
	opt_codec(&subtitle_stream_copy, &subtitle_codec_name,
			AVMEDIA_TYPE_SUBTITLE, arg);
}

void opt_audio_sample_fmt(const char *arg) {
	if (strcmp(arg, "list"))
		audio_sample_fmt = avcodec_get_sample_fmt(arg);
	else {
		list_fmts(avcodec_sample_fmt_string, SAMPLE_FMT_NB);
		av_exit(0);
	}
}

void opt_new_audio_stream(void) {
	AVFormatContext *oc;
	if (nb_output_files <= 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"At least one output file must be specified\n");
		av_exit(1);
	}
	oc = output_files[nb_output_files - 1];
	new_audio_stream(oc);
}

void opt_audio_tag(const char *arg) {
	char *tail;
	audio_codec_tag = strtol(arg, &tail, 0);

	if (!tail || *tail)
		audio_codec_tag = arg[0] + (arg[1] << 8) + (arg[2] << 16)
				+ (arg[3] << 24);
}

void opt_audio_codec(const char *arg) {
	opt_codec(&audio_stream_copy, &audio_codec_name, AVMEDIA_TYPE_AUDIO, arg);
}

int opt_audio_channels(const char *opt, const char *arg) {
	audio_channels = parse_number_or_die(opt, arg, OPT_INT64, 0, INT_MAX);
	return 0;
}

int opt_audio_rate(const char *opt, const char *arg) {
	audio_sample_rate = parse_number_or_die(opt, arg, OPT_INT64, 0, INT_MAX);
	return 0;
}

double parse_number_or_die(const char *context, const char *numstr, int type,
		double min, double max) {
	char *tail;
	const char *error;
	double d = av_strtod(numstr, &tail);
	if (*tail)
		error = "Expected number for %s but found: %s\n";
	else if (d < min || d > max)
		error = "The value for %s was %s which is not within %f - %f\n";
	else if (type == OPT_INT64 && (int64_t) d != d)
		error = "Expected int64 for %s but found %s\n";
	else
		return d;
	fprintf(stderr, error, context, numstr, min, max);
	exit(1);
}

void opt_new_video_stream(void) {
	AVFormatContext *oc;
	if (nb_output_files <= 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"At least one output file must be specified\n");
		av_exit(1);
	}
	oc = output_files[nb_output_files - 1];
	new_video_stream(oc);
}

void opt_video_tag(const char *arg) {
	char *tail;
	video_codec_tag = strtol(arg, &tail, 0);

	if (!tail || *tail)
		video_codec_tag = arg[0] + (arg[1] << 8) + (arg[2] << 16)
				+ (arg[3] << 24);
}

void opt_top_field_first(const char *arg) {
	top_field_first = atoi(arg);
}

void opt_inter_matrix(const char *arg) {
	inter_matrix = av_mallocz(sizeof(uint16_t) * 64);
	parse_matrix_coeffs(inter_matrix, arg);
}

void opt_intra_matrix(const char *arg) {
	intra_matrix = av_mallocz(sizeof(uint16_t) * 64);
	parse_matrix_coeffs(intra_matrix, arg);
}

void opt_vstats_file(const char *arg) {
	av_free(vstats_filename);
	vstats_filename = av_strdup(arg);
}

void opt_vstats(void) {
	char filename[40];
	time_t today2 = time(NULL);
	struct tm *today = localtime(&today2);

	snprintf(filename, sizeof(filename), "vstats_%02d%02d%02d.log",
			today->tm_hour, today->tm_min, today->tm_sec);
	opt_vstats_file(filename);
}

void opt_pass(const char *pass_str) {
	int pass;
	pass = atoi(pass_str);
	if (pass != 1 && pass != 2) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"pass number can be only 1 or 2\n");
		av_exit(1);
	}
	do_pass = pass;
}

int opt_me_threshold(const char *opt, const char *arg) {
	me_threshold = parse_number_or_die(opt, arg, OPT_INT64, INT_MIN, INT_MAX);
	return 0;
}

void opt_video_codec(const char *arg) {
	opt_codec(&video_stream_copy, &video_codec_name, AVMEDIA_TYPE_VIDEO, arg);
}

void opt_video_rc_override_string(const char *arg) {
	video_rc_override_string = arg;
}

void opt_qscale(const char *arg) {
	video_qscale = atof(arg);
	if (video_qscale <= 0 || video_qscale > 255) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"qscale must be > 0.0 and <= 255\n");
		av_exit(1);
	}
}

void opt_pad(const char *arg) {
	__android_log_print(ANDROID_LOG_ERROR, TAG, "Please use vf=pad\n");
	av_exit(1);
}

void opt_frame_crop_right(const char *arg) {
	frame_rightBand = atoi(arg);
	if (frame_rightBand < 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"Incorrect right crop size\n");
		av_exit(1);
	}
	if ((frame_rightBand) >= frame_width) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"Horizontal crop dimensions are outside the range of the original image.\nRemember to crop first and scale second.\n");
		av_exit(1);
	}
	__android_log_print(ANDROID_LOG_ERROR, TAG,
			"-crop* is deprecated in favor of the crop avfilter\n");
	frame_width -= frame_rightBand;
}

void opt_frame_crop_left(const char *arg) {
	frame_leftBand = atoi(arg);
	if (frame_leftBand < 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"Incorrect left crop size\n");
		av_exit(1);
	}
	if ((frame_leftBand) >= frame_width) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"Horizontal crop dimensions are outside the range of the original image.\nRemember to crop first and scale second.\n");
		av_exit(1);
	}
	__android_log_print(ANDROID_LOG_ERROR, TAG,
			"-crop* is deprecated in favor of the crop avfilter\n");
	frame_width -= frame_leftBand;
}

void opt_frame_crop_bottom(const char *arg) {
	frame_bottomBand = atoi(arg);
	if (frame_bottomBand < 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"Incorrect bottom crop size\n");
		av_exit(1);
	}
	if ((frame_bottomBand) >= frame_height) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"Vertical crop dimensions are outside the range of the original image.\nRemember to crop first and scale second.\n");
		av_exit(1);
	}
	__android_log_print(ANDROID_LOG_ERROR, TAG,
			"-crop* is deprecated in favor of the crop avfilter\n");
	frame_height -= frame_bottomBand;
}

void opt_frame_crop_top(const char *arg) {
	frame_topBand = atoi(arg);
	if (frame_topBand < 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"Incorrect top crop size\n");
		av_exit(1);
	}
	if ((frame_topBand) >= frame_height) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"Vertical crop dimensions are outside the range of the original image.\nRemember to crop first and scale second.\n");
		av_exit(1);
	}
	__android_log_print(ANDROID_LOG_ERROR, TAG,
			"-crop* is deprecated in favor of the crop avfilter\n");
	frame_height -= frame_topBand;
}

void opt_frame_pix_fmt(const char *arg) {
	if (strcmp(arg, "list")) {
		frame_pix_fmt = av_get_pix_fmt(arg);
		if (frame_pix_fmt == PIX_FMT_NONE) {
			__android_log_print(ANDROID_LOG_ERROR, TAG,
					"Unknown pixel format requested: %s\n", arg);
			av_exit(1);
		}
	} else {
		show_pix_fmts();
		av_exit(0);
	}
}

void opt_frame_aspect_ratio(const char *arg) {
	int x = 0, y = 0;
	double ar = 0;
	const char *p;
	char *end;

	p = strchr(arg, ':');
	if (p) {
		x = strtol(arg, &end, 10);
		if (end == p)
			y = strtol(end + 1, &end, 10);
		if (x > 0 && y > 0)
			ar = (double) x / (double) y;
	} else
		ar = strtod(arg, NULL);

	if (!ar) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"Incorrect aspect ratio specification.\n");
		av_exit(1);
	}
	frame_aspect_ratio = ar;
}

void opt_frame_size(const char *arg) {
	if (av_parse_video_frame_size(&frame_width, &frame_height, arg) < 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Incorrect frame size\n");
		av_exit(1);
	}
}

int opt_frame_rate(const char *opt, const char *arg) {
	if (av_parse_video_frame_rate(&frame_rate, arg) < 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"Incorrect value for %s: %s\n", opt, arg);
		av_exit(1);
	}
	return 0;
}

int opt_thread_count(const char *opt, const char *arg) {
	thread_count = parse_number_or_die(opt, arg, OPT_INT64, 0, INT_MAX);
#if !HAVE_THREADS
	if (verbose >= 0)
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"Warning: not compiled with thread support, using thread emulation\n");
#endif
	return 0;
}

void opt_format(const char *arg) {
	/* compatibility stuff for pgmyuv */
	if (!strcmp(arg, "pgmyuv")) {
		pgmyuv_compatibility_hack = 1;
		//        opt_image_format(arg);
		arg = "image2";
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"pgmyuv format is deprecated, use image2\n");
	}

	last_asked_format = arg;
}

AVFormatContext *opt_input_file(const char *filename) {
	AVFormatContext *ic;
	AVFormatParameters params, *ap = &params;
	AVInputFormat *file_iformat = NULL;
	int err, i, ret, rfps, rfps_base;
	int64_t timestamp;

	if (last_asked_format) {
		if (!(file_iformat = av_find_input_format(last_asked_format))) {
			__android_log_print(ANDROID_LOG_ERROR, TAG,
					"Unknown input format: '%s'\n", last_asked_format);
			av_exit(1);
		}
		last_asked_format = NULL;
	}

	if (!strcmp(filename, "-"))
		filename = "pipe:";

	using_stdin |= !strncmp(filename, "pipe:", 5)
			|| !strcmp(filename, "/dev/stdin");

	/* get default parameters from command line */
	ic = avformat_alloc_context();
	if (!ic) {
		print_error(filename, AVERROR(ENOMEM));
		av_exit(1);
	}

	memset(ap, 0, sizeof(*ap));
	ap->prealloced_context = 1;
	ap->sample_rate = audio_sample_rate;
	ap->channels = audio_channels;
	ap->time_base.den = frame_rate.num;
	ap->time_base.num = frame_rate.den;
	ap->width = frame_width;
	ap->height = frame_height;
	ap->pix_fmt = frame_pix_fmt;
	// ap->sample_fmt = audio_sample_fmt; //FIXME:not implemented in libavformat
	ap->channel = video_channel;
	ap->standard = video_standard;

	set_context_opts(ic, avformat_opts, AV_OPT_FLAG_DECODING_PARAM);

	ic->video_codec_id = find_codec_or_die(video_codec_name, AVMEDIA_TYPE_VIDEO,
			0, avcodec_opts[AVMEDIA_TYPE_VIDEO]->strict_std_compliance);
	ic->audio_codec_id = find_codec_or_die(audio_codec_name, AVMEDIA_TYPE_AUDIO,
			0, avcodec_opts[AVMEDIA_TYPE_AUDIO]->strict_std_compliance);
	ic->subtitle_codec_id = find_codec_or_die(subtitle_codec_name,
			AVMEDIA_TYPE_SUBTITLE, 0,
			avcodec_opts[AVMEDIA_TYPE_SUBTITLE]->strict_std_compliance);
	ic->flags |= AVFMT_FLAG_NONBLOCK;

	if (pgmyuv_compatibility_hack)
		ic->video_codec_id = CODEC_ID_PGMYUV;

	/* open the input file with generic libav function */
	err = av_open_input_file(&ic, filename, file_iformat, 0, ap);
	if (err < 0) {
		print_error(filename, err);
		av_exit(1);
	}
	if (opt_programid) {
		int i, j;
		int found = 0;
		for (i = 0; i < ic->nb_streams; i++) {
			ic->streams[i]->discard = AVDISCARD_ALL;
		}
		for (i = 0; i < ic->nb_programs; i++) {
			AVProgram *p = ic->programs[i];
			if (p->id != opt_programid) {
				p->discard = AVDISCARD_ALL;
			} else {
				found = 1;
				for (j = 0; j < p->nb_stream_indexes; j++) {
					ic->streams[p->stream_index[j]]->discard =
							AVDISCARD_DEFAULT;
				}
			}
		}
		if (!found) {
			__android_log_print(ANDROID_LOG_ERROR, TAG,
					"Specified program id not found\n");
			av_exit(1);
		}
		opt_programid = 0;
	}

	ic->loop_input = loop_input;

	/* If not enough info to get the stream parameters, we decode the
	 first frames to get it. (used in mpeg case for example) */
	ret = av_find_stream_info(ic);
	if (ret < 0 && verbose >= 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"%s: could not find codec parameters\n", filename);
		av_exit(1);
	}

	timestamp = start_time;
	/* add the stream start time */
	if (ic->start_time != AV_NOPTS_VALUE)
		timestamp += ic->start_time;

	/* if seeking requested, we execute it */
	if (start_time != 0) {
		ret = av_seek_frame(ic, -1, timestamp, AVSEEK_FLAG_BACKWARD);
		if (ret < 0) {
			__android_log_print(ANDROID_LOG_ERROR, TAG,
					"%s: could not seek to position %0.3f\n", filename,
					(double) timestamp / AV_TIME_BASE);
		}
		/* reset seek info */
		start_time = 0;
	}

	/* update the current parameters so that they match the one of the input stream */
	for (i = 0; i < ic->nb_streams; i++) {
		AVStream *st = ic->streams[i];
		AVCodecContext *enc = st->codec;
		avcodec_thread_init(enc, thread_count);
		switch (enc->codec_type) {
		case AVMEDIA_TYPE_AUDIO:
			set_context_opts(enc, avcodec_opts[AVMEDIA_TYPE_AUDIO],
					AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_DECODING_PARAM);
			//__android_log_print(ANDROID_LOG_ERROR, TAG,  "\nInput Audio channels: %d", enc->channels);
			channel_layout = enc->channel_layout;
			audio_channels = enc->channels;
			audio_sample_rate = enc->sample_rate;
			audio_sample_fmt = enc->sample_fmt;
			input_codecs[nb_icodecs++] = avcodec_find_decoder_by_name(
					audio_codec_name);
			if (audio_disable)
				st->discard = AVDISCARD_ALL;
			break;
		case AVMEDIA_TYPE_VIDEO:
			set_context_opts(enc, avcodec_opts[AVMEDIA_TYPE_VIDEO],
					AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_DECODING_PARAM);
			frame_height = enc->height;
			frame_width = enc->width;
			if (ic->streams[i]->sample_aspect_ratio.num)
				frame_aspect_ratio = av_q2d(
						ic->streams[i]->sample_aspect_ratio);
			else
				frame_aspect_ratio = av_q2d(enc->sample_aspect_ratio);
			frame_aspect_ratio *= (float) enc->width / enc->height;
			frame_pix_fmt = enc->pix_fmt;
			rfps = ic->streams[i]->r_frame_rate.num;
			rfps_base = ic->streams[i]->r_frame_rate.den;
			if (enc->lowres) {
				enc->flags |= CODEC_FLAG_EMU_EDGE;
				frame_height >>= enc->lowres;
				frame_width >>= enc->lowres;
			}
			if (me_threshold)
				enc->debug |= FF_DEBUG_MV;

			if (enc->time_base.den != rfps * enc->ticks_per_frame
					|| enc->time_base.num != rfps_base) {

				if (verbose >= 0)
					__android_log_print(ANDROID_LOG_ERROR, TAG,
							"\nSeems stream %d codec frame rate differs from container frame rate: %2.2f (%d/%d) -> %2.2f (%d/%d)\n",
							i, (float) enc->time_base.den / enc->time_base.num,
							enc->time_base.den, enc->time_base.num,

							(float) rfps / rfps_base, rfps, rfps_base);
			}
			/* update the current frame rate to match the stream frame rate */
			frame_rate.num = rfps;
			frame_rate.den = rfps_base;

			input_codecs[nb_icodecs++] = avcodec_find_decoder_by_name(
					video_codec_name);
			if (video_disable)
				st->discard = AVDISCARD_ALL;
			else if (video_discard)
				st->discard = video_discard;
			break;
		case AVMEDIA_TYPE_DATA:
			break;
		case AVMEDIA_TYPE_SUBTITLE:
			input_codecs[nb_icodecs++] = avcodec_find_decoder_by_name(
					subtitle_codec_name);
			if (subtitle_disable)
				st->discard = AVDISCARD_ALL;
			break;
		case AVMEDIA_TYPE_ATTACHMENT:
		case AVMEDIA_TYPE_UNKNOWN:
			nb_icodecs++;
			break;
		default:
			abort();
		}
	}

	input_files[nb_input_files] = ic;
	input_files_ts_offset[nb_input_files] = input_ts_offset
			- (copy_ts ? 0 : timestamp);
	/* dump the file content */
	if (verbose >= 0)
		dump_format(ic, nb_input_files, filename, 0);

	nb_input_files++;

	video_channel = 0;

	av_freep(&video_codec_name);
	av_freep(&audio_codec_name);
	av_freep(&subtitle_codec_name);
	return ic;
}

void opt_map(const char *arg) {
	AVStreamMap *m;
	char *p;

	m = &stream_maps[nb_stream_maps++];

	m->file_index = strtol(arg, &p, 0);
	if (*p)
		p++;

	m->stream_index = strtol(p, &p, 0);
	if (*p) {
		p++;
		m->sync_file_index = strtol(p, &p, 0);
		if (*p)
			p++;
		m->sync_stream_index = strtol(p, &p, 0);
	} else {
		m->sync_file_index = m->file_index;
		m->sync_stream_index = m->stream_index;
	}
}

void opt_map_meta_data(const char *arg) {
	AVMetaDataMap *m;
	char *p;

	m = &meta_data_maps[nb_meta_data_maps++];

	m->out_file = strtol(arg, &p, 0);
	if (*p)
		p++;

	m->in_file = strtol(p, &p, 0);
}

int opt_recording_time(const char *opt, const char *arg) {
	recording_time = parse_time_or_die(opt, arg, 1);
	return 0;
}

int opt_start_time(const char *opt, const char *arg) {
	start_time = parse_time_or_die(opt, arg, 1);
	return 0;
}

int opt_input_ts_offset(const char *opt, const char *arg) {
	input_ts_offset = parse_time_or_die(opt, arg, 1);
	return 0;
}

void opt_input_ts_scale(const char *arg) {
	unsigned int stream;
	double scale;
	char *p;

	stream = strtol(arg, &p, 0);
	if (*p)
		p++;
	scale = strtod(p, &p);

	if (stream >= MAX_STREAMS)
		av_exit(1);

	input_files_ts_scale[nb_input_files][stream] = scale;
}

int opt_rec_timestamp(const char *opt, const char *arg) {
	rec_timestamp = parse_time_or_die(opt, arg, 0) / 1000000;
	return 0;
}

int opt_metadata(const char *opt, const char *arg) {
	char *mid = strchr(arg, '=');

	if (!mid) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Missing =\n");
		av_exit(1);
	}
	*mid++ = 0;

	metadata_count++;
	metadata = av_realloc(metadata, sizeof(*metadata) * metadata_count);
	metadata[metadata_count - 1].key = av_strdup(arg);
	metadata[metadata_count - 1].value = av_strdup(mid);

	return 0;
}

int opt_timelimit(const char *opt, const char *arg) {
#if HAVE_SETRLIMIT
	int lim = parse_number_or_die(opt, arg, OPT_INT64, 0, INT_MAX);
	struct rlimit rl = {lim, lim + 1};
	if (setrlimit(RLIMIT_CPU, &rl))
	perror("setrlimit");
#else
	fprintf(stderr, "Warning: -%s not implemented on this OS\n", opt);
#endif
	return 0;
}

int opt_verbose(const char *opt, const char *arg) {
	verbose = parse_number_or_die(opt, arg, OPT_INT64, -10, 10);
	return 0;
}

void opt_target(const char *arg) {
	enum {
		PAL, NTSC, FILM, UNKNOWN
	} norm = UNKNOWN;
	static const char * const frame_rates[] =
			{ "25", "30000/1001", "24000/1001" };

	if (!strncmp(arg, "pal-", 4)) {
		norm = PAL;
		arg += 4;
	} else if (!strncmp(arg, "ntsc-", 5)) {
		norm = NTSC;
		arg += 5;
	} else if (!strncmp(arg, "film-", 5)) {
		norm = FILM;
		arg += 5;
	} else {
		int fr;
		/* Calculate FR via float to avoid int overflow */
		fr = (int) (frame_rate.num * 1000.0 / frame_rate.den);
		if (fr == 25000) {
			norm = PAL;
		} else if ((fr == 29970) || (fr == 23976)) {
			norm = NTSC;
		} else {
			/* Try to determine PAL/NTSC by peeking in the input files */
			if (nb_input_files) {
				int i, j;
				for (j = 0; j < nb_input_files; j++) {
					for (i = 0; i < input_files[j]->nb_streams; i++) {
						AVCodecContext *c = input_files[j]->streams[i]->codec;
						if (c->codec_type != AVMEDIA_TYPE_VIDEO)
							continue;
						fr = c->time_base.den * 1000 / c->time_base.num;
						if (fr == 25000) {
							norm = PAL;
							break;
						} else if ((fr == 29970) || (fr == 23976)) {
							norm = NTSC;
							break;
						}
					}
					if (norm != UNKNOWN)
						break;
				}
			}
		}
		if (verbose && norm != UNKNOWN)
			__android_log_print(ANDROID_LOG_ERROR, TAG,
					"Assuming %s for target.\n", norm == PAL ? "PAL" : "NTSC");
	}

	if (norm == UNKNOWN) {
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"Could not determine norm (PAL/NTSC/NTSC-Film) for target.\n");
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"Please prefix target with \"pal-\", \"ntsc-\" or \"film-\",\n");
		__android_log_print(ANDROID_LOG_ERROR, TAG,
				"or set a framerate with \"-r xxx\".\n");
		av_exit(1);
	}

	if (!strcmp(arg, "vcd")) {

		opt_video_codec("mpeg1video");
		opt_audio_codec("mp2");
		opt_format("vcd");

		opt_frame_size(norm == PAL ? "352x288" : "352x240");
		opt_frame_rate(NULL, frame_rates[norm]);
		opt_default("g", norm == PAL ? "15" : "18");

		opt_default("b", "1150000");
		opt_default("maxrate", "1150000");
		opt_default("minrate", "1150000");
		opt_default("bufsize", "327680"); // 40*1024*8;

		opt_default("ab", "224000");
		audio_sample_rate = 44100;
		audio_channels = 2;

		opt_default("packetsize", "2324");
		opt_default("muxrate", "1411200"); // 2352 * 75 * 8;

		/* We have to offset the PTS, so that it is consistent with the SCR.
		 SCR starts at 36000, but the first two packs contain only padding
		 and the first pack from the other stream, respectively, may also have
		 been written before.
		 So the real data starts at SCR 36000+3*1200. */
		mux_preload = (36000 + 3 * 1200) / 90000.0; //0.44
	} else if (!strcmp(arg, "svcd")) {

		opt_video_codec("mpeg2video");
		opt_audio_codec("mp2");
		opt_format("svcd");

		opt_frame_size(norm == PAL ? "480x576" : "480x480");
		opt_frame_rate(NULL, frame_rates[norm]);
		opt_default("g", norm == PAL ? "15" : "18");

		opt_default("b", "2040000");
		opt_default("maxrate", "2516000");
		opt_default("minrate", "0"); //1145000;
		opt_default("bufsize", "1835008"); //224*1024*8;
		opt_default("flags", "+scan_offset");

		opt_default("ab", "224000");
		audio_sample_rate = 44100;

		opt_default("packetsize", "2324");

	} else if (!strcmp(arg, "dvd")) {

		opt_video_codec("mpeg2video");
		opt_audio_codec("ac3");
		opt_format("dvd");

		opt_frame_size(norm == PAL ? "720x576" : "720x480");
		opt_frame_rate(NULL, frame_rates[norm]);
		opt_default("g", norm == PAL ? "15" : "18");

		opt_default("b", "6000000");
		opt_default("maxrate", "9000000");
		opt_default("minrate", "0"); //1500000;
		opt_default("bufsize", "1835008"); //224*1024*8;

		opt_default("packetsize", "2048"); // from www.mpucoder.com: DVD sectors contain 2048 bytes of data, this is also the size of one pack.
		opt_default("muxrate", "10080000"); // from mplex project: data_rate = 1260000. mux_rate = data_rate * 8

		opt_default("ab", "448000");
		audio_sample_rate = 48000;

	} else if (!strncmp(arg, "dv", 2)) {

		opt_format("dv");

		opt_frame_size(norm == PAL ? "720x576" : "720x480");
		opt_frame_pix_fmt(
				!strncmp(arg, "dv50", 4) ?
						"yuv422p" : (norm == PAL ? "yuv420p" : "yuv411p"));
		opt_frame_rate(NULL, frame_rates[norm]);

		audio_sample_rate = 48000;
		audio_channels = 2;

	} else {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Unknown target: %s\n",
				arg);
		av_exit(1);
	}
}
