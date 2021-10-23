#include <jni.h>
#include <string>
#include <softdecoder/SoftVideoDecoder.h>
#include <softdecoder/SoftAudioDecoder.h>

//----------------------------Video----------------------------//
extern "C"
JNIEXPORT void JNICALL
Java_com_tencent_qgame_animplayer_SoftVideoDecoder_nativeInit(JNIEnv *env, jobject thiz) {
    jclass clazz = env->GetObjectClass(thiz);
    SoftVideoDecoder *softDecoder = new SoftVideoDecoder();
    jfieldID nativeInstanceId = env->GetFieldID(clazz, "nativeInstance", "J");
    env->SetLongField(thiz, nativeInstanceId, reinterpret_cast<jlong>(softDecoder));
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tencent_qgame_animplayer_SoftVideoDecoder_nativeStartDecode(JNIEnv *env, jobject thiz,
                                                                     jlong native_instance,
                                                                     jobject surface) {
    if(native_instance == -1) {
        return;
    }
    SoftVideoDecoder *softDecoder = (SoftVideoDecoder *) native_instance;
    softDecoder->startDecode(surface, env, thiz);
//    jclass targetClass = env->FindClass("android/view/Surface");
//    env->GetMethodID(targetClass, "isValid", "()Z");
}
extern "C"
JNIEXPORT jobject JNICALL
Java_com_tencent_qgame_animplayer_SoftVideoDecoder_nativeGetMediaFormat(JNIEnv *env, jobject thiz,
                                                                        jlong native_instance) {
    if(native_instance == -1) {
        return nullptr;
    }
    SoftVideoDecoder *softDecoder = (SoftVideoDecoder *) native_instance;
    return softDecoder->getMediaFormat(env);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_tencent_qgame_animplayer_SoftVideoDecoder_nativeRelease(JNIEnv *env, jobject thiz,
                                                                 jlong native_instance) {
    if(native_instance == -1) {
        return;
    }
    jclass clazz = env->GetObjectClass(thiz);
    jfieldID nativeInstanceId = env->GetFieldID(clazz, "nativeInstance", "J");
    env->SetLongField(thiz, nativeInstanceId, (jlong) 0);
    SoftVideoDecoder *softDecoder = (SoftVideoDecoder *) native_instance;
    softDecoder->release(env);
    delete softDecoder;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tencent_qgame_animplayer_SoftVideoDecoder_nativeOnStartPlay(JNIEnv *env, jobject thiz,
                                                                     jlong native_instance,
                                                                     jstring file_path) {
    if(native_instance == -1) {
        return;
    }
    SoftVideoDecoder *softDecoder = (SoftVideoDecoder *) native_instance;
    softDecoder->init(env, file_path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tencent_qgame_animplayer_SoftVideoDecoder_nativeStop(JNIEnv *env, jobject thiz,
                                                              jlong native_instance) {
    if (native_instance == -1) {
        return;
    }
    SoftVideoDecoder *softDecoder = (SoftVideoDecoder *) native_instance;
    softDecoder->stop();
}

//------------------------------Audio-----------------------------//
extern "C"
JNIEXPORT void JNICALL
Java_com_tencent_qgame_animplayer_SoftAudioPlayer_nativeInit(JNIEnv *env, jobject thiz) {
    jclass clazz = env->GetObjectClass(thiz);
    SoftAudioDecoder *softAudioDecoder = new SoftAudioDecoder();
    jfieldID nativeInstanceId = env->GetFieldID(clazz, "nativeInstance", "J");
    env->SetLongField(thiz, nativeInstanceId, reinterpret_cast<jlong>(softAudioDecoder));
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tencent_qgame_animplayer_SoftAudioPlayer_nativeStartPlay(JNIEnv *env, jobject thiz,
                                                                  jlong native_instance,
                                                                  jstring file_path) {
    if(native_instance == -1) {
        return;
    }
    SoftAudioDecoder *softAudioDecoder = reinterpret_cast<SoftAudioDecoder *>(native_instance);
    softAudioDecoder->nativeStartPlay(env, file_path);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_tencent_qgame_animplayer_SoftAudioPlayer_nativeStop(JNIEnv *env, jobject thiz,
                                                             jlong native_instance) {
    if(native_instance == -1) {
        return;
    }
    SoftAudioDecoder *softAudioDecoder = reinterpret_cast<SoftAudioDecoder *>(native_instance);
    softAudioDecoder->stop();
}