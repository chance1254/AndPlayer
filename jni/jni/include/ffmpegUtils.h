#ifndef FFMPEGUTILS_H_
#define FFMPEGUTILS_H_
#define MAX_FILES 100
AVFormatContext *avformat_opts;
char *video_standard;
char *audio_codec_name = NULL;
char *video_codec_name = NULL;

int av_exit(int code);

class JNIHelper
{
public:
	JNIHelper();
	~JNIHelper();
	void opt_codec(int *pstream_copy, char **pcodec_name,
	                      int codec_type, const char *arg);
	int decode_interrupt_cb(void);
	int read_key(void);
};
#endif


