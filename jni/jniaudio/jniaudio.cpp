#include "jniaudio.h"

AndroidAudioTrack::AndroidAudioTrack() {
	mAudioTrack = NULL;
}

AndroidAudioTrack::~AndroidAudioTrack() {
	close();
}

void AndroidAudioTrack::close() {
	if (mAudioTrack) {
		if (at_stop)
			at_stop(mAudioTrack);
		if (at_flush)
			at_flush(mAudioTrack);
		if (at_dtor)
			at_dtor(mAudioTrack);
		free(mAudioTrack);
		mAudioTrack = NULL;
	}
}

int AndroidAudioTrack::set(int streamType, uint32_t sampleRate, int format,
		int channels) {
	int status;
	int minFrameCount = 0;
	int size = 0;

	LOGI("streamTyp = %d, sampleRate = %d, format = %d, channels = %d\n",
			streamType, sampleRate, format, channels);
	close();

	if (at_getMinFrameCount) {
		status = at_getMinFrameCount(&minFrameCount, streamType, sampleRate);
		LOGI("at_getMinFrameCount %d, %d\n", minFrameCount, status);
	}
	//size = minFrameCount * (channels == CHANNEL_OUT_STEREO ? 2 : 1) * 4;

	mAudioTrack = malloc(SIZE_OF_AUDIOTRACK);
	*((uint32_t *) ((uint32_t) mAudioTrack + SIZE_OF_AUDIOTRACK - 4)) =
			0xbaadbaad;
	if (at_ctor) {
		at_ctor(mAudioTrack, streamType, sampleRate, format, channels, size, 0,
		NULL, NULL, 0, 0);
	} else if (at_ctor_legacy) {
		at_ctor_legacy(mAudioTrack, streamType, sampleRate, format, channels,
				size, 0, NULL, NULL, 0);
	}
	assert(
			(*((uint32_t *) ((uint32_t)mAudioTrack + SIZE_OF_AUDIOTRACK - 4)) == 0xbaadbaad));

	/* And Init */
	status = at_initCheck(mAudioTrack);
	LOGI("at_initCheck = %d\n", status);

	/* android 1.6 uses channel count instead of stream_type */
	if (status != 0 && at_ctor_legacy) {
		channels = (channels == CHANNEL_OUT_STEREO) ? 2 : 1;
		at_ctor_legacy(mAudioTrack, streamType, sampleRate, format, channels,
				size, 0, NULL, NULL, 0);
		status = at_initCheck(mAudioTrack);
		LOGI("at_initCheck2 = %d\n", status);
	}
	if (status != 0) {
		__android_log_print(ANDROID_LOG_INFO, TAG, "Cannot create AudioTrack!");
		free(mAudioTrack);
		mAudioTrack = NULL;
	}
	return status;
}

uint32_t AndroidAudioTrack::latency() {
	if (mAudioTrack && at_latency) {
		return at_latency(mAudioTrack);
	}
	return 0;
}

int AndroidAudioTrack::start() {
	if (mAudioTrack && at_start) {
		at_start(mAudioTrack);
		return ANDROID_AUDIOTRACK_RESULT_SUCCESS;
	}
	return ANDROID_AUDIOTRACK_RESULT_ERRNO;
}

int AndroidAudioTrack::write(void* buffer, int size) {
	if (mAudioTrack && at_write) {
		return at_write(mAudioTrack, buffer, size);
	}
	return ANDROID_AUDIOTRACK_RESULT_ERRNO;
}

int AndroidAudioTrack::flush() {
	if (mAudioTrack && at_flush) {
		at_flush(mAudioTrack);
		return ANDROID_AUDIOTRACK_RESULT_SUCCESS;
	}
	return ANDROID_AUDIOTRACK_RESULT_ERRNO;
}

int AndroidAudioTrack::stop() {
	if (mAudioTrack && at_stop) {
		at_stop(mAudioTrack);
		return ANDROID_AUDIOTRACK_RESULT_SUCCESS;
	}
	return ANDROID_AUDIOTRACK_RESULT_ERRNO;
}

int AndroidAudioTrack::reload() {
	return ANDROID_AUDIOTRACK_RESULT_SUCCESS;
}

static void* InitLibrary() {
	/* DL Open libmedia */
	void *p_library;
	p_library = dlopen("libmedia.so", RTLD_NOW);
	if (!p_library)
		return NULL;

	/* Register symbols */
	as_getOutputFrameCount = (AudioSystem_getOutputFrameCount) (dlsym(p_library,
			"_ZN7android11AudioSystem19getOutputFrameCountEPii"));
	as_getOutputLatency = (AudioSystem_getOutputLatency) (dlsym(p_library,
			"_ZN7android11AudioSystem16getOutputLatencyEPji"));
	as_getOutputSamplingRate = (AudioSystem_getOutputSamplingRate) (dlsym(
			p_library, "_ZN7android11AudioSystem21getOutputSamplingRateEPii"));
	at_getMinFrameCount = (AudioTrack_getMinFrameCount) (dlsym(p_library,
			"_ZN7android10AudioTrack16getMinFrameCountEPiij"));
	at_ctor = (AudioTrack_ctor) (dlsym(p_library,
			"_ZN7android10AudioTrackC1EijiiijPFviPvS1_ES1_ii"));
	at_ctor_legacy = (AudioTrack_ctor_legacy) (dlsym(p_library,
			"_ZN7android10AudioTrackC1EijiiijPFviPvS1_ES1_i"));
	at_dtor =
			(AudioTrack_dtor) (dlsym(p_library, "_ZN7android10AudioTrackD1Ev"));
	at_initCheck = (AudioTrack_initCheck) (dlsym(p_library,
			"_ZNK7android10AudioTrack9initCheckEv"));
	at_latency = (AudioTrack_latency) (dlsym(p_library,
			"_ZNK7android10AudioTrack7latencyEv"));
	at_start = (AudioTrack_start) (dlsym(p_library,
			"_ZN7android10AudioTrack5startEv"));
	at_stop = (AudioTrack_stop) (dlsym(p_library,
			"_ZN7android10AudioTrack4stopEv"));
	at_write = (AudioTrack_write) (dlsym(p_library,
			"_ZN7android10AudioTrack5writeEPKvj"));
	at_flush = (AudioTrack_flush) (dlsym(p_library,
			"_ZN7android10AudioTrack5flushEv"));

	LOGI("p_library : %p\n", p_library);
	LOGI("as_getOutputFrameCount : %p\n", as_getOutputFrameCount);
	LOGI("as_getOutputLatency : %p\n", as_getOutputLatency);
	LOGI("as_getOutputSamplingRate : %p\n", as_getOutputSamplingRate);
	LOGI("at_getMinFrameCount : %p\n", at_getMinFrameCount);
	LOGI("at_ctor : %p\n", at_ctor);
	LOGI("at_ctor_legacy : %p\n", at_ctor_legacy);
	LOGI("at_dtor : %p\n", at_dtor);
	LOGI("at_initCheck : %p\n", at_initCheck);
	LOGI("at_latency : %p\n", at_latency);
	LOGI("at_start : %p\n", at_start);
	LOGI("at_stop : %p\n", at_stop);
	LOGI("at_write : %p\n", at_write);
	LOGI("at_flush : %p\n", at_flush);

	/* We need the first 3 or the last 1 */
#if 0
	if (!((as_getOutputFrameCount && as_getOutputLatency && as_getOutputSamplingRate)
					|| at_getMinFrameCount)) {
		dlclose(p_library);
		return NULL;
	}
#endif

	// We need all the other Symbols
	if (!((at_ctor || at_ctor_legacy) && at_dtor && at_initCheck && at_start
			&& at_stop && at_write && at_flush)) {
		dlclose(p_library);
		return NULL;
	}
	return p_library;
}

static int ffmpeg_ao_open(AUDIO_DRIVER_HANDLE* outHandle, uint32_t sampleRate,
		int format, int channels) {
	int ret = ANDROID_AUDIOTRACK_RESULT_ERRNO;
	AndroidAudioTrack* audioTrack = new AndroidAudioTrack();

	*outHandle = NULL;
	if (audioTrack->set(MUSIC, sampleRate, format,
			channels) == ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
		*outHandle = audioTrack;
		ret = ANDROID_AUDIOTRACK_RESULT_SUCCESS;
	} else {
		delete audioTrack;
	}
	return ret;
}

static int ffmpeg_ao_close(AUDIO_DRIVER_HANDLE handle) {
	AndroidAudioTrack* audioTrack = (AndroidAudioTrack*) handle;
	if (!audioTrack)
		return ANDROID_AUDIOTRACK_RESULT_ERRNO;
	delete audioTrack;
	return ANDROID_AUDIOTRACK_RESULT_SUCCESS;
}

static int ffmpeg_ao_set(AUDIO_DRIVER_HANDLE handle, int streamType,
		uint32_t sampleRate, int format, int channels) {
	AndroidAudioTrack* audioTrack = (AndroidAudioTrack*) handle;
	if (!audioTrack)
		return ANDROID_AUDIOTRACK_RESULT_ERRNO;
	return audioTrack->set(streamType, sampleRate, format, channels);
}

static uint32_t ffmpeg_ao_latency(AUDIO_DRIVER_HANDLE handle) {
	AndroidAudioTrack* audioTrack = (AndroidAudioTrack*) handle;
	if (!audioTrack)
		return ANDROID_AUDIOTRACK_RESULT_ERRNO;
	return audioTrack->latency();
}

static int ffmpeg_ao_start(AUDIO_DRIVER_HANDLE handle) {
	AndroidAudioTrack* audioTrack = (AndroidAudioTrack*) handle;
	if (!audioTrack)
		return ANDROID_AUDIOTRACK_RESULT_ERRNO;
	return audioTrack->start();
}

static int ffmpeg_ao_write(AUDIO_DRIVER_HANDLE handle, void *buffer,
		int buffer_size) {
	AndroidAudioTrack* audioTrack = (AndroidAudioTrack*) handle;
	if (!audioTrack)
		return ANDROID_AUDIOTRACK_RESULT_ERRNO;
	return audioTrack->write(buffer, buffer_size);
}

static int ffmpeg_ao_flush(AUDIO_DRIVER_HANDLE handle) {
	AndroidAudioTrack* audioTrack = (AndroidAudioTrack*) handle;
	if (!audioTrack)
		return ANDROID_AUDIOTRACK_RESULT_ERRNO;
	return audioTrack->flush();
}

static int ffmpeg_ao_stop(AUDIO_DRIVER_HANDLE handle) {
	AndroidAudioTrack* audioTrack = (AndroidAudioTrack*) handle;
	if (!audioTrack)
		return ANDROID_AUDIOTRACK_RESULT_ERRNO;
	return audioTrack->stop();
}

static int ffmpeg_ao_reload(AUDIO_DRIVER_HANDLE handle) {
	AndroidAudioTrack* audioTrack = (AndroidAudioTrack*) handle;
	if (!audioTrack)
		return ANDROID_AUDIOTRACK_RESULT_ERRNO;
	return audioTrack->reload();
}

static ffmpeg_ao_t ffmpeg_ao = { ffmpeg_ao_open, ffmpeg_ao_close,
		ffmpeg_ao_latency, ffmpeg_ao_start, ffmpeg_ao_write, ffmpeg_ao_flush,
		ffmpeg_ao_stop, ffmpeg_ao_reload, };

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env = NULL;
	jint result = JNI_ERR;

	libmedia = InitLibrary();
	ffmpeg_ao_register(&ffmpeg_ao);

	LOGI("loaded");
	result = JNI_VERSION_1_4;

	return result;
}

