package com.v2soft.spoiq.ffmpeg.core;

/**
 * Created by imac on 8/5/14.
 */
public class FFMPEGAVRational {
    protected int	pointer;
    private int  	mNum; // numerator
    private int 	mDen; // denominator

    private FFMPEGAVRational() {}

    public int getNumerator() {
        return mNum;
    }

    public int getDenominator() {
        return mDen;
    }

    protected void release() {
        nativeRelease(pointer);
    }

    private native void nativeRelease(int pointer);
}
