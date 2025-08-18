#include "AudioPlayer.hpp"
#include <iostream>
#include <string>

struct AudioPlayer::Impl {
    std::string path; // stored path for reference
    bool loop = false;
    bool playing = false;
};

AudioPlayer::AudioPlayer() : impl(new Impl) {}

AudioPlayer::~AudioPlayer() {
    delete impl;
}

bool AudioPlayer::load(const std::string& path, bool loop) {
    impl->path = path;
    impl->loop = loop;
    return true; // Stub always succeeds
}

bool AudioPlayer::play() {
    if (impl->path.empty()) {
        return false;
    }
    impl->playing = true;
    // This stub just logs the action; actual audio playback requires external libs.
    std::cout << "[AudioPlayer] Playing " << impl->path
              << (impl->loop ? " in loop" : "") << std::endl;
    return true;
}

void AudioPlayer::stop() {
    if (!impl->playing) return;
    impl->playing = false;
    std::cout << "[AudioPlayer] Stopped " << impl->path << std::endl;
}

void AudioPlayer::setLoop(bool loop) {
    impl->loop = loop;
}

bool AudioPlayer::isPlaying() const {
    return impl->playing;
}

