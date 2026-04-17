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
{ "k_h", "KH"   },  // aspirated K
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

// Play a single phoneme WAV file.
// Blocks until playback is complete.
// Expects playWav1 and SD to already be initialised.
static void playPhoneme(const char* mbrola, AudioPlayWav& player) {
    const char* base = phonemeToWav(mbrola);
  char path[32];
  snprintf(path, sizeof(path), "%s.wav", base);
  player.play(path);
  while (player.isPlaying()) { delay(1); }
}

#endif // TTS_PHONEMES_H
