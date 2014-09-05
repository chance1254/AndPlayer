#include "com_v2soft_spoiq_ffmpeg_core_FFMPEGUtils.h"
#include <android/log.h>
#include "jniUtils.h"
#include "methods.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

#define TAG "FFMpegUtils"

struct fields_t {
	jfieldID    surface;
	jmethodID   clb_onVideoFrame;
};
static struct fields_t fields;

static const char *framesOutput = NULL;

jclass FFMpegUtils_getClass(JNIEnv *env) {
	return (*env)->FindClass(env, "com/v2soft/spoiq/ffmpeg/core/FFMPEGUtils");
}

const char *FFMpegUtils_getSignature() {
	return "Lcom/v2soft/spoiq/ffmpeg/core/FFMPEGUtils;";
}

static void save_frame(AVFrame *pFrame, int width, int height, int iFrame)
{
        FILE *pFile;
    	char szFilename[256];
    	int  y;

    	// Open file
    	if(framesOutput == NULL) {
    		sprintf(szFilename, "/sdcard/frame%d.ppm", iFrame);
    	} else {
    		sprintf(szFilename, "%s/frame%d.ppm", framesOutput, iFrame);
    	}

    	pFile=fopen(szFilename, "wb");
    	if(pFile==NULL)
    		return;

    	// Write header
    	fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    	// Write pixel data
    	for(y=0; y<height; y++)
    		fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width * 2, pFile);

    	// Close file
    	fclose(pFile);
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGUtils_native_1av_1release
  (JNIEnv *env, jobject thiz)
{
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGUtils_native_1av_1setOutput
  (JNIEnv *env, jobject thiz, jstring path)
{
    framesOutput = (*env)->GetStringUTFChars(env,path, NULL);
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGUtils_native_1av_1print
  (JNIEnv *env, jobject thiz, jint pAVFormatContext)
{
    int i;
	AVCodecContext *pCodecCtx;
	AVFrame *pFrame;
	AVCodec *pCodec;
	AVFormatContext *pFormatCtx = (AVFormatContext *) pAVFormatContext;
	struct SwsContext *img_convert_ctx;

	__android_log_print(ANDROID_LOG_INFO, TAG, "playing");
    int videoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	if (videoStream == -1) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Didn't find a video stream");
		return;
	}

// Get a pointer to the codec context for the video stream
	pCodecCtx = pFormatCtx->streams[videoStream]->codec;

	// Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Unsupported codec!");
		return; // Codec not found
	}
	// Open codec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Could not open codec");
		return; // Could not open codec
	}

	// Allocate video frame
	pFrame = avcodec_alloc_frame();

	// Allocate an AVFrame structure
	AVFrame *pFrameRGB = avcodec_alloc_frame();
	if (pFrameRGB == NULL) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Could allocate an AVFrame structure");
		return;
	}

	uint8_t *buffer;
	int numBytes;
	// Determine required buffer size and allocate buffer
	numBytes = avpicture_get_size(PIX_FMT_RGB565, pCodecCtx->width,
			pCodecCtx->height);
	buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *) pFrameRGB, buffer, PIX_FMT_RGB565,
			pCodecCtx->width, pCodecCtx->height);

	int w = pCodecCtx->width;
	int h = pCodecCtx->height;
	img_convert_ctx = sws_getContext(w, h, pCodecCtx->pix_fmt, w, h,
			PIX_FMT_RGB565, SWS_BICUBIC, NULL, NULL, NULL);

	int frameFinished;
	AVPacket packet;

	i = 0;
	int result = -1;
	while ((result = av_read_frame(pFormatCtx, &packet)) >= 0) {
		// Is this a packet from the video stream?
		if (packet.stream_index == videoStream) {
			// Decode video frame
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,
					&packet);

			// Did we get a video frame?
			if (frameFinished) {
				// Convert the image from its native format to RGB
				sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0,
						pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

				//FFMpegUtils_saveFrame(pFrameRGB, pCodecCtx->width,
				//		pCodecCtx->height, i);
				handleOnVideoFrame(env, thiz, pFrame, pCodecCtx->width,
						pCodecCtx->height);
				i++;
			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}

// Free the RGB image
	av_free(buffer);
	av_free(pFrameRGB);

	// Free the YUV frame
	av_free(pFrame);

	// Close the codec
	avcodec_close(pCodecCtx);

	// Close the video file
	av_close_input_file(pFormatCtx);

	__android_log_print(ANDROID_LOG_INFO, TAG, "end of playing");
}

void handleOnVideoFrame(JNIEnv *env, jobject object, AVFrame *pFrame, int width, int height)
{
    int size = width * 2;//) * pFrame->linesize[0];
	//__android_log_print(ANDROID_LOG_INFO, TAG, "width: %i, height: %i, linesize: %i", width, height, pFrame->linesize[0]);
	jintArray arr = (*env)->NewIntArray(env,size);
	(*env)->SetIntArrayRegion(env,arr, 0, size, (jint *) pFrame->data[0] /* + y *pFrame->linesize[0]*/);
    (*env)->CallVoidMethod(env,object, fields.clb_onVideoFrame, arr);
}

JNIEXPORT jobject JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGUtils_native_1av_1setInputFile
  (JNIEnv *env, jobject thiz, jstring filePath)
{
    AVFormatContext *pFormatCtx;
	const char *_filePath = (*env)->GetStringUTFChars(env,filePath, NULL);
	// Open video file
	if(avformat_open_input(&pFormatCtx, _filePath, NULL, NULL) != 0) {
		jniThrowException(env,
						  "java/io/IOException",
					      "Can't create input file");
	}
	// Retrieve stream information
	if(av_find_stream_info(pFormatCtx)<0) {
		jniThrowException(env,
						  "java/io/IOException",
						  "Couldn't find stream information");
	}
	return AVFormatContext_create(env, pFormatCtx);
}
