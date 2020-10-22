#include "EncodeTask.h"

EncodeTask::EncodeTask(const std::string& file_name):
    source_file_name_(file_name) {
    ctx = new TaskContext();
    ctx->source_file_name = source_file_name_;
    int dot_index = source_file_name_.find_last_of('.');
    ctx->target_file_name = source_file_name_.insert(dot_index, "_");
}

EncodeTask::~EncodeTask() {
    delete ctx;
}
void encode2mp3(TaskContext* task_ctx) {

    std::fstream target_file(task_ctx->target_file_name,ios::out|ios::app|ios::binary);
    if (!target_file.is_open()) {
        task_ctx->enc_done = true;
        return;
    }

    //设置MP3编码参数
    lame_global_flags* lame_global = lame_init();
    lame_set_in_samplerate(lame_global, task_ctx->in_codec_ctx->sample_rate);
    lame_set_num_channels(lame_global, av_get_channel_layout_nb_channels(task_ctx->in_codec_ctx->channel_layout));
    int source_brate = task_ctx->in_codec_ctx->bit_rate;
    if (source_brate == 0) {
        lame_set_brate(lame_global,  256);
    } else {
        lame_set_brate(lame_global,  source_brate / 1000);
    }
    id3tag_init(lame_global);
    id3tag_v2_only(lame_global);
//    id3tag_set_title(lame_global, QFileInfo(target_file).fileName().toLocal8Bit().data());

    if (lame_init_params(lame_global) == -1) {
        target_file.close();
        std::filesystem::remove(task_ctx->target_file_name);
        std::cerr << "lame_init_params error!" << std::endl;
        task_ctx->enc_done = true;
        return;
    }
    char* pcm_buf[3] = {0};
    pcm_buf[0] = (char*)align_malloc(BUFF_SAMPLE * 4, 16);
    pcm_buf[1] = (char*)align_malloc(BUFF_SAMPLE * 4, 16);
    pcm_buf[2] = (char*)align_malloc(BUFF_SAMPLE * 4, 16);

    for (;;) {
        task_ctx->lock_audio_fifo.lock();
        int read_samples = av_audio_fifo_read(task_ctx->audio_fifo, (void**)pcm_buf, 2400);
        task_ctx->lock_audio_fifo.unlock();
        //编码完成
        if (read_samples < 2400 && task_ctx->dec_done) {
            int mp3_size = lame_encode_flush(lame_global,
                                             (unsigned char*)pcm_buf[2],
                                             BUFF_SAMPLE * 4);
            target_file.write(pcm_buf[2], mp3_size);
            break;
        }
        int mp3_size = lame_encode_buffer_ieee_float(lame_global,
                                                     (const float*)pcm_buf[0],
                                                     (const float*)pcm_buf[1],
                                                     read_samples,
                                                     (unsigned char*)pcm_buf[2],
                                                     BUFF_SAMPLE * 4);
        if (mp3_size < 0) {
            target_file.close();
            std::filesystem::remove(task_ctx->target_file_name);
            switch (mp3_size) {
                case -1:
                    std::cerr << "-1:  mp3buf was too small" << std::endl;
                    break;
                case -2:
                    std::cerr << "-2:  malloc() problem" << std::endl;
                    break;
                case -3:
                    std::cerr << "-3:  lame_init_params() not called" << std::endl;
                    break;
                case -4:
                    std::cerr << "-4:  psycho acoustic problems" << std::endl;
                    break;
                default:
                    break;
            }
            task_ctx->enc_done = true;
            return;
        }
        target_file.write(pcm_buf[2], mp3_size);
    }

    lame_close(lame_global);
    target_file.close();
    align_free(pcm_buf[0]);
    align_free(pcm_buf[1]);
    align_free(pcm_buf[2]);
    pcm_buf[0] = 0;
    pcm_buf[1] = 0;
    pcm_buf[2] = 0;
    task_ctx->enc_done = true;
}
void EncodeTask::run() {
    if (!std::filesystem::exists(ctx->source_file_name)) {
        std::cerr << "file does not exist:" + ctx->source_file_name << std::endl;
        ctx->dec_done = true;
        ctx->enc_done = true;
        return;
    }
    if (avformat_open_input(&ctx->in_format_ctx,
                            ctx->source_file_name.c_str(),
                            0,
                            0) != 0) {
        std::cerr << "Open input file error!" << std::endl;
        return;
    }
    if (avformat_find_stream_info(ctx->in_format_ctx, 0) < 0) {
        std::cerr << "avformat_find_stream_info error!" << std::endl;
        return;
    }

    for (int i = 0; i < ctx->in_format_ctx->nb_streams; ++i) {
        if (ctx->in_format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            ctx->audio_index = i;
            ctx->in_codec_ctx = ctx->in_format_ctx->streams[i]->codec;
            ctx->in_codec = avcodec_find_decoder(ctx->in_codec_ctx->codec_id);
            if (ctx->in_codec == NULL) {
                std::cerr << "No audio data found" << std::endl;
                return;
            }
            break;
        }
    }
    if (avcodec_open2(ctx->in_codec_ctx, ctx->in_codec, 0) < 0) {
        std::cerr << "avcodec_open2 error!" << std::endl;
        return;
    }

    ctx->audio_fifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLTP,
                                          av_get_channel_layout_nb_channels(ctx->in_codec_ctx->channel_layout),
                                          BUFF_SAMPLE);
    ctx->out_frame->nb_samples = BUFF_SAMPLE * 2;
    ctx->out_frame->channel_layout = ctx->in_codec_ctx->channel_layout;
    ctx->out_frame->format = AV_SAMPLE_FMT_FLTP;
    ctx->out_frame->sample_rate = ctx->in_codec_ctx->sample_rate;
    av_frame_get_buffer(ctx->out_frame, 16);

    swr_alloc_set_opts(ctx->swr,
                       //in
                       ctx->in_codec_ctx->channel_layout,
                       AV_SAMPLE_FMT_FLTP,
                       ctx->in_codec_ctx->sample_rate,
                       //out
                       ctx->in_codec_ctx->channel_layout,
                       ctx->in_codec_ctx->sample_fmt,
                       ctx->in_codec_ctx->sample_rate,
                       0, 0);
    swr_init(ctx->swr);

    //不需要转码输出0字节文件
    std::string codec_name(ctx->in_codec->name);
    if (ctx->in_codec->id == AV_CODEC_ID_MP3 &&
        codec_name.find("mp3") != codec_name.npos) {
        std::fstream uname(ctx->target_file_name,ios::app|ios::binary|ios::out);
        uname.close();
        ctx->dec_done = true;
        ctx->enc_done = true;
        return;
    }
    if (ctx->in_codec->id == AV_CODEC_ID_AAC && ctx->in_codec->profiles) {
        if (ctx->in_codec->profiles[0].profile == FF_PROFILE_AAC_LOW) {
            std::fstream uname(ctx->target_file_name,ios::app|ios::binary|ios::out);
            uname.close();
            ctx->dec_done = true;
            ctx->enc_done = true;
            return;
        }
    }
    thread encode_thread(encode2mp3, ctx);

    while (av_read_frame(ctx->in_format_ctx, ctx->packet) >= 0) {
        int got = 0;
        if (ctx->packet->stream_index == ctx->audio_index) {
            int ret = avcodec_decode_audio4(ctx->in_codec_ctx,
                                            ctx->in_frame,
                                            &got,
                                            ctx->packet);
            if (ret < 0) {
                qDebug() << "avcodec_decode_audio4 error!";
                return;
            }
            if (got > 0) {
                AVFrame* tf = ctx->in_frame;
                int count = ctx->in_frame->nb_samples;
                if (ctx->swr) {
                    count = swr_convert(ctx->swr, ctx->out_frame->data,
                                        ctx->out_frame->nb_samples,
                                        (const uint8_t**)ctx->in_frame->data,
                                        ctx->in_frame->nb_samples);
                    tf = ctx->out_frame;
                }
                while (av_audio_fifo_size(ctx->audio_fifo) > BUFF_SAMPLE)
                    QThread::msleep(10);
                ctx->lock_audio_fifo.lock();
                av_audio_fifo_write(ctx->audio_fifo, (void**)tf->data, count);
                ctx->lock_audio_fifo.unlock();

            }

        }
        av_free_packet(ctx->packet);
    }
    encode_thread.join();
    ctx->dec_done = true;
    while (!ctx->enc_done) {
        QThread::msleep(1);
    }
}
