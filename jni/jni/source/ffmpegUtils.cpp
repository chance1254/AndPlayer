#include "../include/jniUtils.h"
#include <android/log.h>
extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
}
#include "../include/ffmpegUtils.h"
#define TAG "ffmpegUtils"
static int nb_output_files = 0;
static AVFormatContext *output_files[MAX_FILES];
static AVFormatContext *input_files[MAX_FILES];
static uint16_t *intra_matrix = NULL;
static uint16_t *inter_matrix = NULL;
static int nb_input_files = 0;
static volatile int received_sigterm = 0;
static short *samples;
unsigned int allocated_audio_out_size, allocated_audio_buf_size;
static uint8_t *audio_out;
static uint8_t *audio_buf;
static AVCodecContext *avcodec_opts[AVMEDIA_TYPE_NB];
static const char **opt_names;
static char *vstats_filename;
static FILE *vstats_file;
static int q_pressed = 0;
struct SwsContext *sws_opts;

int av_exit(int code) {
	int av_file;

	for (av_file = 0; av_file < nb_output_files; ++av_file) {
		AVFormatContext *context = output_files[av_file];
		int stream;
		if (!(context->oformat->flags & AVFMT_NOFILE) && context->pb) {
			avio_close(context->pb);
		}

		for (stream = 0; stream < context->nb_streams; ++stream) {
			avformat_free_context(context);
			av_free(context->streams[stream]->codec);
			av_free(context->streams[stream]);
		}

		for (stream = 0; stream < context->nb_programs; ++stream) {
			avformat_free_context(context);
		}

		for (stream = 0; stream < context->nb_chapters; ++stream) {
			avformat_free_context(context);
		}
		avformat_free_context(context);
		av_free(&context);
	}

	for (av_file = 0; av_file < nb_input_files; ++av_file) {
		av_close_input_file(input_files[av_file]);
	}

	av_free(intra_matrix);
	av_free(inter_matrix);

	if (vstats_file)
		fclose(vstats_file);
	av_free(vstats_filename);

	av_free(opt_names);

	av_free(video_codec_name);
	av_free(audio_codec_name);

	av_free(video_standard);

#if CONFIG_POWERPC_PERF
	void powerpc_display_perf_report(void);
	powerpc_display_perf_report();
#endif /* CONFIG_POWERPC_PERF */

	for (av_file = 0; av_file < AVMEDIA_TYPE_NB; ++av_file)
		av_free(avcodec_opts[av_file]);
	av_free(avformat_opts);
	av_free(sws_opts);
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

	exit(code); /* not all OS-es handle main() return value */
	return code;
}

JNIHelper::JNIHelper() {
}

JNIHelper::~JNIHelper() {
}

void JNIHelper::opt_codec(int* pstream_copy, char** codec_name, int codec_type,
		const char* arg) {
	av_freep(codec_name);
	if (!strcmp(arg, "copy")) {
		*pstream_copy = 1;
	} else {
		*codec_name = av_strdup(arg);
	}
}

int JNIHelper::read_key() {
#if HAVE_TERMIOS_H
	int n = 1;
	unsigned char ch;
	struct timeval tv;
	fd_set rfds;

	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	n = select(1, &rfds, NULL, NULL, &tv);
	if (n > 0) {
		n = read(0, &ch, 1);
		if (n == 1)
		return ch;

		return n;
	}
#elif HAVE_CONIO_H
	if(kbhit())
	return(getch());
#endif
	return -1;

}

int JNIHelper::decode_interrupt_cb() {
	return q_pressed || (q_pressed = this->read_key() == 'q');
}

