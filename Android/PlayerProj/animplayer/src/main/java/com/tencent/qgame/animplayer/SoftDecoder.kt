/*
 * Tencent is pleased to support the open source community by making vap available.
 *
 * Copyright (C) 2020 THL A29 Limited, a Tencent company.  All rights reserved.
 *
 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 *
 * http://opensource.org/licenses/MIT
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.tencent.qgame.animplayer

import android.graphics.SurfaceTexture
import android.media.MediaCodec
import android.media.MediaFormat
import android.util.Log
import android.view.Surface
import com.tencent.qgame.animplayer.file.FileContainer
import com.tencent.qgame.animplayer.file.IFileContainer
import com.tencent.qgame.animplayer.util.ALog
import java.lang.UnsupportedOperationException

/**
 * 软解
 */
class SoftDecoder(player: AnimPlayer) : Decoder(player), SurfaceTexture.OnFrameAvailableListener {

    companion object {
        private const val TAG = "SoftDecoder"

        init {
            System.loadLibrary("native-lib")
        }
    }


    private external fun nativeInit();


    private var nativeInstance: Long = -1
    private var glTexture: SurfaceTexture? = null
    private val bufferInfo by lazy { MediaCodec.BufferInfo() }
    private var needDestroy = false

    // 动画的原始尺寸
    private var videoWidth = 0
    private var videoHeight = 0

    // 动画对齐后的尺寸
    private var alignWidth = 0
    private var alignHeight = 0
    private var outputFormat: MediaFormat? = null

    override fun start(fileContainer: IFileContainer) {
        nativeInit();
        isStopReq = false
        needDestroy = false
        isRunning = true
        renderThread.handler?.post {
            startPlay(fileContainer)
        }
    }

    override fun onFrameAvailable(surfaceTexture: SurfaceTexture?) {
        if (isStopReq) return
        ALog.d(TAG, "onFrameAvailable")
        renderData()
    }

    fun renderData() {
        renderThread.handler?.post {
            try {
                glTexture?.apply {
                    updateTexImage()
                    render?.renderFrame()
                    player.pluginManager.onRendering()
                    render?.swapBuffers()
                }
            } catch (e: Throwable) {
                ALog.e(TAG, "render exception=$e", e)
            }
        }
    }

    private fun startPlay(fileContainer: IFileContainer) {
        if (!(fileContainer is FileContainer)) {
            throw UnsupportedOperationException("not support file container type $fileContainer")
        }
        nativeOnStartPlay(nativeInstance, fileContainer.filePath)
        val format: MediaFormat?
        try {
            format = nativeGetMediaFormat(nativeInstance)
            if (format == null) throw RuntimeException("format is null")
            videoWidth = format.getInteger(MediaFormat.KEY_WIDTH)
            videoHeight = format.getInteger(MediaFormat.KEY_HEIGHT)
            // 防止没有INFO_OUTPUT_FORMAT_CHANGED时导致alignWidth和alignHeight不会被赋值一直是0
            alignWidth = videoWidth
            alignHeight = videoHeight
            ALog.i(TAG, "Video size is $videoWidth x $videoHeight")

            try {
                if (!prepareRender(false)) {
                    throw RuntimeException("render create fail")
                }
            } catch (t: Throwable) {
                onFailed(
                    Constant.REPORT_ERROR_TYPE_CREATE_RENDER,
                    "${Constant.ERROR_MSG_CREATE_RENDER} e=$t"
                )
                release()
                return
            }

            preparePlay(videoWidth, videoHeight)

            render?.apply {
                glTexture = SurfaceTexture(getExternalTexture()).apply {
                    setOnFrameAvailableListener(this@SoftDecoder)
                    setDefaultBufferSize(videoWidth, videoHeight)
                }
                clearFrame()
            }

        } catch (e: Throwable) {
            ALog.e(TAG, "MediaExtractor exception e=$e", e)
            onFailed(
                Constant.REPORT_ERROR_TYPE_EXTRACTOR_EXC,
                "${Constant.ERROR_MSG_EXTRACTOR_EXC} e=$e"
            )
            release()
            return
        }

        val mime = format.getString(MediaFormat.KEY_MIME) ?: ""
        ALog.i(TAG, "Video MIME is $mime")
        decodeThread.handler?.post {
            val surface = Surface(glTexture);
            try {
                if (fileContainer is FileContainer) {
                    nativeStartDecode(nativeInstance, surface)
                } else {
                    throw UnsupportedOperationException("not support file container type $fileContainer")
                }
            } catch (e: Throwable) {
                ALog.e(TAG, "startDecode exception e=$e", e)
                onFailed(
                    Constant.REPORT_ERROR_TYPE_DECODE_EXC,
                    "${Constant.ERROR_MSG_DECODE_EXC} e=$e"
                )
                release()
            } finally {
                surface.release()
                release()
            }
        }
    }

    override fun stop() {
        super.stop()
        nativeStop(nativeInstance)
    }

    private external fun nativeStop(nativeInstance: Long);

    private external fun nativeOnStartPlay(nativeInstance: Long, filePath: String?);

    private external fun nativeStartDecode(nativeInstance: Long, surface: Surface)


    private external fun nativeGetMediaFormat(nativeInstance: Long): MediaFormat?

    /**
     * native解码完成后回调回来
     */
    private fun onVideoDecode(frameIndex: Int, pts: Float) {
        Log.e(TAG, "onVideoDecode: frameIndex${frameIndex} pts${pts}")
        val ptsInUs = pts * 1000_000
        speedControlUtil.preRender(ptsInUs.toLong())
        if (frameIndex == 0) {
            onVideoStart()
        } else {
            player.pluginManager.onDecoding(frameIndex)
            onVideoRender(frameIndex, player.configManager.config)
        }
    }


    private fun release() {
        renderThread.handler?.post {
            render?.clearFrame()
            try {
                ALog.i(TAG, "release")
                nativeRelease(nativeInstance)
                nativeInstance = -1;
                glTexture?.release()
                glTexture = null
                speedControlUtil.reset()
                player.pluginManager.onRelease()
                render?.releaseTexture()
            } catch (e: Throwable) {
                ALog.e(TAG, "release e=$e", e)
            }
            isRunning = false
            onVideoComplete()
            if (needDestroy) {
                destroyInner()
            }
        }
    }

    private external fun nativeRelease(nativeInstance: Long);

    override fun destroy() {
        needDestroy = true
        if (isRunning) {
            stop()
        } else {
            destroyInner()
        }
    }

    private fun destroyInner() {
        renderThread.handler?.post {
            player.pluginManager.onDestroy()
            render?.destroyRender()
            render = null
            onVideoDestroy()
            destroyThread()
        }
    }
}
