package com.v2soft.spoiq.ffmpeg.core;

import android.util.Log;

import com.v2soft.spoiq.ffmpeg.core.FFMPEGAVFormatContext;

import java.io.File;
import java.io.IOException;

/**
 * Created by imac on 8/5/14.
 */
public class FFMPEGUtils {
    protected FFMPEGUtils() {}

    public void setOutput(String path) throws IOException {
        File f = new File(path);
        if(!f.exists()) {
            if(!f.mkdir()) {
                throw new IOException("Couldn't create directory: " + path);
            }
        }
        native_av_setOutput(path);
    }

    public FFMPEGAVFormatContext setInputFile(String filePath) throws IOException {
        return native_av_setInputFile(filePath);
    }

    public void printToSdcard(FFMPEGAVFormatContext context) throws IOException {
        native_av_print(context.pointer);
    }

    public void onVideoFrame(int[] pixels) {
        Log.d("FFMpegUtils", "pixels length: " + pixels.length);
    }

    private native FFMPEGAVFormatContext native_av_setInputFile(String filePath) throws IOException;
    private native void native_av_setOutput(String path);
    private native void native_av_print(int pAVFormatContext) throws IOException;
    private native void native_av_release();
}
