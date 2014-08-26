#ifndef VIDEO_OUTPUT_H_
#define VIDEO_OUTPUT_H_

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

}

#include "thread.h"
#include "time_source.h"
typedef void* VIDEO_DRIVER_HANDLE;
class MediaPlayer;
namespace ffmpeg {
#define VIDEO_PICTURE_QUEUE_SIZE 4
class VideoOutput : public Thread, TimeSource
{
public:
	struct VideoPicture {
		double duration;
		double pts;
		int64_t pos;
		bool skip;
		AVPicture frame;
	};

	VideoOutput(MediaPlayer* player);
	virtual ~VideoOutput();
	virtual int     	open(enum PixelFormat pix_fmt, int width, int height);
	virtual int     	setSurface(void* surface);
	virtual int     	queueVideoFrame(AVFrame* frame, double pts, int64_t pos, double duration);
	virtual int     	close();
	virtual void    	pause();
	virtual void    	resume();
	virtual void    	flush();
	virtual double  	getRealTime();
	inline  double  	getFrameLastPTS() { return frameLastPTS; }
	inline void     	setVideoFrameDrop(int level) { frameDrop = level; }
protected:
	virtual void    	handleRun(void* ptr);
private:
	void            	displayVideoFrame(AVPicture* frame);
	void				rendererLoop();
	MediaPlayer* 		player;
	int 				videoWidth;
	int					videoHeight;
	enum PixelFormat 	format;
	VideoPicture		picture[VIDEO_PICTURE_QUEUE_SIZE];
	int                 pictureQueueSize;
	int                 pictureQueueReadIndex;
	int                 pictureQueueWriteIndex;
	pthread_mutex_t		pictureQueueMutex;
	pthread_cond_t		pictureQueueCondition;
	int 				frameDrop;
	int64_t				frameDropCount;
	double				frameTimer;
	double				frameLastDuration;
	double 				frameLastPTS;
	double				videoCurrentPTS;
	double     			videoCurrentPTSDrift;
	int64_t         	videoCurrentPos;
	VIDEO_DRIVER_HANDLE videoDriverHandle;
};
}
#endif /* VIDEO_OUTPUT_H_ */
