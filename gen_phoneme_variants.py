#!/usr/bin/env python3
"""
gen_phoneme_variants.py — Regenerate b.wav and KH.wav with improved coarticulation.

The problem:
  - b.wav: isolated synthesis loses bilabial formant transitions → sounds like 'g'
  - KH.wav: too short, sounds like a click

The fix: synthesize each phoneme in a short vowel context (@ b @) so MBROLA
bakes in the correct formant transitions, then trim to just the consonant portion.

Usage:
    cd ~/mbrola_work
    python3 /path/to/gen_phoneme_variants.py

Produces variant WAVs in ~/mbrola_work/variants/ for auditioning.
After choosing, copy the winner to TeensyTalkV2/wavs/ and the SD card.
"""

import os
import subprocess
import struct
import tempfile
import numpy as np

US2_DB    = os.path.expanduser("~/mbrola_work/us2")
OUT_DIR   = os.path.expanduser("~/mbrola_work/variants")
WAV_DIR   = os.path.join(os.path.dirname(os.path.abspath(__file__)), "TeensyTalkV2", "wavs")
MBROLA    = "mbrola"

os.makedirs(OUT_DIR, exist_ok=True)

def run_mbrola(pho_text, out_path):
    with tempfile.NamedTemporaryFile(mode='w', suffix='.pho', delete=False) as f:
        f.write(pho_text)
        pho_path = f.name
    try:
        r = subprocess.run([MBROLA, US2_DB, pho_path, out_path],
                           capture_output=True, text=True)
        if r.returncode != 0:
            print(f"  MBROLA error: {r.stderr.strip()}")
            return False
        return True
    finally:
        os.unlink(pho_path)

def read_pcm(path):
    with open(path, 'rb') as f:
        data = f.read()
    pos = 12
    while pos + 8 <= len(data):
        tag = data[pos:pos+4]
        sz = struct.unpack_from('<I', data, pos+4)[0]
        pos += 8
        if tag == b'data':
            return np.frombuffer(data[pos:pos+sz], dtype=np.int16).copy()
        pos += sz
    return np.array([], dtype=np.int16)

def write_wav(path, samples, rate=16000):
    data = samples.astype(np.int16).tobytes()
    with open(path, 'wb') as f:
        f.write(b'RIFF')
        f.write(struct.pack('<I', 36 + len(data)))
        f.write(b'WAVEfmt ')
        f.write(struct.pack('<IHHIIHH', 16, 1, 1, rate, rate*2, 2, 16))
        f.write(b'data')
        f.write(struct.pack('<I', len(data)))
        f.write(data)

def trim_silence(s, threshold=50):
    start = 0
    while start < len(s) and abs(int(s[start])) < threshold: start += 1
    end = len(s)
    while end > start and abs(int(s[end-1])) < threshold: end -= 1
    return s[start:end]

def play(path):
    subprocess.run(['afplay', path])

# ---------------------------------------------------------------------------
# B variants
# ---------------------------------------------------------------------------
print("=" * 60)
print("Generating b.wav variants...")
print("=" * 60)

b_variants = [
    # (label, pho_content, trim_description)
    # Current: isolated
    ("b_isolated_80ms",
     "_ 50\nb 80 50 100\n_ 50\n",
     "current — isolated, 80ms"),

    # In vowel context, full synthesis (@ b @)
    ("b_context_80ms",
     "_ 20\n@ 50 50 100\nb 80 50 100\n@ 50 50 100\n_ 20\n",
     "in @-b-@ context, 80ms"),

    # Longer duration in context
    ("b_context_110ms",
     "_ 20\n@ 50 50 100\nb 110 50 100\n@ 50 50 100\n_ 20\n",
     "in @-b-@ context, 110ms"),

    # With a preceding A vowel (better bilabial transitions from open vowel)
    ("b_context_A_80ms",
     "_ 20\nA 50 50 100\nb 80 50 100\nA 50 50 100\n_ 20\n",
     "in A-b-A context, 80ms"),
]

def extract_consonant(pho_text, consonant_dur_ms):
    """
    Generate a context variant and extract only the consonant samples.
    Parses the .pho to find the consonant's sample offset, skipping
    leading silence and any preceding vowel.
    """
    tmp = tempfile.NamedTemporaryFile(suffix='.wav', delete=False)
    tmp.close()
    if not run_mbrola(pho_text, tmp.name):
        os.unlink(tmp.name)
        return None
    pcm = read_pcm(tmp.name)
    os.unlink(tmp.name)

    # Calculate sample offset of consonant from .pho durations
    offset_samples = 0
    for line in pho_text.strip().splitlines():
        parts = line.split()
        if len(parts) < 2:
            continue
        phoneme = parts[0]
        dur_ms  = int(parts[1])
        if phoneme == '_' or phoneme.islower() == False and phoneme not in ('A','E','I','O','@','V','U','u','i'):
            # Stop when we reach the consonant line
            break
        offset_samples += int(dur_ms * 16000 / 1000)

    dur_samples = int(consonant_dur_ms * 16000 / 1000)
    chunk = pcm[offset_samples : offset_samples + dur_samples]
    return chunk if len(chunk) > 0 else trim_silence(pcm)

for name, pho, desc in b_variants:
    out = os.path.join(OUT_DIR, name + ".wav")
    if "context" in name:
        # Extract only the b portion (skip leading silence + preceding vowel)
        dur_ms = int(name.split("_")[-1].replace("ms",""))
        pcm = extract_consonant(pho, dur_ms)
    else:
        tmp = os.path.join(OUT_DIR, name + "_tmp.wav")
        run_mbrola(pho, tmp)
        pcm = trim_silence(read_pcm(tmp))
        os.unlink(tmp)
    if pcm is not None and len(pcm) > 0:
        write_wav(out, pcm)
        ms = int(len(pcm) / 16000 * 1000)
        print(f"  {name}.wav  ({ms}ms) — {desc}")

# Build complete word "liberty" with each b variant
print()
print("Playing as complete word 'liberty' (l I b @ r t i)...")
print()
l_pcm   = trim_silence(read_pcm(os.path.join(WAV_DIR, "l.wav")))
ih_pcm  = trim_silence(read_pcm(os.path.join(WAV_DIR, "IH.wav")))
sch_pcm = trim_silence(read_pcm(os.path.join(WAV_DIR, "SCH.wav")))
r_pcm   = trim_silence(read_pcm(os.path.join(WAV_DIR, "r.wav")))
t_pcm   = trim_silence(read_pcm(os.path.join(WAV_DIR, "t.wav")))
i_pcm   = trim_silence(read_pcm(os.path.join(WAV_DIR, "i.wav")))

for name, pho, desc in b_variants:
    out = os.path.join(OUT_DIR, name + ".wav")
    if os.path.exists(out):
        print(f"  [{desc}]")
        b_pcm = trim_silence(read_pcm(out))
        word = np.concatenate([l_pcm, ih_pcm, b_pcm, sch_pcm, r_pcm, t_pcm, i_pcm])
        word_path = os.path.join(OUT_DIR, name + "_liberty.wav")
        write_wav(word_path, word)
        play(word_path)
        input("  Press Enter for next...")

# ---------------------------------------------------------------------------
# KH (k) variants
# ---------------------------------------------------------------------------
print()
print("=" * 60)
print("Generating KH.wav variants (aspirated K)...")
print("=" * 60)

k_variants = [
    ("KH_isolated_90ms",
     "_ 50\nk_h 90 50 100\n_ 50\n",
     "current — isolated, 90ms"),

    ("KH_isolated_140ms",
     "_ 50\nk_h 140 50 100\n_ 50\n",
     "isolated, 140ms (longer burst)"),

    ("KH_context_120ms",
     "_ 20\n@ 50 50 100\nk_h 120 50 100\n@ 50 50 100\n_ 20\n",
     "in @-k_h-@ context, 120ms"),

    ("KH_context_160ms",
     "_ 20\n@ 50 50 100\nk_h 160 50 100\n@ 50 50 100\n_ 20\n",
     "in @-k_h-@ context, 160ms"),
]

for name, pho, desc in k_variants:
    out = os.path.join(OUT_DIR, name + ".wav")
    if "context" in name:
        dur_ms = int(name.split("_")[-1].replace("ms",""))
        pcm = extract_consonant(pho, dur_ms)
    else:
        tmp = os.path.join(OUT_DIR, name + "_tmp.wav")
        run_mbrola(pho, tmp)
        pcm = trim_silence(read_pcm(tmp))
        os.unlink(tmp)
    if pcm is not None and len(pcm) > 0:
        write_wav(out, pcm)
        ms = int(len(pcm) / 16000 * 1000)
        print(f"  {name}.wav  ({ms}ms) — {desc}")

# Build complete word "bike" with each k variant
print()
print("Playing as complete word 'bike' (b AI k)...")
print()
b_wav  = trim_silence(read_pcm(os.path.join(WAV_DIR, "b.wav")))
ai_pcm = trim_silence(read_pcm(os.path.join(WAV_DIR, "AI.wav")))

for name, pho, desc in k_variants:
    out = os.path.join(OUT_DIR, name + ".wav")
    if os.path.exists(out):
        print(f"  [{desc}]")
        k_pcm = trim_silence(read_pcm(out))
        word = np.concatenate([b_wav, ai_pcm, k_pcm])
        word_path = os.path.join(OUT_DIR, name + "_bike.wav")
        write_wav(word_path, word)
        play(word_path)
        input("  Press Enter for next...")

print()
print("Done. To install a winner:")
print("  cp ~/mbrola_work/variants/b_context_80ms.wav TeensyTalkV2/wavs/b.wav")
print("  cp ~/mbrola_work/variants/KH_context_140ms.wav TeensyTalkV2/wavs/KH.wav")
print("  Then copy both to the SD card root.")
