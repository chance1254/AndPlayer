package com.v2soft.spoiq.ffmpeg.core;

import android.content.Context;
import android.media.MediaScannerConnection;
import android.net.Uri;

/**
 * Created by imac on 8/5/14.
 */
public class FFMPEGMediaScannerNotifier implements MediaScannerConnection.MediaScannerConnectionClient {
    private MediaScannerConnection mConnection;
    private String mPath;

    private FFMPEGMediaScannerNotifier(Context context, String path) {
        mPath = path;
        mConnection = new MediaScannerConnection(context, this);
        mConnection.connect();
    }

    public static void scan(Context context, String path) {
        new FFMPEGMediaScannerNotifier(context, path);
    }

    @Override
    public void onMediaScannerConnected() {
        mConnection.scanFile(mPath, null);
    }

    @Override
    public void onScanCompleted(String path, Uri uri) {
        mConnection.disconnect();
    }
}
