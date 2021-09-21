#include <jni.h>
#include <string>
#include <softdecoder/SoftDecoder.h>

extern "C"
JNIEXPORT void JNICALL
Java_com_tencent_qgame_animplayer_SoftDecoder_nativeInit(JNIEnv *env, jobject thiz) {
    jclass clazz = env->GetObjectClass(thiz);
    SoftDecoder *softDecoder = new SoftDecoder();
    jfieldID nativeInstanceId = env->GetFieldID(clazz, "nativeInstance", "J");
    env->SetLongField(thiz, nativeInstanceId, reinterpret_cast<jlong>(softDecoder));
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tencent_qgame_animplayer_SoftDecoder_nativeStartDecode(JNIEnv *env, jobject thiz,
                                                                jlong native_instance,
                                                                jobject surface,
                                                                jstring file_path) {
    SoftDecoder * softDecoder = (SoftDecoder*)native_instance;
    softDecoder->startDecode(surface, file_path);

}
extern "C"
JNIEXPORT jobject JNICALL
Java_com_tencent_qgame_animplayer_SoftDecoder_nativeGetMediaFormat(JNIEnv *env, jobject thiz,
                                                                   jlong native_instance) {
    SoftDecoder *softDecoder = (SoftDecoder *) native_instance;
    return softDecoder->getMediaFormat(env);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_tencent_qgame_animplayer_SoftDecoder_nativeRelease(JNIEnv *env, jobject thiz,
                                                            jlong native_instance) {
    jclass clazz = env->GetObjectClass(thiz);
    jfieldID nativeInstanceId = env->GetFieldID(clazz, "nativeInstance", "J");
    env->SetLongField(thiz, nativeInstanceId, (jlong) 0);
    SoftDecoder *softDecoder = (SoftDecoder *) native_instance;
    softDecoder->release();
    delete softDecoder;
}