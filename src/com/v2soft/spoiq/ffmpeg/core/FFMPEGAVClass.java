package com.v2soft.spoiq.ffmpeg.core;

/**
 * Created by imac on 8/5/14.
 */
public class FFMPEGAVClass {
    protected long 			pointer;
    private String 			mClassName; // The name of the class; usually it is the same name as the context structure type to which the AVClass is associated.
    private FFMPEGAVOption 	mOption; // a pointer to the first option specified in the class if any or NULL
    private int 			mVersion; //LIBAVUTIL_VERSION with which this structure was created.
    private int 			mLogLevelOffset; // Offset in the structure where log_level_offset is stored.
    private int 			mParentLogContextOffset; //Offset in the structure where a pointer to the parent context for loging is stored.

    public String getClassName() {
        return mClassName;
    }
    public FFMPEGAVOption getOption() {
        return mOption;
    }
    public int getVersion() {
        return mVersion;
    }
    public int getLogLevelOffset() {
        return mLogLevelOffset;
    }
    public int getParentLogContextOffset() {
        return mParentLogContextOffset;
    }
}
