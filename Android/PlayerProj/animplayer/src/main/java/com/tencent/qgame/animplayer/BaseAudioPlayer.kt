package com.tencent.qgame.animplayer

import com.tencent.qgame.animplayer.file.IFileContainer

abstract class BaseAudioPlayer {

    abstract fun destroy()

    abstract fun start(fileContainer: IFileContainer)

    abstract fun stop()

    var playLoop: Int = 0;
}