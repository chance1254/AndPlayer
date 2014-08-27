#ifndef FFMPEG_ENCODER_AUDIO_AAC_H
#define FFMPEG_ENCODER_AUDIO_AAC_H

extern "C" {
	
#include "../android-aac-enc/inc/voAAC.h"
#include "../android-aac-enc/inc/cmnMemory.h"

}

#include "encoder_audio.h"

namespace ffmpeg {

class EncoderAudioAAC : public EncoderAudio
{
public:
	EncoderAudioAAC(MediaPlayer* mediaPlayer, char* output, bool bEnableReverb);
	~EncoderAudioAAC();

protected:
	
	virtual void			doEncode();
	
private:

	VO_HANDLE 		  		mHandle;
	VO_AUDIO_CODECAPI 		mCodecAPI;
	VO_MEM_OPERATOR   		mMemOperator;
	VO_CODEC_INIT_USERDATA 	mUserData;	
	AACENC_PARAM 			mParams;
};

}

#endif //FFMPEG_ENCODER_AUDIO_AAC_H
