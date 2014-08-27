#ifndef _FFMPEG_VIDEO_OUTPUT_H_
#define _FFMPEG_VIDEO_OUTPUT_H_

extern "C" {
	
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

}

#include "thread.h"
#include "output.h"
#include "time_source.h"

class MediaPlayer;

namespace ffmpeg {

#define VIDEO_PICTURE_QUEUE_SIZE 4

class VideoOutput : public Thread, TimeSource
{
public:
	struct VideoPicture
	{
	    double duration;
		double pts;                                  ///<presentation time stamp for this picture
		int64_t pos;	                             ///<byte position in file
		bool     skip;
		AVPicture frame;
	};

	VideoOutput(MediaPlayer* mediaPlayer);
	virtual ~VideoOutput();
	
	virtual int     open(enum PixelFormat pix_fmt, int width, int height, int rotate);	
	virtual int     setSurface(void* env, void* surface);	
	virtual int 	queueVideoFrame(AVFrame* frame, double pts, int64_t pos, double duration);
	virtual int     close();
	virtual void	pause();
	virtual void	resume();
	virtual void	flush();
	
    virtual double getRealTime();
    
	inline  double getFrameLastPTS() { return mFrameLastPTS; }
	inline void setVideoFrameDrop(int level) { mFrameDrop = level; }
protected:
	virtual void    handleRun(void* ptr);

private:
	void			displayVideoFrame(AVPicture* frame);
	void 			rendererLoop();	
	
private:
	MediaPlayer* mMediaPlayer;
	int			 mVideoWidth;
	int			 mVideoHeight;
	int			 mVideoRotate;
	enum PixelFormat	 mVideoFormat;
	
	VideoPicture mPictureQueue[VIDEO_PICTURE_QUEUE_SIZE];
	int			 mPictureQueueSize;
	int			 mPictureQueueReadIndex;
	int			 mPictureQueueWriteIndex;
	Mutex		 mPictureQueueMutex;	
	Condition	 mPictureQueueCondition;
	
	int		 	 mFrameDrop;
	int64_t		 mFrameDropCount;
	double       mFrameTimer;
	double       mFrameLastDuration;
	double       mFrameLastPTS;
	double		 mVideoCurrentPTS;
	double		 mVideoCurrentPTSDrift;
	int64_t		 mVideoCurrentPos;	
	
	VIDEO_DRIVER_HANDLE mVideoDriverHandle;
};

}

#endif //_FFMPEG_VIDEO_OUTPUT_H_
