package com.tencent.qgame.animplayer;

public class DecoderFactory {

    public static Decoder getInstance(AnimPlayer animPlayer) {
        //软硬解降级逻辑
        final Decoder decoder;
        if (false) {
            decoder = new HardDecoder(animPlayer);
        } else {
            decoder = new SoftDecoder(animPlayer);
        }
        decoder.setPlayLoop(animPlayer.getPlayLoop());
        decoder.setFps(animPlayer.getFps());
        return decoder;
    }
}
