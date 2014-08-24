package com.media.ffmpeg.android;

import android.content.Context;
import android.view.Display;
import android.view.WindowManager;

import com.media.ffmpeg.config.FFMpegConfig;

public class FFMpegConfigAndroid extends FFMpegConfig {
	private static final int AUDIO_RATE = 16000;
	public FFMpegConfigAndroid(Context context) {
		overrideParametres(context);
	}
	
	private void overrideParametres(Context context) {
		setResolution(getScreenResolution(context));
		setRatio(RATIO_3_2);
		setAudioRate(AUDIO_RATE);
		setFrameRate(13);
	}
	
	@SuppressWarnings("deprecation")
	private int[] getScreenResolution(Context context) {
    	Display display = ((WindowManager) context.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay(); 
    	int[] res = new int[] {display.getHeight(), display.getWidth()};
    	return res;
	}

}
