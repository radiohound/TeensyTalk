// tts_phonemes.h
// Maps MBROLA us2 phoneme names to WAV filenames on SD card.
// MBROLA phoneme names that are invalid FAT32 characters get
// a safe substitute filename. Everything else plays as-is.
// Part of TeensyTalk espeak-ng-rewrite branch.

#ifndef TTS_PHONEMES_H
#define TTS_PHONEMES_H

// Phoneme name to WAV filename mapping.
// Only entries where the MBROLA name differs from the WAV filename
// are listed here. All other phonemes use their MBROLA name directly
// as the WAV filename (e.g. "b" -> "b.wav", "EI" -> "EI.wav").

struct PhonemeMap {
  const char* mbrola;   // MBROLA us2 phoneme name
  const char* wavfile;  // WAV filename on SD card (without .wav)
};

// Entries needed because MBROLA name contains FAT32-illegal chars
// or would be ambiguous as a filename.
static const PhonemeMap PHONEME_MAP[] = {
{ "{",   "AE"   },  // apple       - { is not FAT32 safe
{ "@",   "SCH"  },  // about/schwa - @ is not FAT32 safe
{ "@U",  "OW"   },  // over        - @ is not FAT32 safe
{ "r=",  "ER"   },  // her         - = is not FAT32 safe
{ "p_h", "PH"   },  // aspirated P - _ ok but keep consistent
{ "t_h", "TH2"  },  // aspirated T - avoid clash with TH (voiceless th)
{ "k",   "KH"   },  // plain k too weak; aspirated has audible burst
{ "k_h", "KH"   },  // aspirated K (explicit)
{ "4",   "FL"   },  // flapped T (butter, later)
{ "l=",  "LS"   },  // syllabic L  (bottle)
{ "T",   "TH"   },  // voiceless th (think, path) - T clashes with t.wav on FAT32
{ "D",   "DH"   },  // voiced th (the, this) - D clashes with d.wav on FAT32
{ "S",   "SH"   },  // sh sound (she, fish) - S clashes with s.wav on FAT32
{ "Z",   "ZH"   },  // zh sound (vision) - Z clashes with z.wav on FAT32
{ "V",   "UH"   },  // strut vowel (but, cut) - V clashes with v.wav on FAT32
{ "N",   "NG"   },  // ng sound (sing, ring) - N clashes with n.wav on FAT32
{ "I",   "IH"   },  // short I (bit, big) - I clashes with i.wav on FAT32
{ "U",   "UU"   },  // short oo (book, could) - U clashes with u.wav on FAT32
{ NULL,  NULL   }   // sentinel
};

// Look up the WAV filename for a given MBROLA phoneme name.
// Returns the mapped name if found, otherwise returns the
// MBROLA name itself (which is already a valid FAT32 filename).
static const char* phonemeToWav(const char* mbrola) {
    for (int i = 0; PHONEME_MAP[i].mbrola != NULL; i++) {
    if (strcmp(mbrola, PHONEME_MAP[i].mbrola) == 0) {
      return PHONEME_MAP[i].wavfile;
    }
    }
  return mbrola; // use MBROLA name directly as filename
}

// Load one phoneme's PCM data into the shared g_pcmBuf.
// Call pcmReset() before the first phoneme of a word, then
// start(g_pcmBuf, g_pcmLen) on AudioPlayBuffer when all are loaded.
// Minimum duration for a word-final phoneme in samples (55 ms at 16 kHz).
#define FINAL_MIN_SAMPLES 880

static bool isStopPhoneme(const char* mbrola) {
    const char* stops[] = { "b","d","g","p","t","k","k_h","p_h","t_h","tS","dZ","4", NULL };
    for (int i = 0; stops[i]; i++)
        if (strcmp(mbrola, stops[i]) == 0) return true;
    return false;
}

static void loadPhoneme(const char* mbrola, bool isLast = false) {
    const char* base = phonemeToWav(mbrola);
    char path[32];
    snprintf(path, sizeof(path), "%s.wav", base);
    bool isStop = isStopPhoneme(mbrola);
    uint32_t before = g_pcmLen;
    if (!pcmAppendWav(path, isLast, isStop)) {
        Serial.print("MISSING: "); Serial.println(path);
        return;
    }

    // Extend short final phonemes by repeating their aspiration tail,
    // then apply a linear fade-out over the extension to eliminate loop clicks.
    if (isLast) {
        uint32_t added = g_pcmLen - before;
        if (added > 0 && added < FINAL_MIN_SAMPLES) {
            uint32_t tailLen = added / 3;
            if (tailLen < 8) tailLen = 8;
            uint32_t extStart = g_pcmLen;
            while (g_pcmLen - before < FINAL_MIN_SAMPLES
                   && g_pcmLen + tailLen <= PCM_MAX_SAMPLES) {
                memcpy(g_pcmBuf + g_pcmLen,
                       g_pcmBuf + g_pcmLen - tailLen,
                       tailLen * sizeof(int16_t));
                g_pcmLen += tailLen;
            }
            // Fade the extension to zero so loop boundaries are inaudible
            uint32_t extLen = g_pcmLen - extStart;
            for (uint32_t i = 0; i < extLen; i++) {
                int32_t scale = (int32_t)((extLen - i) * 256) / extLen;
                g_pcmBuf[extStart + i] =
                    (int16_t)(((int32_t)g_pcmBuf[extStart + i] * scale) >> 8);
            }
        }
    }
}

#endif // TTS_PHONEMES_H
