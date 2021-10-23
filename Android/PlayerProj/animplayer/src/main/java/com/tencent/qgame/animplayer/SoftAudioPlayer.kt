package com.tencent.qgame.animplayer

import android.media.*
import com.tencent.qgame.animplayer.file.FileContainer
import com.tencent.qgame.animplayer.file.IFileContainer
import com.tencent.qgame.animplayer.util.ALog
import java.lang.RuntimeException
import java.lang.UnsupportedOperationException

/**
 * 使用软解码+OpenSL进行音频播放
 */
class SoftAudioPlayer(val player: AnimPlayer) : BaseAudioPlayer() {

    companion object {
        private const val TAG = "SoftAudioDecoder"

        init {
            System.loadLibrary("native-lib")
        }
    }

    private external fun nativeInit();
    private var nativeInstance: Long = -1

    val decodeThread = HandlerHolder(null, null)
    var isRunning = false
    var isStopReq = false
    var needDestroy = false


    private fun prepareThread(): Boolean {
        return Decoder.createThread(decodeThread, "anim_audio_thread")
    }

    override fun start(fileContainer: IFileContainer) {
        nativeInit()
        isStopReq = false
        needDestroy = false
        if (!prepareThread()) return
        if (isRunning) {
            stop()
        }
        isRunning = true
        decodeThread.handler?.post {
            try {
                startPlay(fileContainer)
            } catch (e: Throwable) {
                ALog.e(TAG, "Audio exception=$e", e)
                release()
            }
        }
    }

    override fun stop() {
        isStopReq = true
        nativeStop(nativeInstance);
    }

    private external fun nativeStop(nativeInstance: Long)

    private fun startPlay(fileContainer: IFileContainer) {
        if (!(fileContainer is FileContainer)) {
            throw UnsupportedOperationException("not support file container type $fileContainer")
        }
        nativeStartPlay(nativeInstance, fileContainer.filePath)
        release()
    }

    private external fun nativeStartPlay(nativeInstance: Long, filePath: String?)


    private fun release() {
        try {
            stop()
        } catch (e: Throwable) {
            ALog.e(TAG, "release exception=$e", e)
        }
        nativeInstance = -1
        isRunning = false
        if (needDestroy) {
            destroyInner()
        }
    }

    override fun destroy() {
        needDestroy = true
        if (isRunning) {
            stop()
        } else {
            destroyInner()
        }
    }

    private fun destroyInner() {
        if (player.isDetachedFromWindow) {
            ALog.i(TAG, "destroyThread")
            decodeThread.handler?.removeCallbacksAndMessages(null)
            decodeThread.thread = Decoder.quitSafely(decodeThread.thread)
        }
    }

}