#include "ffmpegHelpUtils.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavutil/samplefmt.h"
#include <android/log.h>
#define MAX_FILES 100
#define MAX_STREAMS 4
#define TAG "ffmpegHelpUtils"

static int nb_output_files = 2;
static AVFormatContext *output_files[MAX_FILES];
static AVFormatContext *input_files[MAX_FILES];
static int64_t input_files_ts_offset[MAX_FILES];
AVCodecContext *avcodec_opts[AVMEDIA_TYPE_NB];
static int nb_input_files = 2;
static uint16_t *intra_matrix = NULL;
static uint16_t *inter_matrix = NULL;
static const char *last_asked_format = NULL;
static char *vstats_filename;
static FILE *vstats_file;
static char *video_standard;
static char *audio_codec_name = NULL;
static char *video_codec_name = NULL;
static int64_t start_time = 0;
const char **opt_names;
static int opt_programid = 0;
static int audio_sample_rate = 44100;
static int using_stdin = 0;
static int verbose = 1;
static int  video_channel = 0;
static int pgmyuv_compatibility_hack=0;
static int64_t channel_layout = 0;
unsigned int allocated_audio_out_size, allocated_audio_buf_size;
static short *samples;
static volatile int received_sigterm = 0;
static AVBitStreamFilterContext *video_bitstream_filters=NULL;
static AVBitStreamFilterContext *audio_bitstream_filters=NULL;
static AVBitStreamFilterContext *subtitle_bitstream_filters=NULL;
static AVBitStreamFilterContext *bitstream_filters[MAX_FILES][MAX_STREAMS];
static uint8_t *audio_buf;
static uint8_t *audio_out;
struct SwsContext *sws_opts;
static AVFormatContext *avformat_opts;
static int video_disable = 0;
static char *video_language = NULL;
static int do_benchmark = 0;
static int do_hex_dump = 0;
static int do_pkt_dump = 0;
static int do_psnr = 0;
static int do_pass = 0;
static int intra_dc_precision = 8;
static int me_threshold = 0;
static const char *video_rc_override_string=NULL;
static int same_quality = 0;
static int frame_width  = 0;
static int frame_height = 0;
static float video_qscale = 0;
static int intra_only = 0;
static int force_fps = 0;
static float frame_aspect_ratio = 0;
static enum PixelFormat frame_pix_fmt = PIX_FMT_NONE;
const enum AVSampleFormat *audio_sample_fmt = AV_SAMPLE_FMT_NONE;
static int nb_ocodecs;
static int video_global_header = 0;
static int thread_count= 2;
static AVRational frame_rate;
static AVCodec *output_codecs[MAX_FILES*MAX_STREAMS];
static unsigned int video_codec_tag = 0;
static int audio_stream_copy = 0;
static int video_stream_copy = 0;
#define FF_COMPLIANCE_INOFFICIAL -1
#define FF_COMPLIANCE_EXPERIMENTAL -2
static int opt_name_count;
static int copy_ts= 0;
static int64_t input_ts_offset = 0;
static int subtitle_disable = 0;
static char *subtitle_codec_name = NULL;
static char *subtitle_language = NULL;
static int video_discard = 0;
static int audio_disable = 0;
static int audio_channels = 1;
static int nb_icodecs;
static AVCodec *input_codecs[MAX_FILES*MAX_STREAMS];

int av_exit(int result){
    int i;
    for(i = 0; i < nb_output_files; i++){
        AVFormatContext *context = output_files[i];
        int j;
        if (!(context->oformat->flags & AVFMT_NOFILE) && context->pb){
            avio_close(context->pb);
        }

        for(j = 0; j < context->nb_streams; j++){
            avformat_free_context(&context->streams[j]->metadata);
            av_free(context->streams[j]->codec);
            av_free(context->streams[j]);
        }

        for(j = 0; j < context-> nb_programs; j++) {
            avformat_free_context(&context->programs[j]->metadata);
        }
        for(j = 0; j < context->nb_chapters; j++) {
            avformat_free_context(&context->chapters[j]->metadata);
        }
        avformat_free_context(&context->metadata);
        av_free(context);
    }

    for(i = 0; i < nb_input_files; i++) {
        avformat_close_input(input_files[i]);
    }

    av_free(intra_matrix);
    av_free(inter_matrix);

    if (vstats_file)
        avio_close(vstats_file);
    av_free(vstats_filename);

    av_free(opt_names);

    av_free(video_codec_name);
    av_free(audio_codec_name);

    av_free(video_standard);

    for (i = 0; i < AVMEDIA_TYPE_NB; i++){
        av_free(avcodec_opts[i]);
    }

    av_free(avformat_opts);
    av_free(sws_opts);
    av_free(audio_buf);
    av_free(audio_out);
    allocated_audio_buf_size= allocated_audio_out_size= 0;
    av_free(samples);

#if CONFIG_AVFILTER
    avfilter_uninit();
#endif

    if (received_sigterm) {
        __android_log_print(ANDROID_LOG_ERROR, TAG,
            "Received signal %d: terminating.\n",
            (int) received_sigterm);
        exit (255);
    }

    exit(result);
    return result;
}

AVFormatContext *opt_output_file(const char *filename)
{
}

static void choose_pixel_fmt(AVStream *st, AVCodec *codec)
{
    if(codec && codec->pix_fmts){
        const enum PixelFormat *p= codec->pix_fmts;
        for(; *p!=-1; p++){
            if(*p == st->codec->pix_fmt)
                break;
        }
        if(*p == -1
           && !(   st->codec->codec_id==CODEC_ID_MJPEG
                && st->codec->strict_std_compliance <= FF_COMPLIANCE_INOFFICIAL
                && (   st->codec->pix_fmt == PIX_FMT_YUV420P
                    || st->codec->pix_fmt == PIX_FMT_YUV422P)))
            st->codec->pix_fmt = codec->pix_fmts[0];
    }
}

void parse_options(int argc, char **argv, const OptionDef *options,
                   void (* parse_arg_function)(const char*))
{}

void opt_codec(int *pstream_copy, char **pcodec_name,
                      int codec_type, const char *arg)
{
    av_freep(pcodec_name);
    if (!strcmp(arg, "copy")) {
        *pstream_copy = 1;
    } else {
        *pcodec_name = av_strdup(arg);
    }
}

static enum AVCodecID find_codec_or_die(const char *name, int type, int encoder, int strict)
{
    const char *codec_string = encoder ? "encoder" : "decoder";
    AVCodec *codec;

    if(!name)
        return CODEC_ID_NONE;
    codec = encoder ?
        avcodec_find_encoder_by_name(name) :
        avcodec_find_decoder_by_name(name);
    if(!codec) {
        __android_log_print(ANDROID_LOG_ERROR, TAG,  "Unknown %s '%s'\n", codec_string, name);
        av_exit(1);
    }
    if(codec->type != type) {
        __android_log_print(ANDROID_LOG_ERROR, TAG,  "Invalid %s type '%s'\n", codec_string, name);
        av_exit(1);
    }
    if(codec->capabilities & CODEC_CAP_EXPERIMENTAL &&
       strict > FF_COMPLIANCE_EXPERIMENTAL) {
        __android_log_print(ANDROID_LOG_ERROR, TAG,  "%s '%s' is experimental and might produce bad "
                "results.\nAdd '-strict experimental' if you want to use it.\n",
                codec_string, codec->name);
        codec = encoder ?
            avcodec_find_encoder(codec->id) :
            avcodec_find_decoder(codec->id);
        if (!(codec->capabilities & CODEC_CAP_EXPERIMENTAL))
            __android_log_print(ANDROID_LOG_ERROR, TAG,  "Or use the non experimental %s '%s'.\n",
                    codec_string, codec->name);
        av_exit(1);
    }
    return codec->id;
}

void set_context_opts(void *ctx, void *opts_ctx, int flags)
{
    int i;
    for(i=0; i<opt_name_count; i++){
        char buf[256];
        const AVOption *opt;
        const char *str= av_get_string(opts_ctx, opt_names[i], &opt, buf, sizeof(buf));
        /* if an option with name opt_names[i] is present in opts_ctx then str is non-NULL */
        if(str && ((opt->flags & flags) == flags))
            av_set_string3(ctx, opt_names[i], str, 1, NULL);
    }
}

void new_video_stream(AVFormatContext *oc)
{
    AVStream *stream;
    AVCodecContext *codec_context;
    enum AVCodecID codec_id;
    stream = av_new_stream(oc, oc->nb_streams);
    //if stream did not create
    if (!stream) {
        __android_log_print(ANDROID_LOG_ERROR, TAG,  "Could not alloc stream\n");
        av_exit(1);
    }

    avcodec_get_context_defaults3(stream->codec, AVMEDIA_TYPE_VIDEO);
    bitstream_filters[nb_output_files][oc->nb_streams - 1]= video_bitstream_filters;
    video_bitstream_filters= NULL;

    /*avcodec_thread_init(stream->codec, thread_count);*/ //FIXME

    codec_context = stream->codec;

    if(video_codec_tag)
        codec_context->codec_tag= video_codec_tag;

    if((video_global_header&1)
       || (video_global_header==0 && (oc->oformat->flags & AVFMT_GLOBALHEADER))){
        codec_context->flags |= CODEC_FLAG_GLOBAL_HEADER;
        avcodec_opts[AVMEDIA_TYPE_VIDEO]->flags|= CODEC_FLAG_GLOBAL_HEADER;
    }
    if(video_global_header&2){
        codec_context->flags2 |= CODEC_FLAG2_LOCAL_HEADER;
        avcodec_opts[AVMEDIA_TYPE_VIDEO]->flags2|= CODEC_FLAG2_LOCAL_HEADER;
    }

    if (video_stream_copy) {
        codec_context->codec_type = AVMEDIA_TYPE_VIDEO;
        codec_context->sample_aspect_ratio =
        stream->sample_aspect_ratio = av_d2q(frame_aspect_ratio*frame_height/frame_width, 255);
    } else {
        const char *p;
        int i;
        AVCodec *codec;
        AVRational fps= frame_rate.num ? frame_rate : (AVRational){25,1};

        if (video_codec_name) {
            codec_id = find_codec_or_die(video_codec_name, AVMEDIA_TYPE_VIDEO, 1,
                                         codec_context->strict_std_compliance);
            codec = avcodec_find_encoder_by_name(video_codec_name);
            output_codecs[nb_ocodecs] = codec;
        } else {
            codec_id = av_guess_codec(oc->oformat, NULL, oc->filename, NULL, AVMEDIA_TYPE_VIDEO);
            codec = avcodec_find_encoder(codec_id);
        }

        codec_context->codec_id = codec_id;

        set_context_opts(codec_context, avcodec_opts[AVMEDIA_TYPE_VIDEO], AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM);

        if (codec && codec->supported_framerates && !force_fps)
            fps = codec->supported_framerates[av_find_nearest_q_idx(fps, codec->supported_framerates)];
        codec_context->time_base.den = fps.num;
        codec_context->time_base.num = fps.den;

        codec_context->width = frame_width;
        codec_context->height = frame_height;
        codec_context->sample_aspect_ratio = av_d2q(frame_aspect_ratio*codec_context->height/codec_context->width, 255);
        codec_context->pix_fmt = frame_pix_fmt;
        stream->sample_aspect_ratio = codec_context->sample_aspect_ratio;

        choose_pixel_fmt(stream, codec);

        if (intra_only)
            codec_context->gop_size = 0;
        if (video_qscale || same_quality) {
            codec_context->flags |= CODEC_FLAG_QSCALE;
            codec_context->global_quality=
                FF_QP2LAMBDA * video_qscale;
        }

        if(intra_matrix)
            codec_context->intra_matrix = intra_matrix;
        if(inter_matrix)
            codec_context->inter_matrix = inter_matrix;

        p= video_rc_override_string;
        for(i=0; p; i++){
            int start, end, q;
            int e=sscanf(p, "%d,%d,%d", &start, &end, &q);
            if(e!=3){
                __android_log_print(ANDROID_LOG_ERROR, TAG,  "error parsing rc_override\n");
                av_exit(1);
            }
            codec_context->rc_override=
                av_realloc(codec_context->rc_override,
                           sizeof(RcOverride)*(i+1));
            codec_context->rc_override[i].start_frame= start;
            codec_context->rc_override[i].end_frame  = end;
            if(q>0){
                codec_context->rc_override[i].qscale= q;
                codec_context->rc_override[i].quality_factor= 1.0;
            }
            else{
                codec_context->rc_override[i].qscale= 0;
                codec_context->rc_override[i].quality_factor= -q/100.0;
            }
            p= strchr(p, '/');
            if(p) p++;
        }
        codec_context->rc_override_count=i;
        if (!codec_context->rc_initial_buffer_occupancy)
            codec_context->rc_initial_buffer_occupancy = codec_context->rc_buffer_size*3/4;
        codec_context->me_threshold= me_threshold;
        codec_context->intra_dc_precision= intra_dc_precision - 8;

        if (do_psnr)
            codec_context->flags|= CODEC_FLAG_PSNR;

        /* two pass mode */
        if (do_pass) {
            if (do_pass == 1) {
                codec_context->flags |= CODEC_FLAG_PASS1;
            } else {
                codec_context->flags |= CODEC_FLAG_PASS2;
            }
        }
    }
    nb_ocodecs++;
    if (video_language) {
        av_dict_set(&stream->metadata, "language", video_language, 0);
        av_freep(&video_language);
    }

    /* reset some key parameters */
    video_disable = 0;
    av_freep(&video_codec_name);
    video_stream_copy = 0;
    frame_pix_fmt = PIX_FMT_NONE;
}

AVFormatContext *opt_input_file(const char *filename)
{
    AVFormatContext *ic;
    AVFormatParameters params, *ap = &params;
    AVInputFormat *file_iformat = NULL;
    int err, i, ret, rfps, rfps_base;
    int64_t timestamp;

    if (last_asked_format) {
        if (!(file_iformat = av_find_input_format(last_asked_format))) {
            __android_log_print(ANDROID_LOG_ERROR, TAG,  "Unknown input format: '%s'\n", last_asked_format);
            av_exit(1);
        }
        last_asked_format = NULL;
    }

    if (!strcmp(filename, "-"))
        filename = "pipe:";

    using_stdin |= !strncmp(filename, "pipe:", 5) ||
                    !strcmp(filename, "/dev/stdin");

    /* get default parameters from command line */
    ic = avformat_alloc_context();
    if (!ic) {
        //av_print_error(filename, AVERROR(ENOMEM));
        av_exit(1);
    }

    memset(ap, 0, sizeof(*ap));
    ap->prealloced_context = 1;
    ap->sample_rate = audio_sample_rate;
    ap->channels = audio_channels;
    ap->time_base.den = frame_rate.num;
    ap->time_base.num = frame_rate.den;
    ap->width = frame_width;
    ap->height = frame_height;
    ap->pix_fmt = frame_pix_fmt;
   // ap->sample_fmt = audio_sample_fmt; //FIXME:not implemented in libavformat
    ap->channel = video_channel;
    ap->standard = video_standard;

    set_context_opts(ic, avformat_opts, AV_OPT_FLAG_DECODING_PARAM);

    ic->video_codec_id   =
        find_codec_or_die(video_codec_name   , AVMEDIA_TYPE_VIDEO   , 0,
                          avcodec_opts[AVMEDIA_TYPE_VIDEO   ]->strict_std_compliance);
    ic->audio_codec_id   =
        find_codec_or_die(audio_codec_name   , AVMEDIA_TYPE_AUDIO   , 0,
                          avcodec_opts[AVMEDIA_TYPE_AUDIO   ]->strict_std_compliance);
    ic->subtitle_codec_id=
        find_codec_or_die(subtitle_codec_name, AVMEDIA_TYPE_SUBTITLE, 0,
                          avcodec_opts[AVMEDIA_TYPE_SUBTITLE]->strict_std_compliance);
    ic->flags |= AVFMT_FLAG_NONBLOCK;

    if(pgmyuv_compatibility_hack)
        ic->video_codec_id= CODEC_ID_PGMYUV;

    /* open the input file with generic libav function */
    err = avformat_open_input(&ic, filename, file_iformat, ap);
    if (err < 0) {
        //print_error(filename, err);
        av_exit(1);
    }
    if(opt_programid) {
        int i, j;
        int found=0;
        for(i=0; i<ic->nb_streams; i++){
            ic->streams[i]->discard= AVDISCARD_ALL;
        }
        for(i=0; i<ic->nb_programs; i++){
            AVProgram *p= ic->programs[i];
            if(p->id != opt_programid){
                p->discard = AVDISCARD_ALL;
            }else{
                found=1;
                for(j=0; j<p->nb_stream_indexes; j++){
                    ic->streams[p->stream_index[j]]->discard= AVDISCARD_DEFAULT;
                }
            }
        }
        if(!found){
            __android_log_print(ANDROID_LOG_ERROR, TAG,  "Specified program id not found\n");
            av_exit(1);
        }
        opt_programid=0;
    }

    /* If not enough info to get the stream parameters, we decode the
       first frames to get it. (used in mpeg case for example) */
    ret = av_find_stream_info(ic);
    if (ret < 0 && verbose >= 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG,  "%s: could not find codec parameters\n", filename);
        av_exit(1);
    }

    timestamp = start_time;
    /* add the stream start time */
    if (ic->start_time != AV_NOPTS_VALUE)
        timestamp += ic->start_time;

    /* if seeking requested, we execute it */
    if (start_time != 0) {
        ret = av_seek_frame(ic, -1, timestamp, AVSEEK_FLAG_BACKWARD);
        if (ret < 0) {
            __android_log_print(ANDROID_LOG_ERROR, TAG,  "%s: could not seek to position %0.3f\n",
                    filename, (double)timestamp / AV_TIME_BASE);
        }
        /* reset seek info */
        start_time = 0;
    }

    /* update the current parameters so that they match the one of the input stream */
    for(i=0;i<ic->nb_streams;i++) {
        AVStream *st = ic->streams[i];
        AVCodecContext *enc = st->codec;
        //avcodec_thread_init(enc, thread_count);
        switch(enc->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            set_context_opts(enc, avcodec_opts[AVMEDIA_TYPE_AUDIO], AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_DECODING_PARAM);
            //__android_log_print(ANDROID_LOG_ERROR, TAG,  "\nInput Audio channels: %d", enc->channels);
            channel_layout = enc->channel_layout;
            audio_channels = enc->channels;
            audio_sample_rate = enc->sample_rate;
            audio_sample_fmt = &enc->sample_fmt;
            input_codecs[nb_icodecs++] = avcodec_find_decoder_by_name(audio_codec_name);
            if(audio_disable)
                st->discard= AVDISCARD_ALL;
            break;
        case AVMEDIA_TYPE_VIDEO:
            set_context_opts(enc, avcodec_opts[AVMEDIA_TYPE_VIDEO], AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_DECODING_PARAM);
            frame_height = enc->height;
            frame_width = enc->width;
            if(ic->streams[i]->sample_aspect_ratio.num)
                frame_aspect_ratio=av_q2d(ic->streams[i]->sample_aspect_ratio);
            else
                frame_aspect_ratio=av_q2d(enc->sample_aspect_ratio);
            frame_aspect_ratio *= (float) enc->width / enc->height;
            frame_pix_fmt = enc->pix_fmt;
            rfps      = ic->streams[i]->r_frame_rate.num;
            rfps_base = ic->streams[i]->r_frame_rate.den;
            if(enc->lowres) {
                enc->flags |= CODEC_FLAG_EMU_EDGE;
                frame_height >>= enc->lowres;
                frame_width  >>= enc->lowres;
            }
            if(me_threshold)
                enc->debug |= FF_DEBUG_MV;

            if (enc->time_base.den != rfps*enc->ticks_per_frame || enc->time_base.num != rfps_base) {

                if (verbose >= 0)
                    __android_log_print(ANDROID_LOG_ERROR, TAG, "\nSeems stream %d codec frame rate differs from container frame rate: %2.2f (%d/%d) -> %2.2f (%d/%d)\n",
                            i, (float)enc->time_base.den / enc->time_base.num, enc->time_base.den, enc->time_base.num,

                    (float)rfps / rfps_base, rfps, rfps_base);
            }
            /* update the current frame rate to match the stream frame rate */
            frame_rate.num = rfps;
            frame_rate.den = rfps_base;

            input_codecs[nb_icodecs++] = avcodec_find_decoder_by_name(video_codec_name);
            if(video_disable)
                st->discard= AVDISCARD_ALL;
            else if(video_discard)
                st->discard= video_discard;
            break;
        case AVMEDIA_TYPE_DATA:
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            input_codecs[nb_icodecs++] = avcodec_find_decoder_by_name(subtitle_codec_name);
            if(subtitle_disable)
                st->discard = AVDISCARD_ALL;
            break;
        case AVMEDIA_TYPE_ATTACHMENT:
        case AVMEDIA_TYPE_UNKNOWN:
            nb_icodecs++;
            break;
        default:
            abort();
        }
    }

    input_files[nb_input_files] = ic;
    input_files_ts_offset[nb_input_files] = input_ts_offset - (copy_ts ? 0 : timestamp);
    /* dump the file content */
    if (verbose >= 0)
        av_dump_format(ic, nb_input_files, filename, 0);

    nb_input_files++;

    video_channel = 0;

    av_freep(&video_codec_name);
    av_freep(&audio_codec_name);
    av_freep(&subtitle_codec_name);
    return ic;
}

int opt_default(const char *opt, const char *arg)
{
    int type;
    int ret= 0;
    const AVOption *o= NULL;
    int opt_types[]={AV_OPT_FLAG_VIDEO_PARAM, AV_OPT_FLAG_AUDIO_PARAM, 0, AV_OPT_FLAG_SUBTITLE_PARAM, 0};

    for(type=0; type<AVMEDIA_TYPE_NB && ret>= 0; type++){
        const AVOption *o2 = av_find_opt(avcodec_opts[0], opt, NULL, opt_types[type], opt_types[type]);
        if(o2)
            ret = av_set_string3(avcodec_opts[type], opt, arg, 1, &o);
    }
    if(!o)
        ret = av_set_string3(avformat_opts, opt, arg, 1, &o);
    if(!o && sws_opts)
        ret = av_set_string3(sws_opts, opt, arg, 1, &o);
    if(!o){
        if(opt[0] == 'a')
            ret = av_set_string3(avcodec_opts[AVMEDIA_TYPE_AUDIO], opt+1, arg, 1, &o);
        else if(opt[0] == 'v')
            ret = av_set_string3(avcodec_opts[AVMEDIA_TYPE_VIDEO], opt+1, arg, 1, &o);
        else if(opt[0] == 's')
            ret = av_set_string3(avcodec_opts[AVMEDIA_TYPE_SUBTITLE], opt+1, arg, 1, &o);
    }
    if (o && ret < 0) {
        fprintf(stderr, "Invalid value '%s' for option '%s'\n", arg, opt);
        exit(1);
    }
    if (!o) {
        fprintf(stderr, "Unrecognized option '%s'\n", opt);
        exit(1);
    }

//    av_log(NULL, AV_LOG_ERROR, "%s:%s: %f 0x%0X\n", opt, arg, av_get_double(avcodec_opts, opt, NULL), (int)av_get_int(avcodec_opts, opt, NULL));

    //FIXME we should always use avcodec_opts, ... for storing options so there will not be any need to keep track of what i set over this
    opt_names= av_realloc(opt_names, sizeof(void*)*(opt_name_count+1));
    opt_names[opt_name_count++]= o->name;

    if(avcodec_opts[0]->debug || avformat_opts->debug)
        av_log_set_level(AV_LOG_DEBUG);
    return 0;
}

int opt_bitrate(const char *opt, const char *arg)
{
    int codec_type = opt[0]=='a' ? AVMEDIA_TYPE_AUDIO : AVMEDIA_TYPE_VIDEO;

    opt_default(opt, arg);

    if (av_get_int(avcodec_opts[codec_type], "b", NULL) < 1000)
        __android_log_print(ANDROID_LOG_ERROR, TAG,  "WARNING: The bitrate parameter is set too low. It takes bits/s as argument, not kbits/s\n");

    return 0;
}