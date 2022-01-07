package com.tencent.qgame.animplayer;

public class DecoderFactory {

    public static Decoder getInstance(AnimPlayer animPlayer) {
        final Decoder decoder;
        if (animPlayer.getVideoDecodeType() == VideoDecodeType.HARD_DECODE) {
            decoder = new HardVideoDecoder(animPlayer);
        } else {
            decoder = new SoftVideoDecoder(animPlayer);
        }
        decoder.setPlayLoop(animPlayer.getPlayLoop());
        decoder.setFps(animPlayer.getFps());
        return decoder;
    }
}
