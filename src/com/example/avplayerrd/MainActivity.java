package com.example.avplayerrd;

import java.io.IOException;

import com.media.ffmpeg.FFMpeg;
import com.media.ffmpeg.FFMpegException;
import com.media.ffmpeg.android.FFMpegMovieViewAndroid;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;

public class MainActivity extends Activity {

	private static final String 	TAG = "FFMpegPlayerActivity";
	//private static final String 	LICENSE = "This software uses libraries from the FFmpeg project under the LGPLv2.1";
	
	private FFMpegMovieViewAndroid 	mMovieView;
	//private WakeLock				mWakeLock;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		String filePath = Environment.getExternalStorageDirectory() + "/DCIM/Camera/ee.mp4";
		if(filePath == null) {
			Log.d(TAG, "Not specified video file");
			finish();
		} else {
			//PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
		    //mWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, TAG);

			try {
				FFMpeg ffmpeg = new FFMpeg();
				mMovieView = ffmpeg.getMovieView(this);
				try {
					mMovieView.setVideoPath(filePath);
				} catch (IllegalArgumentException e) {
					Log.e(TAG, "Can't set video: " + e.getMessage());
				} catch (IllegalStateException e) {
					Log.e(TAG, "Can't set video: " + e.getMessage());
				} catch (IOException e) {
					Log.e(TAG, "Can't set video: " + e.getMessage());
				}
				setContentView(mMovieView);
			} catch (FFMpegException e) {
				Log.d(TAG, "Error when inicializing ffmpeg: " + e.getMessage());
				finish();
			}
		}
	}
}
