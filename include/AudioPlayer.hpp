#pragma once
#include <string>

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    bool load(const std::string& path, bool loop=false); // mp3/wav/ogg/flac…
    bool play();     // non blocca, usa un player esterno disponibile
    void stop(bool wait = true);     // rilascia la traccia corrente (wait=false: non blocca)
    void setLoop(bool loop);
    bool isPlaying() const;

private:
    struct Impl; Impl* impl;
};
