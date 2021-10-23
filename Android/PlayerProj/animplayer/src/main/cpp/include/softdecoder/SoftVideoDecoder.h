//
// Created by huaxia on 2021/9/21.
//

#ifndef PLAYERPROJ_SOFTVIDEODECODER_H
#define PLAYERPROJ_SOFTVIDEODECODER_H

#include <jni.h>

extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
};

class SoftVideoDecoder {
private:
    int mDecodeFrameIndex = 0;
    char *mFilePath = nullptr;
    AVFormatContext *mFormatContext = nullptr;
    AVStream *mVideoStream = nullptr;
    bool mIsStop = false;

    void decodeAndRender(JNIEnv *pEnv, jobject javaSurface, AVCodecContext *codecContext,
                         AVPixelFormat swsDstPixelFormat, jobject javaInstance);

public:
    SoftVideoDecoder();

    void init(JNIEnv *pEnv, jstring pJstring);

    void release(JNIEnv *pEnv);

    jobject getMediaFormat(JNIEnv *jniEnv);

    void startDecode(jobject javaSurface, JNIEnv *jniEnv, jobject javaInstance);

    void stop();
};


#endif //PLAYERPROJ_SOFTVIDEODECODER_H
