//
// Created by huaxia on 2021/9/21.
//

#include "include/softdecoder/SoftDecoder.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
}

#include "android/log.h"
#include "android/native_window.h"
#include "android/native_window_jni.h"


SoftDecoder::SoftDecoder() {

}

void SoftDecoder::init(JNIEnv *pEnv, jstring file_path) {
    int ret = -1;
    const char *filePathChars = pEnv->GetStringUTFChars(file_path, nullptr);
    filePath = (char *) malloc(sizeof(char) * (int) strlen(filePathChars));
    strcpy(this->filePath, filePathChars);
    formatContext = avformat_alloc_context();
    if (!formatContext) {
        goto clear;
    }
    ret = avformat_open_input(&formatContext, this->filePath, nullptr, nullptr);
    if (ret < 0) {
        goto clear;
    }
    ret = avformat_find_stream_info(formatContext, nullptr);
    if (ret < 0) {
        goto clear;
    }
    return;
    clear:
    if (formatContext) {
        avformat_close_input(&formatContext);
        formatContext = nullptr;
        pEnv->ReleaseStringUTFChars(file_path, filePathChars);
    }
}

void SoftDecoder::release(JNIEnv *pEnv) {
    this->mIsRelease = true;
    if (formatContext) {
        avformat_close_input(&formatContext);
        avformat_free_context(formatContext);
        formatContext = nullptr;
    }
    if (this->filePath) {
        free(this->filePath);
        this->filePath = nullptr;
    }
    this->mDecodeFrameIndex = 0;
}

jobject SoftDecoder::getMediaFormat(JNIEnv *jniEnv) {
    if (formatContext) {
        jclass mediaFormatClass = jniEnv->FindClass("android/media/MediaFormat");
        jmethodID mediaFormatConstructor = jniEnv->GetMethodID(mediaFormatClass, "<init>", "()V");
        jobject mediaFormatInstance = jniEnv->NewObject(mediaFormatClass, mediaFormatConstructor);
        for (int i = 0; i < formatContext->nb_streams; i++) {
            AVStream *stream = formatContext->streams[i];
            if (!stream) {
                continue;
            }
            if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                jmethodID setInteger = jniEnv->GetMethodID(mediaFormatClass, "setInteger",
                                                           "(Ljava/lang/String;I)V");
                jfieldID widthFiled = jniEnv->GetStaticFieldID(mediaFormatClass, "KEY_WIDTH",
                                                               "Ljava/lang/String;");
                jfieldID heightFiled = jniEnv->GetStaticFieldID(mediaFormatClass, "KEY_HEIGHT",
                                                                "Ljava/lang/String;");
                jniEnv->CallVoidMethod(mediaFormatInstance, setInteger,
                                       jniEnv->GetStaticObjectField(mediaFormatClass, widthFiled),
                                       (int) stream->codecpar->width);
                jniEnv->CallVoidMethod(mediaFormatInstance, setInteger,
                                       jniEnv->GetStaticObjectField(mediaFormatClass, heightFiled),
                                       (int) stream->codecpar->height);
            }
        }
        return mediaFormatInstance;
    }
    return nullptr;
}


void SoftDecoder::startDecode(jobject javaSurface, JNIEnv *jniEnv, jobject javaInstance) {
    AVCodecContext *codecContext = nullptr;
    AVPacket *packet = nullptr;
    AVPixelFormat swsDstPixelFormat = AV_PIX_FMT_RGBA;
    int ret = -1;
    int videoStreamIndex = -1;
    if (formatContext->nb_streams <= 0) {
        goto clear;
    }
    for (int i = 0; i < formatContext->nb_streams; i++) {
        AVStream *stream = formatContext->streams[i];
        if (!stream) {
            continue;
        }
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
            if (!codec) {
                goto clear;
            }
            codecContext = avcodec_alloc_context3(codec);
            if (!codecContext) {
                goto clear;
            }
            ret = avcodec_parameters_to_context(codecContext, stream->codecpar);
            if (ret < 0) {
                goto clear;
            }
            ret = avcodec_open2(codecContext, codec, nullptr);
            if (ret) {
                goto clear;
            }
            break;
        }
    }
    __android_log_print(ANDROID_LOG_ERROR, "SoftDecoder", "video stream index: %d",
                        videoStreamIndex);
    packet = av_packet_alloc();
    av_init_packet(packet);
    if (!packet) {
        goto clear;
    }
    while (!mIsRelease) {
        //解封装code
        ret = av_read_frame(formatContext, packet);
        if (ret) {
            __android_log_print(ANDROID_LOG_ERROR, "SoftDecoder", "read decodeFrame error: %s",
                                av_err2str(ret));
            //读到了多媒体文件的尾部，需要清空解码缓冲
            //清空解码缓冲区
            avcodec_send_packet(codecContext, nullptr);
            decodeAndRender(jniEnv, javaSurface, codecContext, swsDstPixelFormat, javaInstance);
            break;
        }
        __android_log_print(ANDROID_LOG_ERROR, "SoftDecoder",
                            "packet stream index: %d packet size: %d",
                            packet->stream_index, packet->size);
        if (packet->stream_index != videoStreamIndex) {
            av_packet_unref(packet);
            continue;
        }
        //解码
        ret = avcodec_send_packet(codecContext, packet);
        if (ret < 0) {
            __android_log_print(ANDROID_LOG_ERROR, "SoftDecoder", "send packet error: %s",
                                av_err2str(ret));
            continue;
        }
        decodeAndRender(jniEnv, javaSurface, codecContext, swsDstPixelFormat, javaInstance);
        av_packet_unref(packet);
    }
    clear:
    if (packet) {
        av_packet_free(&packet);
    }
    if (codecContext) {
        avcodec_close(codecContext);
    }

}

void SoftDecoder::decodeAndRender(JNIEnv *pEnv, jobject javaSurface, AVCodecContext *codecContext,
                                  AVPixelFormat swsDstPixelFormat, jobject javaInstance) {
    SwsContext *swsContext = nullptr;
    AVFrame *decodeFrame = av_frame_alloc();
    jclass softDecoderClass = pEnv->GetObjectClass(javaInstance);
    jmethodID onVideoDecodeMethod = pEnv->GetMethodID(softDecoderClass, "onVideoDecode",
                                                      "(I)V");
    if (decodeFrame == nullptr) {
        return;
    }
    int ret = -1;
    do {
        ret = avcodec_receive_frame(codecContext, decodeFrame);
        if (ret) {
            __android_log_print(ANDROID_LOG_ERROR, "SoftDecoder",
                                "receive decodeFrame error: %s",
                                av_err2str(ret));
            if (ret == AVERROR_EOF) {
                break;
            } else {
                continue;
            }
        } else {
            __android_log_print(ANDROID_LOG_ERROR, "SoftDecoder",
                                "decode decodeFrame width: %d height: %d frameSize: %d",
                                decodeFrame->width,
                                decodeFrame->height,
                                decodeFrame->pkt_size
            );
        }

        pEnv->CallVoidMethod(javaInstance, onVideoDecodeMethod, this->mDecodeFrameIndex++);
        if (swsContext == nullptr) {
            swsContext = sws_getContext(decodeFrame->width, decodeFrame->height,
                                        codecContext->pix_fmt,
                                        decodeFrame->width, decodeFrame->height,
                                        swsDstPixelFormat,
                                        SWS_BILINEAR,
                                        nullptr, nullptr, nullptr);
        }
        //像素格式转换
        uint8_t *data[4];
        int lineSize[4];
        av_image_alloc(data, lineSize, decodeFrame->width, decodeFrame->height,
                       swsDstPixelFormat, 16);
        int swsScaleRet = sws_scale(swsContext, decodeFrame->data, decodeFrame->linesize, 0,
                                    decodeFrame->height,
                                    data, lineSize);
        __android_log_print(ANDROID_LOG_ERROR, "SoftDecoder",
                            "sws_scale: %d ", swsScaleRet);
        //向surface输出
        ANativeWindow *nativeWindow = ANativeWindow_fromSurface(pEnv, javaSurface);

        ANativeWindow_setBuffersGeometry(nativeWindow, decodeFrame->width, decodeFrame->height,
                                         AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM);
        ANativeWindow_Buffer aNativeWindowBuffer;
        ARect aRect;
        ANativeWindow_lock(nativeWindow, &aNativeWindowBuffer, &aRect);
        uint8_t *nativeWindowBuffer = static_cast<uint8_t *>(aNativeWindowBuffer.bits);
        int dstLineSize = aNativeWindowBuffer.stride * 4;
        int srcLineSize = lineSize[0];
        for (int i = 0; i < decodeFrame->height; ++i) {
            memcpy(nativeWindowBuffer + i * dstLineSize, data[0] + i * srcLineSize,
                   srcLineSize);
        }
        ANativeWindow_unlockAndPost(nativeWindow);
        ANativeWindow_release(nativeWindow);
        av_freep(&data[0]);
    } while (!ret);
    if (swsContext) {
        sws_freeContext(swsContext);
    }
    if (decodeFrame) {
        av_frame_free(&decodeFrame);
    }
}


