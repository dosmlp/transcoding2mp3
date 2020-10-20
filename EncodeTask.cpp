#include "EncodeTask.h"

EncodeTask::EncodeTask(const QString& file_name):
    source_file_name_(file_name) {
    ctx = new TaskContext();
    ctx->source_file_name = source_file_name_;
    int dot_index = source_file_name_.lastIndexOf(".");
    ctx->target_file_name = source_file_name_.insert(dot_index, "_");
}

EncodeTask::~EncodeTask() {
    delete ctx;
}
void encode2mp3(TaskContext* task_ctx) {

    QFile target_file(task_ctx->target_file_name);
    if (!target_file.open(QIODevice::ReadWrite)) {
        task_ctx->enc_done = true;
        return;
    }

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
    id3tag_set_title(lame_global, QFileInfo(target_file).fileName().toLocal8Bit().data());

    if (lame_init_params(lame_global) == -1) {
        target_file.close();
        target_file.remove();
        qDebug() << "lame_init_params error!";
        task_ctx->enc_done = true;
        return;
    }
    char* pcm_buf[3] = {0};
    pcm_buf[0] = (char*)alignment::aligned_alloc(16, BUFF_SAMPLE * 4);
    pcm_buf[1] = (char*)alignment::aligned_alloc(16, BUFF_SAMPLE * 4);
    pcm_buf[2] = (char*)alignment::aligned_alloc(16, BUFF_SAMPLE * 4);

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
            target_file.remove();
            switch (mp3_size) {
                case -1:
                    qDebug() << "-1:  mp3buf was too small";
                    break;
                case -2:
                    qDebug() << "-2:  malloc() problem";
                    break;
                case -3:
                    qDebug() << "-3:  lame_init_params() not called";
                    break;
                case -4:
                    qDebug() << "-4:  psycho acoustic problems";
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
    alignment::aligned_free(pcm_buf[0]);
    alignment::aligned_free(pcm_buf[1]);
    alignment::aligned_free(pcm_buf[2]);
    pcm_buf[0] = 0;
    pcm_buf[1] = 0;
    pcm_buf[2] = 0;
    task_ctx->enc_done = true;
}
void EncodeTask::run() {
    if (!QFile::exists(ctx->source_file_name)) {
        qDebug() << "file does not exist:" + ctx->source_file_name;
        ctx->dec_done = true;
        ctx->enc_done = true;
        return;
    }
    if (avformat_open_input(&ctx->in_format_ctx,
                            ctx->source_file_name.toLocal8Bit().data(),
                            0,
                            0) != 0) {
        qDebug() << "Open input file error!";
        return;
    }
    if (avformat_find_stream_info(ctx->in_format_ctx, 0) < 0) {
        qDebug() << "avformat_find_stream_info error!";
        return;
    }

    for (int i = 0; i < ctx->in_format_ctx->nb_streams; ++i) {
        if (ctx->in_format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            ctx->audio_index = i;
            ctx->in_codec_ctx = ctx->in_format_ctx->streams[i]->codec;
            ctx->in_codec = avcodec_find_decoder(ctx->in_codec_ctx->codec_id);
            if (ctx->in_codec == NULL) {
                qDebug() << "No audio data found";
                return;
            }
            break;
        }
    }
    if (avcodec_open2(ctx->in_codec_ctx, ctx->in_codec, 0) < 0) {
        qDebug() << "avcodec_open2 error!";
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
    if (ctx->in_codec->id == AV_CODEC_ID_MP3 &&
        QString(ctx->in_codec->name).contains("mp3")) {
        QFile uname(ctx->target_file_name);
        uname.open(QIODevice::ReadWrite);
        uname.close();
        ctx->dec_done = true;
        ctx->enc_done = true;
        return;
    }
    if (ctx->in_codec->id == AV_CODEC_ID_AAC && ctx->in_codec->profiles) {
        if (ctx->in_codec->profiles[0].profile == FF_PROFILE_AAC_LOW) {
            QFile uname(ctx->target_file_name);
            uname.open(QIODevice::ReadWrite);
            uname.close();
            ctx->dec_done = true;
            ctx->enc_done = true;
            return;
        }
    }
    thread encode_thread(encode2mp3, ctx);
    encode_thread.detach();
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
    ctx->dec_done = true;
    while (!ctx->enc_done) {
        QThread::msleep(1);
    }
}
