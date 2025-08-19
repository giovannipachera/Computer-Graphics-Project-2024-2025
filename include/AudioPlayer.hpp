#pragma once
#include <memory>
#include <string>

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    bool load(const std::string& path, bool loop=false); // mp3/wav/ogg/flac…
    bool play();     // non blocca, usa un player esterno disponibile
    void stop();     // rilascia la traccia corrente
    void setLoop(bool loop);
    bool isPlaying() const;

private:
    struct Impl; std::shared_ptr<Impl> impl;
};
