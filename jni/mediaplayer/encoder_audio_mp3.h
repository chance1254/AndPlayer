#ifndef FFMPEG_ENCODER_AUDIO_MP3_H
#define FFMPEG_ENCODER_AUDIO_MP3_H

#include "encoder_audio.h"
#include "../lame-3.99.5/include/lame.h"

namespace ffmpeg {

class EncoderAudioMP3 : public EncoderAudio
{
public:
	EncoderAudioMP3(MediaPlayer* mediaPlayer, char* output, bool bEnableReverb);
	~EncoderAudioMP3();

protected:
	
	virtual void			doEncode();
	
private:
	lame_t mHandle;
};

}

#endif //FFMPEG_ENCODER_AUDIO_AAC_H
