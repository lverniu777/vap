package com.tencent.qgame.animplayer;

public class AudioPlayerFactory {

    public static BaseAudioPlayer getInstance(AnimPlayer animPlayer) {
        final BaseAudioPlayer audioPlayer;
        //软解降级
        if (true) {
            audioPlayer = new SoftAudioPlayer(animPlayer);
            audioPlayer.setPlayLoop(animPlayer.getPlayLoop());
        } else {
            audioPlayer = new HardAudioPlayer(animPlayer);
            audioPlayer.setPlayLoop(animPlayer.getPlayLoop());
        }
        return audioPlayer;
    }

}
