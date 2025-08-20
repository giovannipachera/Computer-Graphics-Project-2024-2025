#include "AudioPlayer.hpp"
#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#else
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#endif

namespace {
std::mutex g_playersMutex;
std::vector<AudioPlayer*> g_players;

void stopAllPlayers() {
  std::lock_guard<std::mutex> lock(g_playersMutex);
  for (auto* p : g_players) {
    if (p) p->stop(false);
  }
}

void handleSignal(int sig) {
  stopAllPlayers();
  std::_Exit(128 + sig);
}
} // namespace

struct AudioPlayer::Impl {
  std::string path;
  bool loop = false;
  std::atomic<bool> playing{false};
  std::thread worker;

#ifdef _WIN32
  // Alias MCI univoco per istanza
  std::string mciAlias;

  static std::string toLowerExt(const std::string& p) {
    auto pos = p.find_last_of('.');
    std::string ext = (pos == std::string::npos) ? "" : p.substr(pos + 1);
    for (auto& c : ext) c = (char)std::tolower((unsigned char)c);
    return ext;
  }

  static std::string guessMciType(const std::string& p) {
    const std::string ext = toLowerExt(p);
    if (ext == "wav")  return "waveaudio";
    if (ext == "mp3")  return "mpegvideo";
    if (ext == "wma")  return "mpegvideo";
    if (ext == "aac" || ext == "m4a") return "mpegvideo";
    // Fallback: lasciamo che MCI provi a dedurre
    return "";
  }

  static bool mciOk(const std::string& cmd) {
    MCIERROR err = mciSendStringA(cmd.c_str(), nullptr, 0, nullptr);
    return err == 0;
  }

  void mciClose() {
    if (!mciAlias.empty()) {
      std::string stopCmd  = "stop "  + mciAlias;
      std::string closeCmd = "close " + mciAlias;
      mciSendStringA(stopCmd.c_str(),  nullptr, 0, nullptr);
      mciSendStringA(closeCmd.c_str(), nullptr, 0, nullptr);
      mciAlias.clear();
    }
  }

  void run() {
    // Prepara alias univoco
    if (mciAlias.empty()) {
      mciAlias = "ap_" + std::to_string(reinterpret_cast<uintptr_t>(this));
    }

    // Chiudi eventuale device precedente (robusto contro play multipli)
    mciClose();

    // Determina tipo MCI (per MP3/WAV)
    std::string mciType = guessMciType(path);

    // Apri file
    std::string openCmd = "open \"" + path + "\"";
    if (!mciType.empty()) openCmd += " type " + mciType;
    openCmd += " alias " + mciAlias;

    if (!mciOk(openCmd)) {
      playing = false; // non riesce ad aprire: fermiamo
      return;
    }

    // Avvia playback (loop con "repeat")
    std::string playCmd = "play " + mciAlias + (loop ? " repeat" : "");
    if (!mciOk(playCmd)) {
      mciClose();
      playing = false;
      return;
    }

    // Mantieni vivo il thread finché playing è true
    while (playing) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop e close puliti
    mciClose();
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
      baseCmd += " >/dev/null 2>&1"; // silenzia ffplay

    do {
      pid = fork();
      if (pid == 0) {
#ifdef __linux__
        // se il parent muore, uccidi il player
        prctl(PR_SET_PDEATHSIG, SIGKILL);
#endif
        setpgid(0, 0); // nuovo process group
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

AudioPlayer::AudioPlayer() : impl(new Impl) {
  std::lock_guard<std::mutex> lock(g_playersMutex);
  if (g_players.empty()) {
    std::atexit(stopAllPlayers);
    std::signal(SIGINT,  handleSignal);
    std::signal(SIGTERM, handleSignal);
  }
  g_players.push_back(this);
}

AudioPlayer::~AudioPlayer() {
  stop(true);
  {
    std::lock_guard<std::mutex> lock(g_playersMutex);
    auto it = std::find(g_players.begin(), g_players.end(), this);
    if (it != g_players.end()) g_players.erase(it);
  }
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

#ifdef _WIN32
  // Prepara alias prima di lanciare il thread (così stop() sa cosa chiudere subito)
  impl->mciAlias = "ap_" + std::to_string(reinterpret_cast<uintptr_t>(this));
#endif

  impl->worker = std::thread([this] { impl->run(); });
  return true;
}

void AudioPlayer::stop(bool wait) {
  if (!impl->playing) return;
  impl->playing = false;

#ifdef _WIN32
  // Ferma immediatamente il device MCI se già aperto
  if (!impl->mciAlias.empty()) {
    std::string stopCmd  = "stop "  + impl->mciAlias;
    std::string closeCmd = "close " + impl->mciAlias;
    mciSendStringA(stopCmd.c_str(),  nullptr, 0, nullptr);
    mciSendStringA(closeCmd.c_str(), nullptr, 0, nullptr);
    impl->mciAlias.clear();
  }
#else
  if (impl->pid > 0) {
    // uccidi tutto il process group
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
