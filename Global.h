#ifndef GLOBAL_H
#define GLOBAL_H
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <lame.h>
#include <libavutil/audio_fifo.h>
}
#include <iostream>
#include <fstream>
#include <thread>

using namespace std;
using namespace boost;

static int BUFF_SAMPLE = 48000;
struct TaskContext {
    string source_file_name;
    string target_file_name;
    AVFormatContext* in_format_ctx;
    AVCodec* in_codec;
    AVCodecContext* in_codec_ctx;
    AVFrame* in_frame;
    AVFrame* out_frame;
    AVPacket* packet;
    SwrContext* swr;
    int audio_index;
    AVAudioFifo* audio_fifo;
    boost::detail::spinlock lock_audio_fifo;
    volatile bool dec_done;
    volatile bool enc_done;
    TaskContext() {
        in_format_ctx = avformat_alloc_context();
        in_codec = 0;
        in_codec_ctx = 0;
        audio_index = -1;
        audio_fifo = 0;
        in_frame = av_frame_alloc();
        out_frame = av_frame_alloc();
        packet = av_packet_alloc();
        av_init_packet(packet);
        swr = swr_alloc();
        dec_done = false;
        enc_done = false;
        lock_audio_fifo.unlock();
    }
    ~TaskContext() {
        avformat_free_context(in_format_ctx);
        av_packet_free(&packet);
        av_frame_free(&in_frame);
        av_frame_free(&out_frame);
        swr_free(&swr);
        av_audio_fifo_free(audio_fifo);
    }
};
#endif // GLOBAL_H
