#!/usr/bin/env python3
"""
test_audio.py — Desktop audio preview for TeensyTalk V2.

Runs the exact same PCM concatenation algorithm as tts_buffer.h:
  - per-phoneme trim (threshold 50)
  - amplitude normalization to peak 8000
  - 128-sample bidirectional crossfade on interior phonemes
  - 880-sample (55ms) minimum duration for the final phoneme
  - faded tail extension for short final phonemes

Usage:
    python3 test_audio.py "l I b @ r t i"        # phoneme string
    python3 test_audio.py liberty                 # word (rules only, no SD dict)
    python3 test_audio.py "l I b @ r t i" --play  # play immediately via afplay
    python3 test_audio.py "l I b @ r t i" --out result.wav
"""

import sys
import os
import struct
import subprocess
import argparse
import numpy as np

WAV_DIR         = os.path.join(os.path.dirname(__file__), "TeensyTalkV2", "wavs")
PCM_TRIM        = 50
PCM_TARGET_PEAK = 8000
XFADE_SAMPLES   = 128
FINAL_MIN_SAMPLES = 880   # 55 ms at 16 kHz
SAMPLE_RATE     = 16000

PHONEME_MAP = {
    "{":   "AE",
    "@":   "SCH",
    "@U":  "OW",
    "r=":  "ER",
    "p_h": "PH",
    "t_h": "TH2",
    "k":   "KH",
    "k_h": "KH",
    "4":   "FL",
    "l=":  "LS",
    "T":   "TH",
    "D":   "DH",
    "S":   "SH",
    "Z":   "ZH",
    "V":   "UH",
    "N":   "NG",
    "I":   "IH",
    "U":   "UU",
    "AI":  "AI",
    "EI":  "EI",
    "aU":  "aU",
    "OI":  "OI",
}

def phoneme_to_wav(mbrola):
    name = PHONEME_MAP.get(mbrola, mbrola)
    return os.path.join(WAV_DIR, name + ".wav")

def read_wav_pcm(path):
    """Read 16-bit mono PCM samples from a WAV file."""
    with open(path, "rb") as f:
        data = f.read()
    # Find 'data' chunk
    pos = 12
    while pos + 8 <= len(data):
        tag = data[pos:pos+4]
        size = struct.unpack_from("<I", data, pos+4)[0]
        pos += 8
        if tag == b"data":
            samples = np.frombuffer(data[pos:pos+size], dtype=np.int16).copy()
            return samples
        pos += size
    raise ValueError(f"No data chunk in {path}")

def trim_silence(samples, threshold, skip_leading=False):
    """Trim leading and trailing near-silence."""
    start = 0
    if not skip_leading:
        while start < len(samples) and abs(int(samples[start])) < threshold:
            start += 1
    end = len(samples)
    while end > start and abs(int(samples[end-1])) < threshold:
        end -= 1
    return samples[start:end]

def normalize(samples, target_peak):
    """Boost quiet phonemes up to target_peak; never reduce."""
    peak = np.max(np.abs(samples.astype(np.int32)))
    if peak > 0 and peak < target_peak:
        gain = target_peak / peak
        samples = np.clip(samples.astype(np.float32) * gain, -32768, 32767).astype(np.int16)
    return samples

def crossfade(buf, new_samples, xfade):
    """Overlap-add new_samples onto the end of buf with xfade-sample crossfade."""
    if len(buf) < xfade or len(new_samples) < xfade:
        return np.concatenate([buf, new_samples])
    fade_out = np.linspace(1.0, 0.0, xfade, dtype=np.float32)
    fade_in  = np.linspace(0.0, 1.0, xfade, dtype=np.float32)
    overlap  = (buf[-xfade:].astype(np.float32) * fade_out +
                new_samples[:xfade].astype(np.float32) * fade_in)
    result = np.concatenate([buf[:-xfade], overlap.astype(np.int16), new_samples[xfade:]])
    return result

def extend_final(samples, min_samples):
    """Extend short final phonemes by looping their tail with a fade-out."""
    if len(samples) >= min_samples:
        return samples
    tail_len = max(8, len(samples) // 3)
    ext_start = len(samples)
    samples = samples.copy()
    while len(samples) < min_samples:
        tail = samples[-tail_len:]
        samples = np.concatenate([samples, tail])
    samples = samples[:min_samples + tail_len]  # don't overshoot too much
    ext_len = len(samples) - ext_start
    fade = np.linspace(1.0, 0.0, ext_len, dtype=np.float32)
    samples[ext_start:] = np.clip(
        samples[ext_start:].astype(np.float32) * fade, -32768, 32767
    ).astype(np.int16)
    return samples

STOPS = {'b','d','g','p','t','k','k_h','p_h','t_h','tS','dZ','4'}

def build_pcm(phonemes):
    """Concatenate phoneme WAVs into a single PCM array."""
    buf = np.array([], dtype=np.int16)
    for i, ph in enumerate(phonemes):
        is_last = (i == len(phonemes) - 1)
        stop = ph in STOPS
        path = phoneme_to_wav(ph)
        if not os.path.exists(path):
            print(f"  MISSING: {path}")
            continue
        raw = read_wav_pcm(path)
        # Preserve closure period for stops; trim silence elsewhere
        trimmed = trim_silence(raw, PCM_TRIM, skip_leading=(len(buf) == 0 or stop))
        if len(trimmed) == 0:
            continue
        trimmed = normalize(trimmed, PCM_TARGET_PEAK)
        if is_last:
            trimmed = extend_final(trimmed, FINAL_MIN_SAMPLES)
            buf = np.concatenate([buf, trimmed])
        elif stop:
            buf = np.concatenate([buf, trimmed])  # no crossfade into stops
        else:
            buf = crossfade(buf, trimmed, XFADE_SAMPLES)
    return buf

def write_wav(path, samples, sample_rate=SAMPLE_RATE):
    """Write 16-bit mono WAV file."""
    data = samples.astype(np.int16).tobytes()
    with open(path, "wb") as f:
        f.write(b"RIFF")
        f.write(struct.pack("<I", 36 + len(data)))
        f.write(b"WAVEfmt ")
        f.write(struct.pack("<IHHIIHH", 16, 1, 1, sample_rate,
                            sample_rate * 2, 2, 16))
        f.write(b"data")
        f.write(struct.pack("<I", len(data)))
        f.write(data)

def main():
    parser = argparse.ArgumentParser(description="TeensyTalk V2 audio preview")
    parser.add_argument("input", help="Phoneme string (quoted) or single word")
    parser.add_argument("--out", default="preview.wav", help="Output WAV path")
    parser.add_argument("--play", action="store_true", help="Play via afplay after writing")
    args = parser.parse_args()

    # Determine if input looks like a phoneme string (contains spaces or special chars)
    # or a plain word to run through the simple built-in mapping
    text = args.input.strip()
    if " " in text or any(c in text for c in "{}@="):
        phonemes = text.split()
        print(f"Phonemes: {' '.join(phonemes)}")
    else:
        # Try to look up the word in tts_dict.h via a quick grep
        word = text.lower()
        phones = grep_flash_dict(word)
        if phones:
            phonemes = phones.split()
            print(f"{word}: {' '.join(phonemes)} (flash dict)")
        else:
            print(f"{word}: not in flash dict — pass phoneme string directly")
            print(f'  e.g.: python3 test_audio.py "l I b @ r t i"')
            sys.exit(1)

    pcm = build_pcm(phonemes)
    write_wav(args.out, pcm)
    duration_ms = int(len(pcm) / SAMPLE_RATE * 1000)
    print(f"Written {args.out}  ({len(pcm)} samples, {duration_ms} ms)")

    if args.play:
        subprocess.run(["afplay", args.out])

def grep_flash_dict(word):
    """Quick grep of tts_dict.h for a word entry."""
    dict_path = os.path.join(os.path.dirname(__file__), "TeensyTalkV2", "tts_dict.h")
    try:
        with open(dict_path) as f:
            for line in f:
                if f'"{word}"' in line:
                    # Extract phoneme string between second pair of quotes
                    parts = line.split('"')
                    if len(parts) >= 4:
                        return parts[3]
    except FileNotFoundError:
        pass
    return None

if __name__ == "__main__":
    main()
