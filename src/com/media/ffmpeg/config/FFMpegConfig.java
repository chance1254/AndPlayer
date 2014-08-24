package com.media.ffmpeg.config;

import java.util.Arrays;

public class FFMpegConfig {
	public static final String CODEC_MPEG4 = "mpeg4";

	public static final String BITRATE_HIGH = "1024000";
	public static final String BITRATE_MEDIUM = "512000";
	public static final String BITRATE_LOW = "128000";

	public static final int[] RATIO_3_2 = new int[] { 3, 2 };
	public static final int[] RATIO_4_3 = new int[] { 4, 3 };

	private int[] resolution;
	private String codec;
	private String bitrate;
	private int[] ratio;
	private int audioRate;
	private int frameRate;
	private int audioChannels;

	public int[] getResolution() {
		return resolution;
	}

	public void setResolution(int[] resolution) {
		this.resolution = resolution;
	}

	public String getCodec() {
		return codec;
	}

	public void setCodec(String codec) {
		this.codec = codec;
	}

	public String getBitrate() {
		return bitrate;
	}

	public void setBitrate(String bitrate) {
		this.bitrate = bitrate;
	}

	public int[] getRatio() {
		return ratio;
	}

	public void setRatio(int[] ratio) {
		this.ratio = ratio;
	}

	public int getAudioRate() {
		return audioRate;
	}

	public void setAudioRate(int audioRate) {
		this.audioRate = audioRate;
	}

	public int getFrameRate() {
		return frameRate;
	}

	public void setFrameRate(int frameRate) {
		this.frameRate = frameRate;
	}

	public int getAudioChannels() {
		return audioChannels;
	}

	public void setAudioChannels(int audioChannels) {
		this.audioChannels = audioChannels;
	}

	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + audioChannels;
		result = prime * result + audioRate;
		result = prime * result + ((bitrate == null) ? 0 : bitrate.hashCode());
		result = prime * result + ((codec == null) ? 0 : codec.hashCode());
		result = prime * result + frameRate;
		result = prime * result + Arrays.hashCode(ratio);
		result = prime * result + Arrays.hashCode(resolution);
		return result;
	}

	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		FFMpegConfig other = (FFMpegConfig) obj;
		if (audioChannels != other.audioChannels)
			return false;
		if (audioRate != other.audioRate)
			return false;
		if (bitrate == null) {
			if (other.bitrate != null)
				return false;
		} else if (!bitrate.equals(other.bitrate))
			return false;
		if (codec == null) {
			if (other.codec != null)
				return false;
		} else if (!codec.equals(other.codec))
			return false;
		if (frameRate != other.frameRate)
			return false;
		if (!Arrays.equals(ratio, other.ratio))
			return false;
		if (!Arrays.equals(resolution, other.resolution))
			return false;
		return true;
	}
}
