package com.v2soft.spoiq.ffmpeg.core;

/**
 * Created by imac on 8/5/14.
 */
public class FFMPEGAVFormatContext {
    public static final long AV_TIME_BASE = 1000000;

    protected int  						pointer;
    private int 						nb_streams;
    private String 						filename;
    private int 						ctx_flags;
    private long  						start_time;
    private long 						duration;
    private int 						bit_rate;
    private int 						packet_size;
    private int 						max_delay;
    private int 						flags;
    private FFMPEGAVInputFormat 		mInFormat;
    private FFMPEGAVOutputFormat		mOutFormat;

    public int getNbStreams() {
        return nb_streams;
    }

    public String getFilename() {
        return filename;
    }

    public int getCtxFlags() {
        return ctx_flags;
    }

    public long getStartTime() {
        return start_time;
    }

    public int getBitrate() {
        return bit_rate;
    }

    public int getPacketSize() {
        return packet_size;
    }

    public int getMaxDelay() {
        return max_delay;
    }

    public int getFlags() {
        return flags;
    }

    private FFMPEGAVFormatContext(){}

    public void release() {
        nativeRelease(pointer);
    }

    public Duration getDuration() {
        Duration d = new Duration();
        d.secs = (int) (duration / FFMPEGAVFormatContext.AV_TIME_BASE);
        d.mins = d.secs / 60;
        d.secs %= 60;
        d.hours = d.mins / 60;
        d.mins %= 60;
        return d;
    }

    public int getDurationInSeconds() {
        return (int) (duration / FFMPEGAVFormatContext.AV_TIME_BASE);
    }

    public int getDurationInMiliseconds() {
        return (int) duration / 1000;
    }

    private native void nativeRelease(int pointer);

    public class Duration {
        public int hours;
        public int mins;
        public int secs;

        private Duration(){}
    }

}
