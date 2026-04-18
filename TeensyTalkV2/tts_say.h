// tts_say.h
// say() and sayNumber() for TeensyTalk V2.
// Uses dictionary lookup first, then letter-to-sound rules.
// Outputs MBROLA phonemes -> WAV playback via tts_phonemes.h
// Requires tts_dict.h, tts_rules.h, tts_phonemes.h to be included first.

#ifndef TTS_SAY_H
#define TTS_SAY_H

#define MAX_WORD_LEN  64

// Normalize a single word in-place:
// - lowercase everything
// - strip leading/trailing punctuation
// - handle apostrophes (contractions: strip 's, 't, 'll etc.)
static void normalizeWord(char* word) {
    // Lowercase
  for (int i = 0; word[i]; i++) {
    if (word[i] >= 'A' && word[i] <= 'Z') word[i] += 32;
  }
  // Strip leading punctuation (keep letters and digits)
  int start = 0;
  while (word[start] && !isalnum(word[start])) start++;
  if (start > 0) memmove(word, word+start, strlen(word)-start+1);
  // Strip trailing punctuation (keep letters and digits)
  int len = strlen(word);
  while (len > 0 && !isalnum(word[len-1])) { word[--len] = 0; }
  // Handle common contractions - strip suffix
  // 's  -> strip (possessive / is)
  // 't  -> already in dict (don't, won't, can't)
  // 'll -> strip (he'll -> he)
  // 've -> strip
  // 're -> strip
  // 'd  -> strip
  char* apos = strchr(word, '\'');
  if (apos) *apos = 0; // just truncate at apostrophe for now
}

// Speak a single already-normalised lowercase word.
// Loads all phoneme PCM into RAM first, then plays as one gapless stream.
static void speakWord(const char* word, AudioPlayBuffer& player) {
    if (!word || word[0] == 0) return;

    // 1. Dictionary lookup: flash dict first (hand-curated corrections),
    //    then SD dict (CMU data for broad coverage).
    const char* dictPhones = dictLookup(word);
    if (!dictPhones) dictPhones = sdDictLookup(word);

    char phonBuf[MAX_PHONEMES][MAX_PHONEME_LEN];
    int nPhon = 0;

    if (dictPhones) {
        char tmp[128];
        strncpy(tmp, dictPhones, sizeof(tmp)-1);
        tmp[sizeof(tmp)-1] = 0;
        char* tok = strtok(tmp, " ");
        while (tok && nPhon < MAX_PHONEMES) {
            strncpy(phonBuf[nPhon], tok, MAX_PHONEME_LEN-1);
            phonBuf[nPhon][MAX_PHONEME_LEN-1] = 0;
            nPhon++;
            tok = strtok(NULL, " ");
        }
    } else {
        nPhon = applyRules(word, phonBuf, MAX_PHONEMES);
    }

    // 2. Load all phonemes into RAM buffer
    pcmReset();
    for (int i = 0; i < nPhon; i++)
        loadPhoneme(phonBuf[i], i == nPhon - 1);

    // 3. Play the whole word as one continuous stream
    player.start(g_pcmBuf, g_pcmLen);
    while (player.isPlaying()) { delayMicroseconds(200); }

    // Debug output
    Serial.print(word);
    Serial.print(": ");
    for (int i = 0; i < nPhon; i++) {
        Serial.print(phonBuf[i]);
        Serial.print(" ");
    }
    Serial.println(dictPhones ? "(dict)" : "(rules)");
}

static bool isAllDigits(const char* w) {
    if (!w || !w[0]) return false;
    for (int i = 0; w[i]; i++) if (!isdigit((unsigned char)w[i])) return false;
    return true;
}

// Main say() function.
// Accepts a string of words, splits on spaces/punctuation,
// normalises each word and speaks it.
// Commas add a short pause, periods add a longer pause.
static void say(const char* text, AudioPlayBuffer& player) {
    if (!text) return;

  char buf[256];
  strncpy(buf, text, sizeof(buf)-1);
  buf[sizeof(buf)-1] = 0;

  // Walk through character by character
  // Split into words on whitespace, handle pause punctuation
  char word[MAX_WORD_LEN];
  int wi = 0;

  for (int i = 0; buf[i] != 0; i++) {
    char c = buf[i];

    if (c == ',' ) {
      // Flush current word if any
      if (wi > 0) {
        word[wi] = 0;
        normalizeWord(word);
        if (word[0]) speakWordOrNumber(word, player);
        wi = 0;
      }
      delay(80); // short pause for comma
    } else if (c == '.' || c == '!' || c == '?') {
      if (wi > 0) {
        word[wi] = 0;
        normalizeWord(word);
        if (word[0]) speakWordOrNumber(word, player);
        wi = 0;
      }
      delay(200); // sentence pause
    } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      if (wi > 0) {
        word[wi] = 0;
        normalizeWord(word);
        if (word[0]) speakWordOrNumber(word, player);
        wi = 0;
      }
      delay(50); // inter-word gap
    } else {
      if (wi < MAX_WORD_LEN - 1) word[wi++] = c;
    }
  }
  // Flush final word
  if (wi > 0) {
    word[wi] = 0;
    normalizeWord(word);
    if (word[0]) speakWord(word, player);
  }
}

// sayNumber() - speaks any integer from -999,999 to 999,999
// Fixed typo from original ("thousand" not "thousdand")
// Uses say() internally so benefits from dict+rules automatically
static void sayNumber(long n, AudioPlayBuffer& player) {
    if (n < 0) {
    speakWord("negative", player);
    delay(50);
    sayNumber(-n, player);
    return;
    }
  if (n == 0) {
    speakWord("zero", player);
    return;
  }
  if (n >= 1000000) {
    sayNumber(n / 1000000, player);
    delay(30);
    speakWord("million", player);
    delay(50);
    n %= 1000000;
    if (n > 0 && n < 100) speakWord("and", player);
  }
  if (n >= 1000) {
    sayNumber(n / 1000, player);
    delay(30);
    speakWord("thousand", player);
    delay(50);
    n %= 1000;
    if (n > 0 && n < 100) speakWord("and", player);
  }
  if (n >= 100) {
    sayNumber(n / 100, player);
    delay(30);
    speakWord("hundred", player);
    delay(50);
    n %= 100;
    if (n > 0) speakWord("and", player);
  }
  if (n >= 20) {
    const char* tens[] = { "", "", "twenty", "thirty", "forty",
                           "fifty", "sixty", "seventy", "eighty", "ninety" };
    speakWord(tens[n / 10], player);
    delay(30);
    n %= 10;
  }
  if (n > 0) {
    const char* ones[] = { "", "one", "two", "three", "four", "five",
                           "six", "seven", "eight", "nine", "ten",
                           "eleven", "twelve", "thirteen", "fourteen",
                           "fifteen", "sixteen", "seventeen", "eighteen",
                           "nineteen" };
    speakWord(ones[n], player);
  }
}

static void speakWordOrNumber(const char* word, AudioPlayBuffer& player) {
    if (!word || !word[0]) return;
    if (isAllDigits(word)) sayNumber(atol(word), player);
    else speakWord(word, player);
}

#endif // TTS_SAY_H
