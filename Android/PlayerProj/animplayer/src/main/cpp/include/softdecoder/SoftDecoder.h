//
// Created by huaxia on 2021/9/21.
//

#ifndef PLAYERPROJ_SOFTDECODER_H
#define PLAYERPROJ_SOFTDECODER_H

#include <jni.h>

extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
};

class SoftDecoder {
private:
    int mDecodeFrameIndex = 0;
    char *filePath = nullptr;
    AVFormatContext *formatContext = nullptr;
    bool mIsRelease = false;

    void decodeAndRender(JNIEnv *pEnv, jobject javaSurface, AVCodecContext *codecContext,
                         AVPixelFormat swsDstPixelFormat, jobject javaInstance);

public:
    SoftDecoder();

    void init(JNIEnv *pEnv, jstring pJstring);

    void release(JNIEnv *pEnv);

    jobject getMediaFormat(JNIEnv *jniEnv);

    void startDecode(jobject javaSurface, JNIEnv *jniEnv, jobject javaInstance);

};


#endif //PLAYERPROJ_SOFTDECODER_H
