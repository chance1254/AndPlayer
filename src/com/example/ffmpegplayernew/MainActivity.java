package com.example.ffmpegplayernew;

import java.io.IOException;

import com.v2soft.spoiq.ffmpeg.core.FFMPEG;
import com.v2soft.spoiq.ffmpeg.core.FFMpegMovieViewAndroid;
import com.v2soft.spoiq.ffmpeg.core.exceptions.FFMPEGException;

import android.app.Activity;
import android.content.Intent;
import android.graphics.SurfaceTexture;
import android.media.AudioManager;
import android.media.SoundPool;
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
		String filePath = Environment.getExternalStorageDirectory() + "/DCIM/Camera/ee1.mp4";
		try {
            FFMPEG ffmpeg = new FFMPEG();
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
		} catch(Exception e){
			e.printStackTrace();
		}
	}
}
