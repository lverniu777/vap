//
// Created by huaxia on 2021/9/21.
//

#include "include/softdecoder/SoftVideoDecoder.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
}

#include "android/log.h"
#include "android/native_window.h"
#include "android/native_window_jni.h"


SoftVideoDecoder::SoftVideoDecoder() {

}

void SoftVideoDecoder::init(JNIEnv *pEnv, jstring file_path) {
    int ret = -1;
    const char *filePathChars = pEnv->GetStringUTFChars(file_path, nullptr);
    mFilePath = (char *) malloc(sizeof(char) * (int) strlen(filePathChars));
    strcpy(this->mFilePath, filePathChars);
    pEnv->ReleaseStringUTFChars(file_path, filePathChars);
    bool isSuccess = false;
    do {
        mFormatContext = avformat_alloc_context();
        if (!mFormatContext) {
            isSuccess = false;
            break;
        }
        ret = avformat_open_input(&mFormatContext, this->mFilePath, nullptr, nullptr);
        if (ret < 0) {
            isSuccess = false;
            break;
        }
        ret = avformat_find_stream_info(mFormatContext, nullptr);
        if (ret < 0) {
            isSuccess = false;
            break;
        }
        isSuccess = true;
    } while (false);

    if (!isSuccess && mFormatContext) {
        avformat_close_input(&mFormatContext);
        mFormatContext = nullptr;
    }
}

void SoftVideoDecoder::release(JNIEnv *pEnv) {
    if (mFormatContext) {
        avformat_close_input(&mFormatContext);
        mFormatContext = nullptr;
    }
    if (this->mFilePath) {
        free(this->mFilePath);
        this->mFilePath = nullptr;
    }
    this->mDecodeFrameIndex = 0;
}

jobject SoftVideoDecoder::getMediaFormat(JNIEnv *jniEnv) {
    if (mFormatContext) {
        jclass mediaFormatClass = jniEnv->FindClass("android/media/MediaFormat");
        jmethodID mediaFormatConstructor = jniEnv->GetMethodID(mediaFormatClass, "<init>", "()V");
        jobject mediaFormatInstance = jniEnv->NewObject(mediaFormatClass, mediaFormatConstructor);
        for (int i = 0; i < mFormatContext->nb_streams; i++) {
            AVStream *stream = mFormatContext->streams[i];
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


void SoftVideoDecoder::startDecode(jobject javaSurface, JNIEnv *jniEnv, jobject javaInstance) {
    AVCodecContext *codecContext = nullptr;
    AVPacket *packet = nullptr;
    AVPixelFormat swsDstPixelFormat = AV_PIX_FMT_RGBA;
    int ret = -1;
    int videoStreamIndex = -1;
    if (mFormatContext->nb_streams <= 0) {
        goto clear;
    }
    for (int i = 0; i < mFormatContext->nb_streams; i++) {
        AVStream *stream = mFormatContext->streams[i];
        if (!stream) {
            continue;
        }
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            mVideoStream = stream;
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
    __android_log_print(ANDROID_LOG_ERROR, "SoftVideoDecoder", "video stream index: %d",
                        videoStreamIndex);
    packet = av_packet_alloc();
    av_init_packet(packet);
    if (!packet) {
        goto clear;
    }
    while (!mIsStop) {
        //解封装code
        ret = av_read_frame(mFormatContext, packet);
        if (ret) {
            __android_log_print(ANDROID_LOG_ERROR, "SoftVideoDecoder", "read decodeFrame error: %s",
                                av_err2str(ret));
            //读到了多媒体文件的尾部，需要清空解码缓冲
            //清空解码缓冲区
            avcodec_send_packet(codecContext, nullptr);
            decodeAndRender(jniEnv, javaSurface, codecContext, swsDstPixelFormat, javaInstance);
            break;
        }
        __android_log_print(ANDROID_LOG_ERROR, "SoftVideoDecoder",
                            "packet stream index: %d packet size: %d",
                            packet->stream_index, packet->size);
        if (packet->stream_index != videoStreamIndex) {
            av_packet_unref(packet);
            continue;
        }
        //解码
        ret = avcodec_send_packet(codecContext, packet);
        if (ret < 0) {
            __android_log_print(ANDROID_LOG_ERROR, "SoftVideoDecoder", "send packet error: %s",
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
        avcodec_free_context(&codecContext);
    }

}

void SoftVideoDecoder::decodeAndRender(JNIEnv *pEnv, jobject javaSurface, AVCodecContext *codecContext,
                                       AVPixelFormat swsDstPixelFormat, jobject javaInstance) {
    SwsContext *swsContext = nullptr;
    AVFrame *decodeFrame = av_frame_alloc();
    if (decodeFrame == nullptr) {
        return;
    }
    jclass softDecoderClass = pEnv->GetObjectClass(javaInstance);
    jmethodID onVideoDecodeMethod = pEnv->GetMethodID(softDecoderClass, "onVideoDecode", "(IF)V");
    jclass surfaceClass = pEnv->FindClass("android/view/Surface");
    jmethodID surfaceIsValidMethodId = pEnv->GetMethodID(surfaceClass, "isValid", "()Z");
    int ret = -1;
    do {
        if (!pEnv->CallBooleanMethod(javaSurface,surfaceIsValidMethodId)) {
            break;
        }
        ret = avcodec_receive_frame(codecContext, decodeFrame);
        if (ret) {
            __android_log_print(ANDROID_LOG_ERROR, "SoftVideoDecoder",
                                "receive decodeFrame error: %s",
                                av_err2str(ret));
            if (ret == AVERROR_EOF) {
                break;
            } else {
                continue;
            }
        } else {
            __android_log_print(ANDROID_LOG_ERROR, "SoftVideoDecoder",
                                "decode decodeFrame width: %d height: %d frameSize: %d",
                                decodeFrame->width,
                                decodeFrame->height,
                                decodeFrame->pkt_size
            );
        }

        pEnv->CallVoidMethod(javaInstance, onVideoDecodeMethod, this->mDecodeFrameIndex++,
                             (float) (decodeFrame->pts * av_q2d(this->mVideoStream->time_base)));
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
        __android_log_print(ANDROID_LOG_ERROR, "SoftVideoDecoder",
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
    av_frame_free(&decodeFrame);
}

void SoftVideoDecoder::stop() {
    this->mIsStop = true;
}


