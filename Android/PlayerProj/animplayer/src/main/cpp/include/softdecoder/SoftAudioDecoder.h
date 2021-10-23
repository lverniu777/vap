//
// Created by huaxia on 2021/10/23.
//

#ifndef PLAYERPROJ_SOFTAUDIODECODER_H
#define PLAYERPROJ_SOFTAUDIODECODER_H

#include <jni.h>
#include "SoftVideoDecoder.h"

class SoftAudioDecoder {
private:
    bool mIsStop;
public:
    void nativeStartPlay(JNIEnv *, jstring);

    void stop();

    void decodePCMAndPlay(AVCodecContext *pContext);
};


#endif //PLAYERPROJ_SOFTAUDIODECODER_H
