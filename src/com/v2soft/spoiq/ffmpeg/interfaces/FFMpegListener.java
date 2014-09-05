package com.v2soft.spoiq.ffmpeg.interfaces;

import com.v2soft.spoiq.ffmpeg.core.FFMPEGReport;

/**
 * Created by imac on 8/5/14.
 */
public interface FFMpegListener {
    public void onConversionProcessing(FFMPEGReport report);
    public void onConversionStarted();
    public void onConversionCompleted();
    public void onError(Exception e);
}
