package com.v2soft.spoiq.ffmpeg.core;

import java.io.File;

/**
 * Created by imac on 8/5/14.
 */
public class FFMPEGFile {
    protected File mFile;
    protected FFMPEGAVFormatContext	mContext;

    protected FFMPEGFile(File file, FFMPEGAVFormatContext context) {
        mFile = file;
        mContext = context;
    }

    public File getFile() {
        return mFile;
    }

    public FFMPEGAVFormatContext getContext() {
        return mContext;
    }

    public boolean exists() {
        return mFile.exists();
    }

    public void delete() {
        mFile.delete();
        mFile = null;
        //mContext.release();
        //mContext = null;
    }
}
