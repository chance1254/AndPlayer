#ifndef FFMPEG_DECODER_AUDIO_H
#define FFMPEG_DECODER_AUDIO_H

#include "decoder.h"

class DecoderAudio : public IDecoder
{
public:
    DecoderAudio(MediaPlayer* mediaPlayer, AVStream* stream);

    ~DecoderAudio();

private:
    double                      mAudioClock;
    int16_t*                    mSamples;
    int                         mSamplesSize;

    bool                        prepare();
    bool                        decode(void* ptr);
    bool                        process(AVPacket *packet);
};

#endif //FFMPEG_DECODER_AUDIO_H
