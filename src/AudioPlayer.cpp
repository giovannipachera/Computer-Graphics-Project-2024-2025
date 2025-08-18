#include "AudioPlayer.hpp"
#include <atomic>
#include <cstdlib>
#include <string>
#include <thread>

struct AudioPlayer::Impl {
    std::string path;
    bool loop = false;
    std::atomic<bool> playing{false};
    std::thread worker;
    std::string playerCmd;

    bool findPlayer() {
        const std::string candidates[] = {
            "ffplay -nodisp -autoexit", // ffmpeg
            "aplay",                    // ALSA
            "afplay",                   // macOS
            "open"                      // fallback (macOS)
        };
        for (const auto& base : candidates) {
            std::string program = base.substr(0, base.find(' '));
            std::string cmd = "which " + program + " > /dev/null 2>&1";
            if (std::system(cmd.c_str()) == 0) {
                playerCmd = base;
                return true;
            }
        }
        return false;
    }

    void run() {
        if (playerCmd.empty() && !findPlayer()) {
            playing = false;
            return;
        }
      
        std::string baseCmd = playerCmd + " \"" + path + "\"";
        if (playerCmd.find("ffplay") != std::string::npos)
            baseCmd += " >/dev/null 2>&1"; // silence ffplay output

        do {
            std::system(baseCmd.c_str());
        } while (loop && playing);
      
        playing = false;
    }
};

AudioPlayer::AudioPlayer() : impl(new Impl) {}

AudioPlayer::~AudioPlayer() {
    stop();
    delete impl;
}

bool AudioPlayer::load(const std::string& path, bool loop) {
    impl->path = path;
    impl->loop = loop;
    return true;
}

bool AudioPlayer::play() {
    if (impl->path.empty() || impl->playing) return false;
    impl->playing = true;
    impl->worker = std::thread([this]{ impl->run(); });
    return true;
}

void AudioPlayer::stop() {
    if (!impl->playing) return;
    impl->playing = false;
    if (!impl->playerCmd.empty()) {
        std::string prog = impl->playerCmd.substr(0, impl->playerCmd.find(' '));
        std::string cmd = "pkill -f \"" + prog + "\" > /dev/null 2>&1";
        std::system(cmd.c_str());
    }
    if (impl->worker.joinable()) impl->worker.join();
}

void AudioPlayer::setLoop(bool loop) { impl->loop = loop; }

bool AudioPlayer::isPlaying() const { return impl->playing; }

