#include "AudioPlayer.hpp"
#include <atomic>
#include <cstdlib>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#endif

struct AudioPlayer::Impl {
    std::string path;
    bool loop = false;
    std::atomic<bool> playing{false};
    std::thread worker;

#ifdef _WIN32
    void run() {
        UINT flags = SND_FILENAME | SND_ASYNC;
        if (loop) flags |= SND_LOOP;
        PlaySound(path.c_str(), NULL, flags);
        while (playing) std::this_thread::sleep_for(std::chrono::milliseconds(100));
        PlaySound(NULL, NULL, 0); // stop any playing sound
    }
#else
    std::string playerCmd;
    pid_t pid = -1;

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
            pid = fork();
            if (pid == 0) {
                setpgid(0, 0); // new process group so we can kill children
                execl("/bin/sh", "sh", "-c", baseCmd.c_str(), (char*)nullptr);
                _exit(127);
            } else if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
                pid = -1;
            } else {
                playing = false;
                break;
            }
        } while (loop && playing);

        playing = false;
    }
#endif
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

void AudioPlayer::stop(bool wait) {
    if (!impl->playing) return;
    impl->playing = false;
#ifdef _WIN32
    PlaySound(NULL, NULL, 0);
#else
    if (impl->pid > 0) {
        // kill the whole process group to terminate the player
        kill(-impl->pid, SIGKILL);
    }
#endif
    if (wait) {
        if (impl->worker.joinable()) impl->worker.join();
    } else {
        if (impl->worker.joinable()) impl->worker.detach();
    }
}

void AudioPlayer::setLoop(bool loop) { impl->loop = loop; }

bool AudioPlayer::isPlaying() const { return impl->playing; }

