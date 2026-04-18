#!/usr/bin/env python3
"""
build_dict.py — Build DICT.BIN for TeensyTalk from the CMU Pronouncing Dictionary.

Downloads cmudict, converts ARPABET → MBROLA us2 phonemes, writes sorted
fixed-width binary records to DICT.BIN.  Copy DICT.BIN to the SD card root.

Usage:
    python3 build_dict.py               # writes DICT.BIN
    python3 build_dict.py out.bin       # custom output path

Record format (128 bytes each, sorted alphabetically by word):
    bytes  0-31  : word, null-padded  (max 31 chars)
    bytes 32-127 : phoneme string, null-padded  (max 95 chars, MBROLA us2 names)
"""

import urllib.request
import sys
import os

CMU_URL = ("https://raw.githubusercontent.com/"
           "cmusphinx/cmudict/master/cmudict.dict")

WORD_LEN  = 32
PHONE_LEN = 96
REC_SIZE  = WORD_LEN + PHONE_LEN   # 128

# ARPABET phoneme → MBROLA us2 phoneme name.
# Vowels with stress digit 0 (unstressed) map to schwa; 1/2 map to the
# stressed variant.  All other stress digits are stripped before lookup.
ARPABET = {
    # Vowels
    "AA":  "A",    # f[a]ther
    "AE":  "{",    # c[a]t
    "AH0": "@",    # [a]bout  (unstressed schwa)
    "AH1": "V",    # b[u]t    (stressed strut)
    "AH2": "V",    # secondary stress — treat as strut
    "AO":  "O",    # th[ou]ght
    "AW":  "aU",   # h[ow]
    "AY":  "AI",   # b[i]te
    "EH":  "E",    # b[e]t
    "ER":  "r=",   # b[ir]d
    "EY":  "EI",   # b[ai]t
    "IH":  "I",    # b[i]t
    "IY":  "i",    # b[ea]t
    "OW":  "@U",   # b[oa]t
    "OY":  "OI",   # b[oy]
    "UH":  "U",    # b[oo]k
    "UW":  "u",    # f[oo]d
    # Consonants
    "B":   "b",
    "CH":  "tS",
    "D":   "d",
    "DH":  "D",    # [th]e   (voiced)
    "F":   "f",
    "G":   "g",
    "HH":  "h",
    "JH":  "dZ",
    "K":   "k",
    "L":   "l",
    "M":   "m",
    "N":   "n",
    "NG":  "N",    # si[ng]
    "P":   "p",
    "R":   "r",
    "S":   "s",
    "SH":  "S",    # [sh]e
    "T":   "t",
    "TH":  "T",    # [th]ink (voiceless)
    "V":   "v",
    "W":   "w",
    "Y":   "j",
    "Z":   "z",
    "ZH":  "Z",    # vi[si]on
}


def convert_phones(arpabet_tokens):
    """Convert a list of ARPABET tokens to a MBROLA phoneme string.
    Returns None if any token is unrecognised."""
    out = []
    for tok in arpabet_tokens:
        # Try with stress digit first (AH0 / AH1 / AH2), then stripped
        if tok in ARPABET:
            out.append(ARPABET[tok])
        else:
            base = tok.rstrip("012")
            if base in ARPABET:
                out.append(ARPABET[base])
            else:
                return None   # unknown phoneme — skip word
    return " ".join(out)


def main():
    out_path = sys.argv[1] if len(sys.argv) > 1 else "DICT.BIN"

    print("Downloading CMU Pronouncing Dictionary …")
    with urllib.request.urlopen(CMU_URL) as resp:
        raw = resp.read().decode("latin-1")

    entries = {}
    skipped_long = 0
    skipped_alpha = 0
    skipped_phoneme = 0

    for line in raw.splitlines():
        if line.startswith(";;;") or not line.strip():
            continue

        parts = line.split()
        if len(parts) < 2:
            continue

        raw_word = parts[0]
        # Skip alternate pronunciations  e.g. "THE(2)"
        if "(" in raw_word:
            continue

        word = raw_word.lower()

        # Only pure alphabetic words
        if not word.isalpha():
            skipped_alpha += 1
            continue

        # Skip words too long for the record
        if len(word) >= WORD_LEN:
            skipped_long += 1
            continue

        phones = convert_phones(parts[1:])
        if phones is None:
            skipped_phoneme += 1
            continue

        if len(phones) >= PHONE_LEN:
            skipped_long += 1
            continue

        # Only keep first pronunciation (don't overwrite)
        if word not in entries:
            entries[word] = phones

    words = sorted(entries)

    print(f"  Entries:          {len(words):,}")
    print(f"  Skipped (long):   {skipped_long:,}")
    print(f"  Skipped (alpha):  {skipped_alpha:,}")
    print(f"  Skipped (phones): {skipped_phoneme:,}")
    print(f"Writing {out_path} …")

    with open(out_path, "wb") as f:
        for word in words:
            phones = entries[word]
            word_b  = word.encode("ascii").ljust(WORD_LEN,  b"\x00")[:WORD_LEN]
            phone_b = phones.encode("ascii").ljust(PHONE_LEN, b"\x00")[:PHONE_LEN]
            f.write(word_b + phone_b)

    size_kb = os.path.getsize(out_path) / 1024
    print(f"Done — {size_kb:.0f} KB  ({len(words):,} words × {REC_SIZE} bytes)")
    print(f"Copy {out_path} to the root of your SD card.")


if __name__ == "__main__":
    main()
