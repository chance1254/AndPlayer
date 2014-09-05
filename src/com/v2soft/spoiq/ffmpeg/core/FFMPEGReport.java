package com.v2soft.spoiq.ffmpeg.core;

/**
 * Created by imac on 8/5/14.
 */
public class FFMPEGReport {
    private double totalSize;
    private double time;
    private double bitrate;

    public double getTotalSize() {
        return totalSize;
    }

    public void setTotalSize(double totalSize) {
        this.totalSize = totalSize;
    }

    public double getTime() {
        return time;
    }

    public void setTime(double time) {
        this.time = time;
    }

    public double getBitrate() {
        return bitrate;
    }

    public void setBitrate(double bitrate) {
        this.bitrate = bitrate;
    }
}
