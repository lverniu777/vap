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
    char *filePath = nullptr;
    AVFormatContext *formatContext = nullptr;
    bool mIsRelease = false;

    void decodeAndRender(JNIEnv *pEnv, jobject javaSurface,
                         AVCodecContext *codecContext,
                         AVPixelFormat swsDstPixelFormat);

public:
    SoftDecoder();

    void init(JNIEnv *pEnv, jstring pJstring);

    void release(JNIEnv *pEnv);

    jobject getMediaFormat(JNIEnv *jniEnv);

    void startDecode(JNIEnv *jniEnv, jobject pJobject);

};


#endif //PLAYERPROJ_SOFTDECODER_H
