#ifndef FFMPEG_DECODER_AUDIO_H
#define FFMPEG_DECODER_AUDIO_H

#include "decoder.h"
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

typedef void (*AudioDecodingHandler) (int16_t*,int);

class DecoderAudio : public IDecoder
{
public:
    DecoderAudio(AVStream* stream);

    ~DecoderAudio();

    AudioDecodingHandler		onDecode;

private:
    int16_t*                    mSamples;
    int                         mSamplesSize;

    bool                        prepare();
    bool                        decode(void* ptr);
    bool                        process(AVPacket *packet);
};

#endif //FFMPEG_DECODER_AUDIO_H
