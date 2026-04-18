# TeensyTalk V2

Text-to-speech for the Teensy 4.1 + Audio Shield. Converts arbitrary English text to speech by concatenating MBROLA phoneme WAV files, using a two-tier pronunciation lookup system and eSpeak-ng-derived letter-to-sound rules.

---

## How It Works

### 1. Text → Words
`say("some text")` splits the input on whitespace and punctuation. Commas insert a short pause (80 ms), periods/question marks/exclamation marks insert a longer pause (200 ms), and spaces insert a 50 ms inter-word gap.

Each word is normalized: lowercased, leading/trailing punctuation stripped, contractions truncated at the apostrophe.

### 2. Word → Phonemes (two-tier lookup)

**Tier 1 — Flash dictionary** (`tts_dict.h`)
A small hand-curated table compiled into firmware. Checked first. Use this to override or correct any word that the SD dictionary or rules get wrong.

**Tier 2 — SD card dictionary** (`DICT.BIN` on SD card)
~125,000 words from the CMU Pronouncing Dictionary, converted from ARPABET to MBROLA us2 phoneme names. Binary-searched in ~17 seeks (~34 ms worst case). Falls back to rules if the word is not found.

**Tier 3 — Letter-to-sound rules** (`tts_rules.h`)
eSpeak-ng-derived context-sensitive rules applied letter by letter when neither dictionary has the word.

### 3. Phonemes → Audio

All phoneme WAV files are 16 kHz, 16-bit mono, stored in the SD card root directory. For each word:
1. Each phoneme's WAV is read from SD and appended into a RAM buffer.
2. Adjacent phonemes are joined with a 128-sample bidirectional crossfade to eliminate click artifacts at boundaries.
3. Short word-final phonemes (< 55 ms) are extended by looping their aspiration tail with a linear fade-out.
4. The entire word is played as one continuous PCM stream through the Teensy Audio library — no gaps between phonemes.

The I²S sample rate is set to 16000 Hz in `setup()` to match the phoneme WAV files.

---

## SD Card Contents

Copy all of the following to the **root directory** of the SD card (no subdirectory):

### Phoneme WAV files (~45 files)
All files from `TeensyTalkV2/wavs/` — listed below with their MBROLA phoneme mapping:

| WAV filename | MBROLA phoneme | Example |
|---|---|---|
| `A.wav` | `A` | f**a**ther |
| `AE.wav` | `{` | c**a**t |
| `AI.wav` | `AI` | b**i**te |
| `SCH.wav` | `@` | **a**bout (schwa) |
| `OW.wav` | `@U` | b**oa**t |
| `E.wav` | `E` | b**e**t |
| `EI.wav` | `EI` | b**ai**t |
| `ER.wav` | `r=` | b**ir**d |
| `IH.wav` | `I` | b**i**t |
| `i.wav` | `i` | b**ea**t |
| `O.wav` | `O` | th**ou**ght |
| `OI.wav` | `OI` | b**oy** |
| `OR.wav` | `OR` | f**or** |
| `UH.wav` | `V` | b**u**t |
| `UU.wav` | `U` | b**oo**k |
| `aU.wav` | `aU` | h**ow** |
| `b.wav` | `b` | **b**at |
| `d.wav` | `d` | **d**og |
| `dZ.wav` | `dZ` | **j**ump |
| `DH.wav` | `D` | **th**e (voiced) |
| `f.wav` | `f` | **f**at |
| `FL.wav` | `4` | bu**tt**er (flap T) |
| `g.wav` | `g` | **g**o |
| `h.wav` | `h` | **h**at |
| `j.wav` | `j` | **y**es |
| `k.wav` | `k` | (mapped to KH — see below) |
| `KH.wav` | `k`, `k_h` | **k**ite (aspirated, used for all K) |
| `l.wav` | `l` | **l**et |
| `LS.wav` | `l=` | bott**le** (syllabic L) |
| `m.wav` | `m` | **m**at |
| `n.wav` | `n` | **n**et |
| `NG.wav` | `N` | si**ng** |
| `p.wav` | `p` | **p**at |
| `PH.wav` | `p_h` | aspirated P |
| `r.wav` | `r` | **r**at |
| `s.wav` | `s` | **s**at |
| `SH.wav` | `S` | **sh**e |
| `t.wav` | `t` | **t**ap |
| `TH.wav` | `T` | **th**ink (voiceless) |
| `TH2.wav` | `t_h` | aspirated T |
| `tS.wav` | `tS` | **ch**ip |
| `u.wav` | `u` | f**oo**d |
| `v.wav` | `v` | **v**at |
| `w.wav` | `w` | **w**et |
| `z.wav` | `z` | **z**oo |
| `ZH.wav` | `Z` | vi**si**on |

### Dictionary file
| File | Description |
|---|---|
| `DICT.BIN` | CMU Pronouncing Dictionary, ~125,000 words, binary format |

**Important:** All files go directly in the SD card root — no subdirectories.

---

## Building DICT.BIN

`build_dict.py` (in the repo root) downloads the CMU Pronouncing Dictionary, converts ARPABET phonemes to MBROLA us2 names, and writes the sorted binary file.

```bash
python3 build_dict.py          # writes DICT.BIN in current directory
python3 build_dict.py out.bin  # custom output path
```

Copy the resulting `DICT.BIN` to the SD card root. The file is ~15 MB (~125,000 entries × 128 bytes each).

On boot, the firmware prints: `SD dict loaded: 125247 entries`

---

## How the Phoneme WAV Files Were Made

The WAV files are individual phoneme recordings extracted from the **MBROLA us2 voice** — a diphone synthesis voice for American English developed as part of the MBROLA project. Each file contains a single phoneme sound at 16 kHz, 16-bit mono.

The MBROLA project: https://github.com/numediart/MBROLA  
The us2 voice database is available from the MBROLA voices repository.

To regenerate or add phonemes, install MBROLA and the us2 voice database, then synthesize individual phoneme sequences using a `.pho` input file (phoneme name + duration in ms + pitch points).

---

## Fixing a Mispronounced Word

### Step 1 — Identify the phonemes

Run the sketch and type the word into the Serial Monitor. The debug output shows what phonemes were used and whether they came from the dictionary or rules:

```
rocket: r A k I t (dict)
travel: t r { v @ l (rules)
```

Determine the correct MBROLA us2 phoneme sequence for the word. Reference the WAV filename table above for phoneme names — note that some phonemes use special characters (`{`, `@`, `r=`, etc.) that map to renamed WAV files.

### Step 2 — Add to the flash dictionary

Open [TeensyTalkV2/tts_dict.h](TeensyTalkV2/tts_dict.h) and add an entry in alphabetical order:

```cpp
{ "travel",  "t r { v @ l"    },
{ "rocket",  "r A k I t"      },   // ← new entry, in alphabetical position
```

**The list must stay in strict alphabetical order** — it is searched with `strcmp` comparisons. Adding an entry out of order will cause incorrect lookups for nearby words.

### Step 3 — Rebuild and flash

Recompile and upload. The flash dictionary takes priority over the SD card dictionary, so your correction will always win.

---

## MBROLA Phoneme Reference (us2)

| MBROLA | WAV file | Sound |
|---|---|---|
| `A` | A.wav | "father" vowel |
| `{` | AE.wav | "cat" vowel |
| `@` | SCH.wav | schwa — "about", "-er" unstressed |
| `@U` | OW.wav | "boat" diphthong |
| `AI` | AI.wav | "bite" diphthong |
| `aU` | aU.wav | "how" diphthong |
| `E` | E.wav | "bet" vowel |
| `EI` | EI.wav | "bait" diphthong |
| `I` | IH.wav | "bit" short vowel |
| `i` | i.wav | "beat" long vowel |
| `O` | O.wav | "thought" vowel |
| `OI` | OI.wav | "boy" diphthong |
| `r=` | ER.wav | "bird" r-colored vowel |
| `U` | UU.wav | "book" vowel |
| `u` | u.wav | "food" vowel |
| `V` | UH.wav | "but" strut vowel |
| `b` | b.wav | voiced bilabial stop |
| `d` | d.wav | voiced alveolar stop |
| `D` | DH.wav | voiced "th" (the, this) |
| `dZ` | dZ.wav | "jump" affricate |
| `f` | f.wav | voiceless labiodental |
| `4` | FL.wav | flapped T (butter, later) |
| `g` | g.wav | voiced velar stop |
| `h` | h.wav | glottal fricative |
| `j` | j.wav | palatal approximant (yes) |
| `k` | KH.wav | voiceless velar stop (aspirated) |
| `l` | l.wav | lateral approximant |
| `l=` | LS.wav | syllabic L (bottle) |
| `m` | m.wav | bilabial nasal |
| `n` | n.wav | alveolar nasal |
| `N` | NG.wav | velar nasal (sing) |
| `p` | p.wav | voiceless bilabial stop |
| `r` | r.wav | approximant |
| `s` | s.wav | voiceless alveolar fricative |
| `S` | SH.wav | "she" fricative |
| `t` | t.wav | voiceless alveolar stop |
| `T` | TH.wav | voiceless "th" (think) |
| `tS` | tS.wav | "chip" affricate |
| `u` | u.wav | high back rounded vowel |
| `v` | v.wav | voiced labiodental |
| `w` | w.wav | labio-velar approximant |
| `z` | z.wav | voiced alveolar fricative |
| `Z` | ZH.wav | "vision" fricative |

---

## Hardware

- Teensy 4.1
- Teensy Audio Shield (SGTL5000)
- Micro SD card formatted FAT32

Audio output is on the headphone jack of the Audio Shield.

---

## Source Files

| File | Purpose |
|---|---|
| `TeensyTalkV2.ino` | Setup, audio graph, serial input loop |
| `tts_buffer.h` | PCM RAM buffer, WAV loading, crossfade, normalization |
| `tts_phonemes.h` | MBROLA→WAV filename mapping, `loadPhoneme()` |
| `tts_dict.h` | Flash dictionary (hand-curated, highest priority) |
| `tts_dict_sd.h` | SD card binary dictionary, `sdDictInit()`, `sdDictLookup()` |
| `tts_rules.h` | eSpeak-ng-derived letter-to-sound rules |
| `tts_say.h` | `say()`, `sayNumber()`, word splitting and normalization |
| `build_dict.py` | Builds `DICT.BIN` from CMU Pronouncing Dictionary |
