package com.v2soft.spoiq.ffmpeg.config;

/**
 * Created by imac on 8/5/14.
 */
public final class FFMPEGConfig {
    private static final String CODEC_MPEG4 = "mpeg4";
    public static final String BITRATE_HIGH = "1024000";
    public static final String BITRATE_MEDIUM = "512000";
    private static final String BITRATE_LOW = "128000";
    public static final int[] RATIO_3_2 = new int[]{3,2};
    private static final int[] RATIO_4_3 = new int[]{4,3};

    private int[] videoResolution;
    private int audioRate;
    private int frameRate;
    private int audioChannelsCount;
    private int[] ratio;

    public String getCodec(){
        return CODEC_MPEG4;
    }

    public String getBitrate(){
        return BITRATE_LOW;
    }

    public int[] getRatio(){
        return RATIO_4_3;
    }

    public void setRatio(int[] ratio){
        this.ratio = ratio;
    }

    public int getAudioChannelsCount() {
        return audioChannelsCount;
    }

    public void setAudioChannelsCount(int audioChannelsCount) {
        this.audioChannelsCount = audioChannelsCount;
    }

    public int getFrameRate() {
        return frameRate;
    }

    public void setFrameRate(int frameRate) {
        this.frameRate = frameRate;
    }

    public int getAudioRate() {
        return audioRate;
    }

    public void setAudioRate(int audioRate) {
        this.audioRate = audioRate;
    }

    public int[] getVideoResolution() {
        return videoResolution;
    }

    public void setVideoResolution(int[] videoResolution) {
        this.videoResolution = videoResolution;
    }
}
