package com.v2soft.spoiq.ffmpeg.core;

import android.content.Context;
import android.util.Log;

import com.v2soft.spoiq.ffmpeg.config.FFMPEGConfig;
import com.v2soft.spoiq.ffmpeg.core.exceptions.FFMPEGException;
import com.v2soft.spoiq.ffmpeg.interfaces.FFMpegListener;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

/**
 * Created by imac on 8/5/14.
 */
public class FFMPEG {
    public static final String[] NATIVE_LIBS = new String[]{
            "avutil-52",
            "avcodec-55",
            "avformat-55",
            "swscale-2",
            "mediaplayer",
            "jniaudio",
            "jnivideo",
            "ffmpeg_jni"

    };

    public static final String[] EXTENSIONS = new String[]{
            "mp4"
    };

    private static boolean isLoaded = false;
    private Thread thread;
    private FFMpegListener listener;
    private FFMPEGFile mInputFile;
    private FFMPEGFile mOutputFile;
    private boolean mConverting;

    public FFMPEG() throws FFMPEGException {
        if (!loadLibs()) {
            throw new FFMPEGException(FFMPEGException.LEVEL_FATAL, "Couldn't load native libs");
        }
        native_avcodec_register_all();
        native_av_register_all();
        mConverting = false;
    }

    /**
     * loads all native libraries
     *
     * @return true if all libraries was loaded, otherwise return false
     */
    private static boolean loadLibs() {
        if (isLoaded) {
            return true;
        }
        boolean err = false;
        for (int i = 0; i < NATIVE_LIBS.length; i++) {
            try {
                System.loadLibrary(NATIVE_LIBS[i]);
            } catch (UnsatisfiedLinkError e) {
                // fatal error, we can't load some our libs
                Log.d("FFMpeg", "Couldn't load lib: " + NATIVE_LIBS[i] + " - " + e.getMessage());
                err = true;
            }
        }
        if (!err) {
            isLoaded = true;
        }
        return isLoaded;
    }

    public FFMPEGUtils getUtils() {
        return new FFMPEGUtils();
    }

    public boolean isConverting() {
        return mConverting;
    }

    public void setListener(FFMpegListener listener) {
        this.listener = listener;
    }

    public FFMPEGFile getOutputFile() {
        return mOutputFile;
    }

    public FFMPEGFile getInputFile() {
        return mInputFile;
    }

    public void init(String inputFile, String outputFile) throws RuntimeException, IOException {
        native_av_init();

        mInputFile = setInputFile(inputFile);
        mOutputFile = setOutputFile(outputFile);
    }

    public void setConfig(FFMPEGConfig config) {
        setFrameSize(config.getVideoResolution()[0], config.getVideoResolution()[1]);
        setAudioChannels(config.getAudioChannelsCount());
        setAudioRate(config.getAudioRate());
        setFrameRate(config.getFrameRate());
        setVideoCodec(config.getCodec());
        setFrameAspectRatio(config.getRatio()[0], config.getRatio()[1]);
        setBitrate(config.getBitrate());

        native_av_parse_options(new String[]{"ffmpeg", mOutputFile.getFile().getAbsolutePath()});
    }

    public FFMpegMovieViewAndroid getMovieView(Context context) {
        return new FFMpegMovieViewAndroid(context);
    }

    public void setBitrate(String bitrate) {
        native_av_setBitrate("b", bitrate);
    }

    public void setFrameAspectRatio(int x, int y) {
        native_av_setFrameAspectRatio(x, y);
    }

    public void setVideoCodec(String codec) {
        native_av_setVideoCodec(codec);
    }

    public void setAudioRate(int rate) {
        native_av_setAudioRate(rate);
    }

    public void setAudioChannels(int channels) {
        native_av_setAudioChannels(channels);
    }

    public void setFrameRate(int rate) {
        native_av_setFrameRate(rate);
    }

    public void setFrameSize(int width, int height) {
        native_av_setFrameSize(width, height);
    }

    public FFMPEGFile setInputFile(String filePath) throws IOException {
        File f = new File(filePath);
        if (!f.exists()) {
            throw new FileNotFoundException("File: " + filePath + " doesn't exist");
        }
        FFMPEGAVFormatContext c = native_av_setInputFile(filePath);
        return new FFMPEGFile(f, c);
    }

    public FFMPEGFile setOutputFile(String filePath) throws FileNotFoundException {
        File f = new File(filePath);
        if (f.exists()) {
            f.delete();
        }
        //FFMpegAVFormatContext c = native_av_setOutputFile(filePath);
        return new FFMPEGFile(f, null);
    }

    public void newVideoStream(FFMPEGAVFormatContext context) {
        native_av_newVideoStream(context.pointer);
    }

    public void convert() throws RuntimeException {
        mConverting = true;
        if (listener != null) {
            listener.onConversionStarted();
        }

        native_av_convert();

        mConverting = false;
        if (listener != null) {
            listener.onConversionCompleted();
        }
    }

    public void convertAsync() throws RuntimeException {
        thread = new Thread() {
            @Override
            public void run() {
                try {
                    convert();
                } catch (RuntimeException e) {
                    if (listener != null) {
                        listener.onError(e);
                    }
                }
            }
        };
        thread.start();
    }

    public void waitOnEnd() throws InterruptedException {
        if (thread == null) {
            return;
        }
        thread.join();
    }

    public void release() {
        native_av_release(1);
    }

    /**
     * callback called by native code to inform java about conversion
     *
     * @param total_size
     * @param time
     * @param bitrate
     */
    private void onReport(double total_size, double time, double bitrate) {
        if (listener != null) {
            FFMPEGReport report = new FFMPEGReport();
            report.setTotalSize(total_size);
            report.setTime(time);
            report.setBitrate(bitrate);
            listener.onConversionProcessing(report);
        }
    }

    @Override
    protected void finalize() throws Throwable {
        Log.d("FFMpeg", "finalizing ffmpeg main class");
        isLoaded = false;
    }

    private native void native_avcodec_register_all();

    private native void native_av_register_all();

    //{ "native_avdevice_register_all", "()V", (void*) avdevice_register_all },

    //{ "native_avfilter_register_all", "()V", (void*) avfilter_register_all },

    private native void native_av_init() throws RuntimeException;

    private native FFMPEGAVFormatContext native_av_setInputFile(String filePath) throws IOException;

    private native FFMPEGAVFormatContext native_av_setOutputFile(String filePath) throws IOException;

    private native int native_av_setBitrate(String opt, String arg);

    private native void native_av_newVideoStream(int pointer);

    /**
     * ar
     *
     * @param rate
     */
    private native void native_av_setAudioRate(int rate);

    private native void native_av_setAudioChannels(int channels);

    private native void native_av_setVideoChannel(int channel);

    /**
     * r
     *
     * @param rate
     * @throws RuntimeException
     */
    private native FFMPEGAVRational native_av_setFrameRate(int rate) throws RuntimeException;

    /**
     * ration
     *
     * @param x
     * @param y
     */
    private native void native_av_setFrameAspectRatio(int x, int y);

    /**
     * codec
     *
     * @param codec
     */
    private native void native_av_setVideoCodec(String codec);

    /**
     * resolution
     *
     * @param width
     * @param height
     */
    private native void native_av_setFrameSize(int width, int height);

    private native void native_av_parse_options(String[] args) throws RuntimeException;

    private native void native_av_convert() throws RuntimeException;

    private native int native_av_release(int code);
}
