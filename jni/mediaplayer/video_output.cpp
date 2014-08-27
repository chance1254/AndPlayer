
#define TAG "FFMpegVideoOutput"

extern "C" {
	
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavutil/log.h"
#include "libavutil/time.h"
#include "../yuv2rgb/include/yuv2rgb.h"
	
} // end of extern C

#include "output.h"
#include "video_output.h"
#include "mediaplayer.h"

#define FRAME_SKIP_FACTOR 0.05

namespace ffmpeg {

static void rotate(int angle, unsigned char* dst,unsigned char* src,int dststride,int srcstride,int w,int h){
    int y;
	
	switch(angle) {
		case 270:
			dst+=dststride*(h-1);
			dststride*=-1;
			for(y=0;y<h;y++){
				int x;
				for(x=0;x<w;x++) dst[x]=src[y+x*srcstride];
				dst+=dststride;
			}
			break;
		case 90:
			src+=srcstride*(w-1);
			srcstride*=-1;
			for(y=0;y<h;y++){
				int x;
				for(x=0;x<w;x++) dst[x]=src[y+x*srcstride];
				dst+=dststride;
			}
			break;
		case 180:
			dst+=dststride*(h-1);
			dststride*=-1;
			for(y=0;y<h;y++){
				int x;
				for(x=0;x<w;x++) dst[x]=src[w-1-x+y*srcstride];
				dst+=dststride;
			}
			break;
		default:
			assert(0);
    }
}

VideoOutput::VideoOutput(MediaPlayer* mediaPlayer)
		: mMediaPlayer(mediaPlayer)
		, mVideoWidth(0), mVideoHeight(0), mVideoRotate(0), mVideoFormat(PIX_FMT_NONE)
		, mPictureQueueSize(0), mPictureQueueReadIndex(0), mPictureQueueWriteIndex(0)
		, mFrameDrop(mediaPlayer->videoFrameDrop()), mFrameDropCount(0)
	    , mFrameTimer(0), mFrameLastDuration(0), mFrameLastPTS(0)
		, mVideoCurrentPTS(0), mVideoCurrentPTSDrift(0), mVideoCurrentPos(0)
		, mVideoDriverHandle(NULL)
{
	for(int i = 0; i < VIDEO_PICTURE_QUEUE_SIZE; i++) {
		memset(&mPictureQueue[i], 0, sizeof(mPictureQueue[i]));
	}
}

VideoOutput::~VideoOutput()
{
    for(int i = 0; i < VIDEO_PICTURE_QUEUE_SIZE; i++) {
        AVPicture* frame = &mPictureQueue[i].frame;
        if (frame->data[0]) {
            avpicture_free(frame);
            memset(frame, 0, sizeof(AVPicture));
        }
	}
}

int VideoOutput::open(enum PixelFormat pix_fmt, int width, int height, int rotate)
{
    mVideoFormat = pix_fmt;
    mVideoWidth  = width;
	mVideoHeight = height;
	mVideoRotate = rotate;
    Output::VideoDriver_open(&mVideoDriverHandle);
    
	startAsync();
    return 0;
}

int VideoOutput::setSurface(void* env, void* surface)
{
    return Output::VideoDriver_setSurface(mVideoDriverHandle, env, surface);
}

int VideoOutput::close()
{
    Thread::stop();
	mPictureQueueMutex.lock();
    mPictureQueueCondition.broadcast();
	mPictureQueueMutex.unlock();

    LOGI("waiting on end of video output thread");
    int ret = -1;
    if((ret = wait()) != 0) {
        LOGI("Couldn't cancel video output thread: %i", ret);
    }    
    Output::VideoDriver_close(mVideoDriverHandle);
    mVideoDriverHandle = NULL;

    return ret;    
}

int VideoOutput::queueVideoFrame(AVFrame* frame, double pts, int64_t pos, double duration)
{
#if 0
	displayVideoFrame((AVPicture*)frame);
	
	return 0;
#else
	VideoPicture *vp;

    /* wait until we have space to put a new picture */
	mPictureQueueMutex.lock();
    while (mPictureQueueSize >= VIDEO_PICTURE_QUEUE_SIZE && mRunning) {
		mPictureQueueCondition.wait(mPictureQueueMutex);
    }
	mPictureQueueMutex.unlock();

	if (!mRunning)
		return -1;

	vp = &mPictureQueue[mPictureQueueWriteIndex];
	if (vp->frame.data[0] == NULL) {
	    //int err = avpicture_alloc(&vp->frame, mVideoFormat, mVideoWidth, mVideoHeight);
	    int err = av_image_alloc(vp->frame.data, vp->frame.linesize, mVideoWidth, mVideoHeight, mVideoFormat, 16);	    
        if (err < 0) {
            LOGE("!!!OOM!!! err %d, (%d, %d, %d)", err, mVideoFormat, mVideoWidth, mVideoHeight);
            return err;
        }
	}
	if (mVideoRotate != 0) {
		assert(PIX_FMT_YUV420P == mVideoFormat);
		AVPicture* pic = (AVPicture*)frame;
		rotate(mVideoRotate, vp->frame.data[0], pic->data[0],
               vp->frame.linesize[0], pic->linesize[0],
               mVideoWidth, mVideoHeight);
		rotate(mVideoRotate, vp->frame.data[1], pic->data[1],
               vp->frame.linesize[1], pic->linesize[1],
               mVideoWidth / 2, mVideoHeight / 2);
		rotate(mVideoRotate, vp->frame.data[2], pic->data[2],
               vp->frame.linesize[1], pic->linesize[1],
               mVideoWidth / 2, mVideoHeight / 2);
	} else {
		av_picture_copy(&vp->frame, (AVPicture*) frame, mVideoFormat, mVideoWidth, mVideoHeight);	
	}	
	vp->pts = pts;	
	vp->pos = pos;
	vp->duration = duration;
	vp->skip = false;
	//LOGI("%s: pts %f, duration %f\n", __FUNCTION__, pts, duration);
	
	mPictureQueueMutex.lock();
	if (++mPictureQueueWriteIndex == VIDEO_PICTURE_QUEUE_SIZE)
		mPictureQueueWriteIndex = 0;
    mPictureQueueSize++;
	mPictureQueueCondition.broadcast();
	mPictureQueueMutex.unlock();
	
	return 0;
#endif	
}

void VideoOutput::pause()
{
    Thread::pause();
}

void VideoOutput::resume()
{
    Thread::resume();        
}

void VideoOutput::flush()
{
	mPictureQueueMutex.lock();
	for(int i = 0; i < VIDEO_PICTURE_QUEUE_SIZE; i++) {
		VideoPicture *vp = &mPictureQueue[i];
		vp->skip = true;
	}
	while (mPictureQueueSize > 0 && mRunning && !mPausing) {
		//LOGI("VideoOutput::flush1() %d\n", mPictureQueueSize);
		mPictureQueueCondition.wait(mPictureQueueMutex);
		//LOGI("VideoOutput::flush2() %d\n", mPictureQueueSize);
	}
	mVideoCurrentPos = -1;
	mFrameLastPTS = AV_NOPTS_VALUE;
	mFrameLastDuration = 0;
	mFrameTimer = (double)av_gettime() / 1000000.0;
	mPictureQueueMutex.unlock();	
}

double VideoOutput::getRealTime()
{
    if (mPausing)
    {
        return mVideoCurrentPTS;
    } else {
        return mVideoCurrentPTSDrift + av_gettime() * 1e-6;
    }
}

void VideoOutput::handleRun(void* ptr)
{
	rendererLoop();
}

void VideoOutput::displayVideoFrame(AVPicture* frame)
{
	int w, h, pixelformat, stride;
	uint8_t* pixels;
		
#ifdef FPS_DEBUGGING
	{
		timeval pTime;
		static int frames = 0;
		static double t1 = -1;
		static double t2 = -1;

		gettimeofday(&pTime, NULL);
		t2 = pTime.tv_sec + (pTime.tv_usec / 1000000.0);
		if (t1 == -1 || t2 > t1 + 1) {
			LOGI("Video fps:%i", frames);			
			t1 = t2;
			frames = 0;
		}
		frames++;
	}
#endif	
	
	if (Output::VideoDriver_lockSurface(mVideoDriverHandle, &w, &h, &pixelformat, &stride, (void**)&pixels) != ANDROID_SURFACE_RESULT_SUCCESS) {
		return;
	}
	if (mVideoWidth > w || mVideoHeight > h) {
	    LOGE("unsupported video scale");
	    return;
	}
	
	if (   mVideoFormat == PIX_FMT_YUV420P
		&& pixelformat == 0x32315659) {//optimized for ImageFormat.YV12
		int y_height_stride = ((h + 0x01) & ~0x01);
		int y_size = stride * y_height_stride;
		int c_stride = ((stride / 2 + 15) & ~15);
		int c_size = (y_height_stride / 2) * c_stride;
		uint8_t *dst[4] = {pixels, pixels + y_size + c_size, pixels + y_size, 0};
		int dstStride[4] = {stride, c_stride, c_stride, 0};
		av_image_copy(dst, dstStride,
                    (const uint8_t**)frame->data, frame->linesize,
                    PIX_FMT_YUV420P, mVideoWidth, mVideoHeight);		    
    } else if ( mVideoFormat == PIX_FMT_YUV420P
		     && pixelformat == 4) { //RGB_565 optimized for yuv2rgb
		static int dither;
		yuv420_2_rgb565(pixels, 
							frame->data[0],										
							frame->data[1], 
							frame->data[2],
							mVideoWidth,
							mVideoHeight,
							frame->linesize[0],  // Y span
							frame->linesize[1],  // UV span Width / 2 
							(stride << 1),  //Dest stride * bpp (stride * 2bytes)
							yuv2rgb565_table, 
							dither++);					
    } else if ( mVideoFormat == PIX_FMT_YUV420P
		    && (pixelformat == 2 || pixelformat == 1)) { //RGBX_8888, RGBA_8888, optimized for yuv2rgb
		static int dither;
		yuv420_2_rgb8888(pixels, 
							frame->data[0], 
							frame->data[1], 
							frame->data[2],
							mVideoWidth,
							mVideoHeight,
							frame->linesize[0],  // Y span
							frame->linesize[1],  // UV span Width / 2 
							(stride << 2),  //Dest stride * bpp (stride * 4bytes)
							yuv2rgb565_table,
							dither++);
    } else if (mVideoFormat == PIX_FMT_YUV420P
		&& pixelformat == 3) { //RGB_888 optimized for yuv2rgb
		static int dither;
		yuv420_2_rgb888(pixels, 
							frame->data[0], 
							frame->data[1], 
							frame->data[2],
							mVideoWidth,
							mVideoHeight,
							frame->linesize[0],  // Y span
							frame->linesize[1],  // UV span Width / 2 
							(stride * 3),  //Dest stride * bpp (stride * 3bytes)
							yuv2rgb565_table, 
							dither++);																			
	} else {
		LOGE("unsupported video format convert");
	}
	Output::VideoDriver_unlockSurface(mVideoDriverHandle);
}

void VideoOutput::rendererLoop()
{	
	VideoPicture *vp;
	double time, next_target;
    double last_duration, duration, delay;

    mFrameTimer = av_gettime() / 1000000.0;    
	while(mRunning) {
	    //LOGI("rendererLoop: %d, running %d\n", __LINE__, mRunning);
		if (mPausing) {
			this->delay(10);
			continue;
		}
		/* wait until we have a new picture */
		mPictureQueueMutex.lock();
		while (mPictureQueueSize == 0 && mRunning) {
		    //LOGI("rendererLoop: %d, running %d\n", __LINE__, mRunning);
			mPictureQueueCondition.wait(mPictureQueueMutex);
			//LOGI("rendererLoop: %d, running %d\n", __LINE__, mRunning);
		}
		mPictureQueueMutex.unlock();
		if (!mRunning) {
			break;
		}

        /* dequeue the picture */
        vp = &mPictureQueue[mPictureQueueReadIndex];
		if(vp->skip) {
			mPictureQueueMutex.lock();
			/* update queue size and signal for next picture */
            if (++mPictureQueueReadIndex == VIDEO_PICTURE_QUEUE_SIZE)
                mPictureQueueReadIndex = 0;
			mPictureQueueSize--;
			mPictureQueueCondition.broadcast();
			mPictureQueueMutex.unlock();                
            continue;
        }
        
        /* compute nominal last_duration */
        last_duration = vp->pts - mFrameLastPTS;
        if (last_duration > 0 && last_duration < 10.0) {
            /* if duration of the last frame was sane, update last_duration in video state */
            mFrameLastDuration = last_duration;
        }
        delay = mMediaPlayer->computeTargetDelay(mFrameLastDuration);
         		
		time = av_gettime() / 1000000.0;   
        if((time) < mFrameTimer + delay) {
			//LOGI("%s: time %f, frame_timer %f delay %f\n", __FUNCTION__, time, mFrameTimer, delay);
			this->delay(FFMIN((int)(mFrameTimer + delay - time) * 500, 10));
			continue;
		}
		if (delay > 0)
		    mFrameTimer += delay * FFMAX(1, floor((time - mFrameTimer) / delay));
		
		/* update current video pts */
		mVideoCurrentPTS = vp->pts;
		mVideoCurrentPTSDrift = mVideoCurrentPTS - time;
        mVideoCurrentPos = vp->pos;
        mFrameLastPTS = vp->pts;
		
        if(mPictureQueueSize > 1){
            VideoPicture* nextvp = &mPictureQueue[(mPictureQueueReadIndex + 1) % VIDEO_PICTURE_QUEUE_SIZE];
            assert(nextvp->pts >= vp->pts);
            duration = nextvp->pts - vp->pts;
        } else {
            duration = vp->duration;
        }
        //LOGI("%s: time %f, frame_timer %f, duration %f, pts %f\n", __FUNCTION__, time, mFrameTimer, duration, vp->pts);
        
		if((mFrameDrop >= AV_DROP_LEVEL_DROP_FRAME) && time > (mFrameTimer + duration)) {
			static int dropCount = 0;
            if((dropCount++ % AV_VIDEO_FRAME_DROP_COUNT) != 0) {
				mFrameDropCount++;
                //LOGI("%s: dropframe: time %f, frame_timer %f pts %f (%lld)\n", __FUNCTION__, time, mFrameTimer, vp->pts, mFrameDropCount);
				mPictureQueueMutex.lock();
				/* update queue size and signal for next picture */
                if (++mPictureQueueReadIndex == VIDEO_PICTURE_QUEUE_SIZE)
                    mPictureQueueReadIndex = 0;
				mPictureQueueSize--;
				mPictureQueueCondition.broadcast();
				mPictureQueueMutex.unlock();                
                continue;
            }
        }
		
		//LOGI("rendererLoop: %d, running %d\n", __LINE__, mRunning);
		//double start = (double)av_gettime() / 1000.0;
		displayVideoFrame(&(vp->frame));
		//double end = (double)av_gettime() / 1000.0;
		//LOGI("displayVideoFrame: %fms\n", end-start);
		//LOGI("rendererLoop: %d, running %d\n", __LINE__, mRunning);
		
		mPictureQueueMutex.lock();
		/* update queue size and signal for next picture */
        if (++mPictureQueueReadIndex == VIDEO_PICTURE_QUEUE_SIZE)
            mPictureQueueReadIndex = 0;		
		mPictureQueueSize--;
		mPictureQueueCondition.broadcast();
		mPictureQueueMutex.unlock();		
    }
    
    LOGI("end of video rendererLoop\n");
}

}

