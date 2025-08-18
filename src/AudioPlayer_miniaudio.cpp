#include "AudioPlayer.hpp"
#include <string>

// Abilita i decoder che usi (definiscili PRIMA di includere miniaudio.h)
#define MA_ENABLE_MP3
#define MA_ENABLE_FLAC
#define MA_ENABLE_VORBIS

// Singola definizione dell'implementazione (evitiamo duplicati)
#define MINIAUDIO_IMPLEMENTATION
#include "../external/miniaudio.h"

struct AudioPlayer::Impl {
    ma_engine engine{};
    ma_sound  sound{};
    bool      hasSound = false;
    bool      loop = false;
    bool      engineOk = false;
};

AudioPlayer::AudioPlayer() : impl(new Impl{}) {
    ma_engine_config cfg = ma_engine_config_init();
    // usa backend di sistema; se vuoi forzarne uno: cfg.backend = ma_backend_wasapi/alsa/coreaudio...
    ma_result r = ma_engine_init(&cfg, &impl->engine);
    impl->engineOk = (r == MA_SUCCESS);
}

AudioPlayer::~AudioPlayer() {
    if (impl->hasSound) { ma_sound_uninit(&impl->sound); impl->hasSound = false; }
    if (impl->engineOk) { ma_engine_uninit(&impl->engine); impl->engineOk = false; }
    delete impl;
}

static const char* ma_errstr(ma_result r) {
    // miniaudio fornisce una descrizione testuale
    return ma_result_description(r);
}

bool AudioPlayer::load(const std::string& path, bool loop) {
    if (!impl->engineOk) return false;
    if (impl->hasSound) {
        ma_sound_uninit(&impl->sound);
        impl->hasSound = false;
    }

    // Nota: evitiamo MA_SOUND_FLAG_DECODE: alcuni ambienti hanno problemi con il resource manager
    ma_uint32 flags = 0; // era: MA_SOUND_FLAG_DECODE
    ma_result r = ma_sound_init_from_file(&impl->engine, path.c_str(),
                                          flags,
                                          /*pGroup*/   nullptr,
                                          /*onInitCB*/ nullptr,
                                          &impl->sound);

    if (r != MA_SUCCESS) {
        // stampa l'errore in stderr (verrà visto dal chiamante nei log)
        fprintf(stderr, "[AUDIO][ERR] ma_sound_init_from_file('%s') -> %d (%s)\n",
                path.c_str(), r, ma_errstr(r));
        return false;
    }

    impl->loop = loop;
    ma_sound_set_looping(&impl->sound, loop ? MA_TRUE : MA_FALSE);
    impl->hasSound = true;
    return true;
}

bool AudioPlayer::play() {
    if (!impl->engineOk || !impl->hasSound) return false;
    ma_result r = ma_sound_start(&impl->sound);
    if (r != MA_SUCCESS) {
        fprintf(stderr, "[AUDIO][ERR] ma_sound_start() -> %d (%s)\n", r, ma_errstr(r));
        return false;
    }
    return true;
}

void AudioPlayer::stop() {
    if (!impl->hasSound) return;
    ma_sound_stop(&impl->sound);
    ma_sound_uninit(&impl->sound);
    impl->hasSound = false;
}

void AudioPlayer::setLoop(bool loop) {
    impl->loop = loop;
    if (impl->hasSound) ma_sound_set_looping(&impl->sound, loop ? MA_TRUE : MA_FALSE);
}

bool AudioPlayer::isPlaying() const {
    if (!impl->hasSound) return false;
    return ma_sound_is_playing(&impl->sound) == MA_TRUE;
}
