//
//  audioconvert.h
//  NewPlayer
//
//  Created by Slava on 8/5/14.
//  Copyright (c) 2014 Techery. All rights reserved.
//

#ifndef NewPlayer_audioconvert_h
#define NewPlayer_audioconvert_h

/**
 * @file
 * Audio format conversion routines
 */


#include "avcodec.h"


/**
 * Generate string corresponding to the sample format with
 * number sample_fmt, or a header if sample_fmt is negative.
 *
 * @param[in] buf the buffer where to write the string
 * @param[in] buf_size the size of buf
 * @param[in] sample_fmt the number of the sample format to print the corresponding info string, or
 * a negative value to print the corresponding header.
 * Meaningful values for obtaining a sample format info vary from 0 to SAMPLE_FMT_NB -1.
 */
void avcodec_sample_fmt_string(char *buf, int buf_size, int sample_fmt);

/**
 * @return NULL on error
 */
const char *avcodec_get_sample_fmt_name(int sample_fmt);

/**
 * @return SAMPLE_FMT_NONE on error
 */
enum SampleFormat avcodec_get_sample_fmt(const char* name);

/**
 * @return NULL on error
 */
const char *avcodec_get_channel_name(int channel_id);

/**
 * Return description of channel layout
 */
void avcodec_get_channel_layout_string(char *buf, int buf_size, int nb_channels, int64_t channel_layout);

/**
 * Guess the channel layout
 * @param nb_channels
 * @param codec_id Codec identifier, or CODEC_ID_NONE if unknown
 * @param fmt_name Format name, or NULL if unknown
 * @return Channel layout mask
 */
int64_t avcodec_guess_channel_layout(int nb_channels, enum CodecID codec_id, const char *fmt_name);

/**
 * @return the number of channels in the channel layout.
 */
int avcodec_channel_layout_num_channels(int64_t channel_layout);

struct AVAudioConvert;
typedef struct AVAudioConvert AVAudioConvert;

/**
 * Create an audio sample format converter context
 * @param out_fmt Output sample format
 * @param out_channels Number of output channels
 * @param in_fmt Input sample format
 * @param in_channels Number of input channels
 * @param[in] matrix Channel mixing matrix (of dimension in_channel*out_channels). Set to NULL to ignore.
 * @param flags See FF_MM_xx
 * @return NULL on error
 */
AVAudioConvert *av_audio_convert_alloc(enum SampleFormat out_fmt, int out_channels,
                                       enum SampleFormat in_fmt, int in_channels,
                                       const float *matrix, int flags);

/**
 * Free audio sample format converter context
 */
void av_audio_convert_free(AVAudioConvert *ctx);

/**
 * Convert between audio sample formats
 * @param[in] out array of output buffers for each channel. set to NULL to ignore processing of the given channel.
 * @param[in] out_stride distance between consecutive output samples (measured in bytes)
 * @param[in] in array of input buffers for each channel
 * @param[in] in_stride distance between consecutive input samples (measured in bytes)
 * @param len length of audio frame size (measured in samples)
 */
int av_audio_convert(AVAudioConvert *ctx,
                     void * const out[6], const int out_stride[6],
                     const void * const  in[6], const int  in_stride[6], int len);

#endif /* AVCODEC_AUDIOCONVERT_H */
