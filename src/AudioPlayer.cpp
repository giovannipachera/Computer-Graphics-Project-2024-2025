// AudioPlayer.cpp — versione robusta Windows + *nix
#include "AudioPlayer.hpp"
#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <cstdio>
#include <cwctype>
#include <filesystem>
#include <iostream>





#ifdef _WIN32
  #define NOMINMAX
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
  // === Windows (MCI wide) ===
  std::wstring mciAliasW;
  std::wstring pathW;

  static std::wstring toWide(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(len ? len - 1 : 0, L'\0');
    if (len) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], len);
    return w;
  }

  static std::wstring lowerExt(const std::wstring& p) {
    size_t pos = p.find_last_of(L'.');
    std::wstring ext = (pos == std::wstring::npos) ? L"" : p.substr(pos + 1);
    for (auto& c : ext) c = (wchar_t)std::towlower(c);
    return ext;
  }

  static std::wstring guessMciType(const std::wstring& p) {
    auto ext = lowerExt(p);
    if (ext == L"wav")  return L"waveaudio";
    if (ext == L"mp3")  return L"mpegvideo";
    if (ext == L"wma")  return L"mpegvideo";
    if (ext == L"aac" || ext == L"m4a") return L"mpegvideo";
    return L""; // lascia dedurre a MCI
  }

  static bool mciOk(const std::wstring& cmd, const char* ctx = nullptr) {
    MCIERROR err = mciSendStringW(cmd.c_str(), nullptr, 0, nullptr);
    if (err != 0) {
      wchar_t buf[256]{};
      mciGetErrorStringW(err, buf, 255);
      char mb[512];
      int n = WideCharToMultiByte(CP_UTF8, 0, buf, -1, mb, sizeof(mb), nullptr, nullptr);
      if (n > 0) {
        char line[1024];
        std::snprintf(line, sizeof(line), "[AUDIO][MCI ERROR]%s%s%s -> %s\n",
                      ctx ? " (" : "", ctx ? ctx : "", ctx ? ")" : "", mb);
        OutputDebugStringA(line);
        std::fwrite(line, 1, strlen(line), stderr);
      }
      return false;
    }
    return true;
  }

  void mciClose() {
    if (!mciAliasW.empty()) {
      std::wstring stopCmd  = L"stop "  + mciAliasW;
      std::wstring closeCmd = L"close " + mciAliasW;
      mciSendStringW(stopCmd.c_str(),  nullptr, 0, nullptr);
      mciSendStringW(closeCmd.c_str(), nullptr, 0, nullptr);
      mciAliasW.clear();
    }
  }

  // Fallback solo per WAV (sincrono ma rodato)
  bool tryPlaySoundLoopIfWav() {
    auto ext = lowerExt(pathW);
    if (ext != L"wav") return false;

    // 🔹 Debug: controlla se il file esiste
    if (!std::filesystem::exists(path)) {
      OutputDebugStringA("[AUDIO][DEBUG] File WAV non trovato!\n");
      return false;
    }

    DWORD flags = SND_FILENAME | SND_ASYNC | (loop ? SND_LOOP : 0);

    // 🔹 Debug: prova PlaySoundW e segnala se fallisce
    if (!PlaySoundW(pathW.c_str(), nullptr, flags)) {
      OutputDebugStringA("[AUDIO][DEBUG] PlaySoundW fallito!\n");
      return false; // esci subito invece di continuare il loop
    }

    OutputDebugStringA("[AUDIO][DEBUG] PlaySoundW partito correttamente\n");

    while (playing) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    PlaySoundW(nullptr, nullptr, 0); // stop
    OutputDebugStringA("[AUDIO][DEBUG] PlaySoundW fermato\n");
    return true;
  }


  void run() {
    pathW = toWide(path);

    std::wstring wpath = toWide(path);
    std::wcout << L"[DEBUG] File wide path: " << wpath << std::endl;





    // Prova subito con PlaySound per WAV (robustissimo)
    if (tryPlaySoundLoopIfWav()) {
      return;
    }

    // Prepara alias univoco
    if (mciAliasW.empty()) {
      wchar_t temp[64];
      std::swprintf(temp, 64, L"ap_%p", (void*)this);
      mciAliasW = temp;
    }

    // Chiudi eventuale device precedente
    mciClose();

    std::wstring mciType = guessMciType(pathW);

    // Apri file
    std::wstring openCmd = L"open \"" + pathW + L"\"";
    if (!mciType.empty()) openCmd += L" type " + mciType;
    openCmd += L" alias " + mciAliasW + L" shareable";
    if (!mciOk(openCmd, "open")) {
      playing = false;
      return;
    }

    // Imposta time format (alcuni mpegvideo lo richiedono)
    if (!mciOk(L"set " + mciAliasW + L" time format milliseconds", "set time format")) {
      mciClose();
      playing = false;
      return;
    }

    // Avvio
    std::wstring playCmd = L"play " + mciAliasW + (loop ? L" repeat" : L"");
    if (!mciOk(playCmd, "play")) {
      // Tentiamo un seek e riproviamo una volta
      mciOk(L"seek " + mciAliasW + L" to start", "seek start (retry)");
      if (!mciOk(playCmd, "play (retry)")) {
        mciClose();
        playing = false;
        return;
      }
    }

    // Mantieni vivo il thread finché playing è true
    // (Se repeat non funzionasse per il codec, riavviamo manualmente)
    auto lastCheck = std::chrono::steady_clock::now();
    while (playing) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      if (!loop) continue;

      // ogni 500ms controlla lo stato
      auto now = std::chrono::steady_clock::now();
      if (now - lastCheck < std::chrono::milliseconds(500)) continue;
      lastCheck = now;

      wchar_t mode[64]{};
      if (mciSendStringW((L"status " + mciAliasW + L" mode").c_str(), mode, 63, nullptr) == 0) {
        if (std::wcscmp(mode, L"stopped") == 0) {
          // loop manuale
          mciOk(L"seek " + mciAliasW + L" to start", "seek loop");
          mciOk(L"play " + mciAliasW, "play loop");
        }
      }
    }

    // Stop e close puliti
    mciClose();
  }
#else
  // === Unix/macOS (processo esterno) ===
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
#ifdef SIGABRT
    std::signal(SIGABRT, handleSignal);
#endif
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
  // Prepara alias subito (utile se stop() arriva prestissimo)
  if (impl->mciAliasW.empty()) {
    wchar_t temp[64];
    std::swprintf(temp, 64, L"ap_%p", (void*)impl);
    impl->mciAliasW = temp;
  }
#endif

  impl->worker = std::thread([this] { impl->run(); });
  return true;
}

void AudioPlayer::stop(bool wait) {
  if (!impl->playing) return;
  impl->playing = false;

#ifdef _WIN32
  // Ferma immediatamente il device MCI se aperto
  if (!impl->mciAliasW.empty()) {
    std::wstring stopCmd  = L"stop "  + impl->mciAliasW;
    std::wstring closeCmd = L"close " + impl->mciAliasW;
    mciSendStringW(stopCmd.c_str(),  nullptr, 0, nullptr);
    mciSendStringW(closeCmd.c_str(), nullptr, 0, nullptr);
    impl->mciAliasW.clear();
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
