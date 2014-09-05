package com.v2soft.spoiq.ffmpeg.core;

/**
 * Created by imac on 8/5/14.
 */
public class FFMPEGAVCodecContext {
    private long 						mPointer;
    private FFMPEGAVClass 				mAVClass;
    private int 						mBitRate;
    private int 						mBitRateTolerance;
    private int 						mFlags;

    private int 						mMeMethod; 				// Motion estimation algorithm used for video coding.
    private short 						mExtraData; 			// some codecs need / can use extradata like Huffman tables.
    private int 						mExtradataSize;

    // This is the fundamental unit of time (in seconds) in terms of which frame timestamps are represented.
    private FFMPEGAVRational			mTimeBase;

    // picture width / height.
    private int 						mWidth;
    private int 						mHeight;

    // the number of pictures in a group of pictures, or 0 for intra_only
    // encoding: Set by user.
    private int 						mGopSize;


    // samples per second
    private int 						mSampleRate;

    // number of audio channels
    private int 						mChannels;

    // Samples per packet, initialized when calling 'init'.
    private int 						mFrameSize;

    // audio or video frame number
    private int 						mFrameNumber;



    public int getWidth() {
        return mWidth;
    }

    public int getHeight() {
        return mHeight;
    }

    public int getBitRate() {
        return mBitRate;
    }

    public int getBitRateTolerance() {
        return mBitRateTolerance;
    }

    public int getSampleRate() {
        return mSampleRate;
    }

    public int getChannels() {
        return mChannels;
    }

    public int getFrameSize() {
        return mFrameSize;
    }

    public int getFrameNumber() {
        return mFrameNumber;
    }

    protected native void release();
}
