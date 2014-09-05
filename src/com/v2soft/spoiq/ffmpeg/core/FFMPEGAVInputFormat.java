package com.v2soft.spoiq.ffmpeg.core;

/**
 * Created by imac on 8/5/14.
 */
public class FFMPEGAVInputFormat {
    protected long					pointer;
    private String 					mName;
    private String 					mLongName; // Descriptive name for the format, meant to be more human-readable than name.
    private int 					mPrivDataSize; // Size of private data so that it can be allocated in the wrapper.
    private int 					mFlags; // Can use flags: AVFMT_NOFILE, AVFMT_NEEDNUMBER.
    private String 					mExtensions; // If extensions are defined, then no probe is done.
    private FFMPEGAVCodecTag		mCodecTag;
    private FFMPEGAVInputFormat 	mNext;

    public String getName() {
        return mName;
    }
    public String getLongName() {
        return mLongName;
    }
    public int getPrivDataSize() {
        return mPrivDataSize;
    }
    public int getFlags() {
        return mFlags;
    }
    public String getExtensions() {
        return mExtensions;
    }
    public FFMPEGAVCodecTag getCodecTag() {
        return mCodecTag;
    }
    public FFMPEGAVInputFormat getNext() {
        return mNext;
    }

    private native void nativeRelease(int pointer);
}
