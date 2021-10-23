//
// Created by huaxia on 2021/10/23.
//

#include "softdecoder/SoftAudioDecoder.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
}

#include "android/log.h"

void SoftAudioDecoder::nativeStartPlay(JNIEnv *env, jstring filePath) {
    const char *filePathStr = env->GetStringUTFChars(filePath, nullptr);
    AVFormatContext *formatContext = avformat_alloc_context();
    AVCodecContext *codecContext = nullptr;
    AVPacket *avPacket = nullptr;
    do {
        int ret = avformat_open_input(&formatContext, filePathStr, nullptr, nullptr);
        if (ret) {
            break;
        }
        ret = avformat_find_stream_info(formatContext, nullptr);
        if (ret) {
            break;
        }
        int audioIndex = -1;
        AVStream *audioStream = nullptr;
        for (int i = 0; i < formatContext->nb_streams; i++) {
            AVStream *stream = formatContext->streams[i];
            if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                audioStream = stream;
                audioIndex = i;
                break;
            }
        }
        if (audioStream == nullptr) {
            //回调Java层，没有音轨
            break;
        }
        AVCodec *audioCodec = avcodec_find_decoder(audioStream->codecpar->codec_id);
        if (audioCodec == nullptr) {
            break;
        }
        codecContext = avcodec_alloc_context3(audioCodec);
        if (codecContext == nullptr) {
            break;
        }
        ret = avcodec_parameters_to_context(codecContext, audioStream->codecpar);
        if (ret) {
            break;
        }
        ret = avcodec_open2(codecContext, audioCodec, nullptr);
        if (ret) {
            break;
        }
        avPacket = av_packet_alloc();
        av_init_packet(avPacket);

        while (!mIsStop) {
            //解封装code
            ret = av_read_frame(formatContext, avPacket);
            if (ret) {
                __android_log_print(ANDROID_LOG_ERROR, "SoftAudioDecoder",
                                    "read frame packet error: %s",
                                    av_err2str(ret));
                //读到了多媒体文件的尾部，需要清空解码缓冲
                //清空解码缓冲区
                avcodec_send_packet(codecContext, nullptr);
                decodePCMAndPlay(codecContext);
                break;
            }
            __android_log_print(ANDROID_LOG_ERROR, "SoftAudioDecoder",
                                "read decodeFrame packet stream index: %d packet size: %d",
                                avPacket->stream_index, avPacket->size);
            if (avPacket->stream_index != audioIndex) {
                av_packet_unref(avPacket);
                continue;
            }
            //解码
            ret = avcodec_send_packet(codecContext, avPacket);
            if (ret < 0) {
                __android_log_print(ANDROID_LOG_ERROR, "SoftAudioDecoder", "send packet error: %s",
                                    av_err2str(ret));
                continue;
            }
            decodePCMAndPlay(codecContext);
            av_packet_unref(avPacket);
        }
    } while (false);

    if (formatContext) {
        avformat_close_input(&formatContext);
    }
    if (codecContext) {
        avcodec_free_context(&codecContext);
    }
    if (avPacket) {
        av_packet_free(&avPacket);
    }
    env->ReleaseStringUTFChars(filePath, filePathStr);
}

void SoftAudioDecoder::decodePCMAndPlay(AVCodecContext *codecContext) {
    SwrContext *swrContext = nullptr;
    AVFrame *decodeFrame = av_frame_alloc();
    if (decodeFrame == nullptr) {
        return;
    }
    int ret = -1;
    do {
        ret = avcodec_receive_frame(codecContext, decodeFrame);
        if (ret) {
            __android_log_print(ANDROID_LOG_ERROR, "SoftAudioDecoder",
                                "receive decodeFrame error: %s",
                                av_err2str(ret));
            if (ret == AVERROR_EOF) {
                break;
            } else {
                continue;
            }
        } else {
            __android_log_print(ANDROID_LOG_ERROR, "SoftAudioDecoder",
                                "decode decodeFrame sampleRate: %d bitWidth: %d channelCount: %d frameSize: %d",
                                decodeFrame->sample_rate,
                                decodeFrame->format,
                                decodeFrame->channels,
                                decodeFrame->pkt_size
            );
        }
        if (swrContext == nullptr) {
            swrContext = swr_alloc_set_opts(nullptr,
                                            AV_CH_LAYOUT_STEREO,
                                            AV_SAMPLE_FMT_S16,
                                            44100,
                                            decodeFrame->channel_layout,
                                            (AVSampleFormat) decodeFrame->format,
                                            decodeFrame->sample_rate,
                                            0, nullptr
            );
            if (swr_init(swrContext)) {
                break;
            }
        }
        uint8_t **dstData = nullptr;
        int dstLinSize;
        int dstChannelCount = decodeFrame->channels;
        av_samples_alloc_array_and_samples(&dstData, &dstLinSize, dstChannelCount, 1024,
                                           AV_SAMPLE_FMT_S16, 0);
        int swrRet = swr_convert(swrContext, dstData, 1024, (const uint8_t **) decodeFrame->data,
                                 decodeFrame->nb_samples);
        if (swrRet < 0) {
            __android_log_print(ANDROID_LOG_ERROR, "SoftAudioDecoder",
                                "swr_convert error: %s",
                                av_err2str(swrRet)
            );
        } else {
            __android_log_print(ANDROID_LOG_ERROR, "SoftAudioDecoder",
                                "swr_convert: %d",
                                swrRet
            );
        }
        av_freep(&dstData[0]);
        av_freep(&dstData);
    } while (!ret);
    if (swrContext) {
        swr_free(&swrContext);
    }
    av_frame_free(&decodeFrame);
}

void SoftAudioDecoder::stop() {
    this->mIsStop = true;
}
