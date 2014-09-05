package com.v2soft.spoiq.ffmpeg.core.exceptions;

/**
 * Created by imac on 8/5/14.
 */
public class FFMPEGException extends Exception {
    public static final int LEVEL_FATAL = -1;
    public static final int LEVEL_ERROR = -2;
    public static final int LEVEL_WARNING = -3;

    private int mLevel;

    public FFMPEGException(int level, String msg) {
        super(msg);
        mLevel = level;
    }

    public int getLevel() {
        return mLevel;
    }
}
