// tts_buffer.h
// Gapless phoneme playback via pre-buffered PCM in RAM.
// All SD reads happen before playback starts, eliminating
// inter-phoneme gaps caused by SD card open latency.

#ifndef TTS_BUFFER_H
#define TTS_BUFFER_H

#include <AudioStream.h>

// 2.5 seconds at 16kHz 16-bit mono — enough for any word or short phrase
#define PCM_MAX_SAMPLES 40000

// ---------------------------------------------------------------------------
// AudioPlayBuffer — plays a RAM buffer of 16-bit PCM samples via the
// Teensy audio interrupt. Drop-in replacement for AudioPlayWav for our use.
// ---------------------------------------------------------------------------
class AudioPlayBuffer : public AudioStream {
public:
    AudioPlayBuffer() : AudioStream(0, NULL), _buf(nullptr),
                        _pos(0), _len(0), _active(false) {}

    void start(const int16_t* buf, uint32_t nSamples) {
        _buf    = buf;
        _len    = nSamples;
        _pos    = 0;
        _active = true;
    }

    bool isPlaying() const { return _active; }

    virtual void update(void) override {
        if (!_active) return;
        audio_block_t* block = allocate();
        if (!block) return;

        uint32_t n = _len - _pos;
        if (n > AUDIO_BLOCK_SAMPLES) n = AUDIO_BLOCK_SAMPLES;
        if (n > 0)
            memcpy(block->data, _buf + _pos, n * sizeof(int16_t));
        if (n < AUDIO_BLOCK_SAMPLES) {
            memset(block->data + n, 0, (AUDIO_BLOCK_SAMPLES - n) * sizeof(int16_t));
            _active = false;
        }
        _pos += n;

        transmit(block);
        release(block);
    }

private:
    const int16_t*   _buf;
    volatile uint32_t _pos;
    volatile uint32_t _len;
    volatile bool     _active;
};

// ---------------------------------------------------------------------------
// Shared PCM buffer — filled word-by-word, played as one continuous stream
// ---------------------------------------------------------------------------
static int16_t   g_pcmBuf[PCM_MAX_SAMPLES];
static uint32_t  g_pcmLen = 0;

static void pcmReset() { g_pcmLen = 0; }

// Leading/trailing trim: only removes true digital silence (< 0.15% of max).
// Low enough to preserve plosive bursts; removes the dead samples at
// the very start/end of MBROLA WAVs before the fade-in/out begins.
#define PCM_TRIM 50

// Normalize each phoneme up to this peak before mixing (out of 32767).
// Only boosts — phonemes already louder than this are left unchanged.
// Brings weak plosives (k, p, t) up to be audible alongside vowels.
#define PCM_TARGET_PEAK 8000

// Crossfade window in samples (8 ms at 16 kHz).
#define XFADE_SAMPLES 128

// Find the data chunk in a WAV file and return its byte offset.
// Returns 0 if not found or on error.
static uint32_t wavDataOffset(File& f) {
    char tag[4];
    f.seek(12); // skip RIFF header
    while (f.available() >= 8) {
        f.read(tag, 4);
        uint32_t sz = 0;
        f.read((uint8_t*)&sz, 4);
        if (memcmp(tag, "data", 4) == 0) return f.position();
        f.seek(f.position() + sz);
    }
    return 0;
}

// Append raw PCM from a WAV file into g_pcmBuf.
// 1. Trims leading near-silence (MBROLA fade-in envelope).
// 2. Crossfades the join with the previous phoneme to eliminate
//    the amplitude step that causes audible clipping between phonemes.
static bool pcmAppendWav(const char* path, bool isLast = false, bool isStop = false) {
    File f = SD.open(path);
    if (!f) return false;

    uint32_t dataPos = wavDataOffset(f);
    if (dataPos == 0) { f.close(); return false; }

    uint32_t samples = (f.size() - dataPos) / sizeof(int16_t);
    if (samples == 0) { f.close(); return false; }

    uint32_t avail = PCM_MAX_SAMPLES - g_pcmLen;
    if (samples > avail) samples = avail;

    int16_t* dst = g_pcmBuf + g_pcmLen;
    f.seek(dataPos);
    f.read((uint8_t*)dst, samples * sizeof(int16_t));
    f.close();

    // Trim true digital silence from both ends.
    // For stops, skip leading trim entirely — the closure period (near-silence
    // before the burst) is an important acoustic cue and must be preserved.
    uint32_t start = 0;
    if (g_pcmLen > 0 && !isStop)  // skip leading trim on first phoneme or any stop
        while (start < samples && abs(dst[start]) < PCM_TRIM) start++;

    // Trim trailing digital silence
    uint32_t end = samples;
    while (end > start && abs(dst[end - 1]) < PCM_TRIM) end--;

    uint32_t trimmed = end - start;
    if (trimmed == 0) return true;
    if (start > 0)
        memmove(dst, dst + start, trimmed * sizeof(int16_t));

    // Normalize: boost quiet phonemes up to PCM_TARGET_PEAK (never reduce).
    int16_t peak = 0;
    for (uint32_t i = 0; i < trimmed; i++) {
        int16_t a = abs(dst[i]);
        if (a > peak) peak = a;
    }
    if (peak > 0 && peak < PCM_TARGET_PEAK) {
        int32_t gain256 = ((int32_t)PCM_TARGET_PEAK * 256) / peak;
        for (uint32_t i = 0; i < trimmed; i++) {
            int32_t s = ((int32_t)dst[i] * gain256) >> 8;
            if (s >  32767) s =  32767;
            if (s < -32768) s = -32768;
            dst[i] = (int16_t)s;
        }
    }

    // Crossfade interior phonemes. Skip for the last phoneme or a stop consonant:
    // plosive bursts live at the onset and crossfading fades them out.
    // The closure silence at the start of a plosive prevents any hard click.
    if (!isLast && !isStop && g_pcmLen >= XFADE_SAMPLES && trimmed >= XFADE_SAMPLES) {
        int16_t* prev = g_pcmBuf + g_pcmLen - XFADE_SAMPLES;
        for (uint32_t i = 0; i < XFADE_SAMPLES; i++) {
            int32_t a = (int32_t)(i * 256) / XFADE_SAMPLES; // 0..255
            prev[i] = (int16_t)(((256 - a) * (int32_t)prev[i]
                               +        a  * (int32_t)dst[i]) >> 8);
        }
        memmove(g_pcmBuf + g_pcmLen, dst + XFADE_SAMPLES,
                (trimmed - XFADE_SAMPLES) * sizeof(int16_t));
        g_pcmLen += trimmed - XFADE_SAMPLES;
    } else {
        g_pcmLen += trimmed;
    }
    return true;
}

#endif // TTS_BUFFER_H
