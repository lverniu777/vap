//
// Created by huaxia on 2021/9/21.
//

#ifndef PLAYERPROJ_SOFTDECODER_H
#define PLAYERPROJ_SOFTDECODER_H
#include <jni.h>

class SoftDecoder {
public:
    SoftDecoder();
    void init();
    void release();
    jobject getMediaFormat(JNIEnv * jniEnv);
    void startDecode(jobject surface, jstring filePath);
};


#endif //PLAYERPROJ_SOFTDECODER_H
