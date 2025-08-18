#pragma once
#include <string>

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    bool load(const std::string& path, bool loop=false); // mp3/wav/ogg/flac…
    bool play();     // non blocca
    void stop();     // rilascia la traccia corrente
    void setLoop(bool loop);
    bool isPlaying() const;

private:
    struct Impl; Impl* impl;
};
