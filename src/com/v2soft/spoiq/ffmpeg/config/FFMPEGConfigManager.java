package com.v2soft.spoiq.ffmpeg.config;

import android.content.Context;
import android.view.Display;
import android.view.WindowManager;

/**
 * Created by imac on 8/5/14.
 */
public class FFMPEGConfigManager {
    private static final int CHANNELS_COUNT = 2;
    private static final int AUDIO_RATE = 44100;
    private static final int VIDEO_FPS = 30;
    private FFMPEGConfig config;
    private Context context;

    public FFMPEGConfigManager(Context context){
        this.context = context;
        initConfig();
    }

    private void initConfig(){
        config = new FFMPEGConfig();
        config.setAudioChannelsCount(CHANNELS_COUNT);
        config.setRatio(FFMPEGConfig.RATIO_3_2);
        config.setAudioRate(AUDIO_RATE);
        config.setFrameRate(VIDEO_FPS);
        config.setVideoResolution(getScreenResolution(context));
    }

    private int[] getScreenResolution(Context context) {
        Display display = ((WindowManager) context.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
        int[] res = new int[]{display.getHeight() / 2, display.getWidth() / 2};
        return res;
    }
}
