// tts_rules.h
// Letter-to-sound rule engine for TeensyTalk V2.
// Rules are derived from eSpeak-ng dictsource/en_rules.
// Each rule has: left context, match string, right context, phoneme output.
// Rules are checked in order; first match wins.
// Output phonemes use MBROLA us2 names, space-separated.

#ifndef TTS_RULES_H
#define TTS_RULES_H

// Maximum phonemes a single word can produce
#define MAX_PHONEMES 64
#define MAX_PHONEME_LEN 8

// A single letter-to-sound rule
struct L2SRule {
  const char* lctx;    // required left context (chars before match), or ""
  const char* match;   // the grapheme(s) to consume
  const char* rctx;    // required right context (chars after match), or ""
  const char* phones;  // output phonemes, space-separated, or "" for silent
};

// Vowel set - used in rules as shorthand
static bool isVowel(char c) {
    return c=='a'||c=='e'||c=='i'||c=='o'||c=='u'||c=='y';
}
static bool isConsonant(char c) {
    return c>='a' && c<='z' && !isVowel(c);
}

// Check if left context matches (right-aligned against position)
// lctx is checked right-to-left against chars before pos
static bool matchLeft(const char* word, int pos, const char* lctx) {
    if (!lctx || lctx[0]==0) return true;
  int llen = strlen(lctx);
  if (pos < llen) return false;
  for (int i=0; i<llen; i++) {
    char need = lctx[llen-1-i];
    char have = word[pos-1-i];
    if (need=='V' && isVowel(have)) continue;   // V = any vowel
    if (need=='C' && isConsonant(have)) continue; // C = any consonant
    if (need != have) return false;
  }
  return true;
}

// Check if right context matches (left-aligned from pos+matchlen)
static bool matchRight(const char* word, int pos, const char* rctx) {
    if (!rctx || rctx[0]==0) return true;
  int rlen = strlen(rctx);
  for (int i=0; i<rlen; i++) {
    char need = rctx[i];
    char have = word[pos+i];
    if (have==0) return false;
    if (need=='V' && isVowel(have)) continue;
    if (need=='C' && isConsonant(have)) continue;
    if (need=='#') return (have==0);  // # = end of word
    if (need != have) return false;
  }
  return true;
}

// ---------------------------------------------------------------
// RULE TABLES - one array per initial letter
// Derived from eSpeak-ng en_rules, simplified for MBROLA us2 output
// ---------------------------------------------------------------

static const L2SRule rules_a[] = {
{ "",  "au",  "",    "O"        },  // audit, cause
{ "",  "aw",  "",    "O"        },  // law, saw
{ "",  "ai",  "",    "EI"       },  // rain, mail
{ "",  "ay",  "",    "EI"       },  // day, play
{ "",  "air", "",    "E r"      },  // air, fair
{ "",  "are", "#",   "E r"      },  // bare, care (silent e)
{ "",  "al",  "k",   "O"        },  //alk, talk
{ "",  "al",  "l",   "O"        },  // all, ball
{ "",  "al",  "m",   "A"        },  // calm, palm
{ "",  "a",   "tion","EI"       },  // nation
{ "",  "a",   "CV",  "EI"       },  // consonant then vowel = long A
{ "",  "a",   "C#",  "EI"       },  // silent E pattern
{ "",  "a",   "",    "{"        },  // default short A
};

static const L2SRule rules_b[] = {
{ "",  "bb",  "",    "b"        },  // rabbit (single sound)
{ "",  "b",   "#",   "b"        },  // end of word
{ "",  "b",   "",    "b"        },  // default
};

static const L2SRule rules_c[] = {
{ "",  "ch",  "",    "tS"       },  // chair, church
{ "",  "ck",  "",    "k"        },  // back, luck
{ "",  "cc",  "e",   "k s"      },  // accent
{ "",  "cc",  "i",   "k s"      },  // accident
{ "",  "cc",  "",    "k"        },  // account
{ "",  "cia", "",    "S"        },  // social
{ "",  "cion","",    "S @n"     },  // suspicion
{ "",  "c",   "e",   "s"        },  // cent, face
{ "",  "c",   "i",   "s"        },  // city, acid
{ "",  "c",   "y",   "s"        },  // cycle
{ "",  "c",   "",    "k"        },  // default hard C
};

static const L2SRule rules_d[] = {
{ "",  "dge", "",    "dZ"       },  // judge, edge
{ "",  "dd",  "",    "d"        },  // odd, add
{ "",  "d",   "",    "d"        },  // default
};

static const L2SRule rules_e[] = {
{ "",  "ea",  "r",   "r="       },  // ear, fear
{ "",  "ear", "",    "E r"      },  // bear, wear
{ "",  "ea",  "",    "i"        },  // eat, each
{ "",  "ee",  "r",   "I r"      },  // beer, deer
{ "",  "ee",  "",    "i"        },  // see, tree
{ "",  "eu",  "",    "j u"      },  // feud, few
{ "",  "ew",  "",    "j u"      },  // new, dew
{ "",  "ey",  "",    "i"        },  // key, honey
{ "",  "eigh","",    "EI"       },  // eight, weight
{ "",  "e",   "C#",  "i"        },  // pete (silent E makes long E)
{ "",  "e",   "#",   ""         },  // silent E at end
{ "",  "e",   "r",   "r="       },  // her, fern
{ "",  "e",   "",    "E"        },  // default short E
};

static const L2SRule rules_f[] = {
{ "",  "ff",  "",    "f"        },  // off, staff
{ "",  "f",   "",    "f"        },  // default
};

static const L2SRule rules_g[] = {
{ "",  "gh",  "",    ""         },  // light, night (silent)
{ "",  "gn",  "",    "n"        },  // gnome, sign
{ "",  "gg",  "",    "g"        },  // egg, bigger
{ "",  "g",   "e",   "dZ"       },  // gem, age
{ "",  "g",   "i",   "dZ"       },  // gin, magic
{ "",  "g",   "y",   "dZ"       },  // gym
{ "",  "g",   "",    "g"        },  // default hard G
};

static const L2SRule rules_h[] = {
{ "",  "h",   "",    "h"        },  // default
};

static const L2SRule rules_i[] = {
{ "",  "igh", "",    "AI"       },  // light, night, high
{ "",  "ie",  "",    "i"        },  // field, piece
{ "",  "ion", "",    "j @n"     },  // million, union
{ "",  "ir",  "",    "r="       },  // bird, girl
{ "",  "i",   "C#",  "AI"       },  // bike, kite (silent E)
{ "",  "i",   "nd",  "AI"       },  // find, mind, kind
{ "",  "i",   "ld",  "AI"       },  // mild, wild
{ "",  "i",   "",    "I"        },  // default short I
};

static const L2SRule rules_j[] = {
{ "",  "j",   "",    "dZ"       },  // default
};

static const L2SRule rules_k[] = {
{ "",  "kn",  "",    "n"        },  // knee, know, knife
{ "",  "k",   "",    "k"        },  // default
};

static const L2SRule rules_l[] = {
{ "",  "ll",  "",    "l"        },  // bell, full
{ "",  "l",   "",    "l"        },  // default
};

static const L2SRule rules_m[] = {
{ "",  "mm",  "",    "m"        },  // hammer
{ "",  "m",   "",    "m"        },  // default
};

static const L2SRule rules_n[] = {
{ "",  "ng",  "#",   "N"        },  // sing, ring (word final)
{ "",  "ng",  "C",   "N"        },  // finger, longer
{ "",  "ng",  "",    "n dZ"     },  // range, change
{ "",  "nn",  "",    "n"        },  // inn, connect
{ "",  "n",   "",    "n"        },  // default
};

static const L2SRule rules_o[] = {
{ "",  "oo",  "k",   "U"        },  // book, look, cook
{ "",  "oo",  "",    "u"        },  // food, moon, too
{ "",  "ow",  "",    "@U"       },  // low, show, know
{ "",  "oa",  "",    "@U"       },  // boat, coat
{ "",  "ou",  "gh",  "O"        },  // ought, thought
{ "",  "ou",  "",    "aU"       },  // out, found
{ "",  "oi",  "",    "OI"       },  // oil, coin
{ "",  "oy",  "",    "OI"       },  // boy, toy
{ "",  "or",  "",    "O r"      },  // for, more, door
{ "",  "o",   "CV",  "@U"       },  // hope, note (silent E)
{ "",  "o",   "C#",  "@U"       },  // go, no (word final)
{ "",  "o",   "",    "A"        },  // default short O
};

static const L2SRule rules_p[] = {
{ "",  "ph",  "",    "f"        },  // phone, graph
{ "",  "pp",  "",    "p"        },  // happy, apply
{ "",  "p",   "",    "p"        },  // default
};

static const L2SRule rules_q[] = {
{ "",  "qu",  "",    "k w"      },  // queen, quick
{ "",  "q",   "",    "k"        },  // default
};

static const L2SRule rules_r[] = {
{ "",  "rr",  "",    "r"        },  // carry, mirror
{ "",  "r",   "",    "r"        },  // default
};

static const L2SRule rules_s[] = {
{ "",  "sh",  "",    "S"        },  // ship, fish
{ "",  "sion","",    "Z @n"     },  // vision, occasion
{ "",  "tion","",    "S @n"     },  // nation, station
{ "",  "ss",  "",    "s"        },  // miss, class
{ "V", "s",   "V",   "z"        },  // between vowels = Z
{ "",  "s",   "#",   "z"        },  // plurals/verbs at end after vowel
{ "",  "s",   "",    "s"        },  // default
};

static const L2SRule rules_t[] = {
{ "",  "th",  "",    "T"        },  // think, path (default voiceless)
{ "",  "tch", "",    "tS"       },  // catch, match
{ "",  "tion","",    "S @n"     },  // nation, station
{ "",  "tia", "",    "S @"      },  // partial
{ "",  "tt",  "",    "t"        },  // butter, little
{ "",  "t",   "",    "t"        },  // default
};

static const L2SRule rules_u[] = {
{ "",  "ur",  "",    "r="       },  // burn, turn, fur
{ "",  "ue",  "",    "j u"      },  // cue, blue
{ "",  "ui",  "",    "u"        },  // fruit, suit
{ "",  "u",   "C#",  "j u"      },  // cute, mule (silent E)
{ "",  "u",   "",    "V"        },  // default short U
};

static const L2SRule rules_v[] = {
{ "",  "v",   "",    "v"        },  // default
};

static const L2SRule rules_w[] = {
{ "",  "wh",  "",    "w"        },  // what, where
{ "",  "wr",  "",    "r"        },  // write, wrong
{ "",  "w",   "",    "w"        },  // default
};

static const L2SRule rules_x[] = {
{ "^", "x",   "",    "z"        },  // xylophone (word initial)
{ "",  "x",   "",    "k s"      },  // default
};

static const L2SRule rules_y[] = {
{ "",  "y",   "#",   "i"        },  // happy, city (word final)
{ "",  "y",   "C",   "I"        },  // gym, myth
{ "",  "y",   "V",   "j"        },  // yes, yellow (before vowel)
{ "",  "y",   "",    "AI"       },  // my, by, fly
};

static const L2SRule rules_z[] = {
{ "",  "zz",  "",    "z"        },  // jazz, buzz
{ "",  "z",   "",    "z"        },  // default
};

// Rule table index - one entry per letter a-z
struct RuleSet {
  const L2SRule* rules;
  int count;
};

static const RuleSet RULE_SETS[26] = {
{ rules_a, sizeof(rules_a)/sizeof(rules_a[0]) },
{ rules_b, sizeof(rules_b)/sizeof(rules_b[0]) },
{ rules_c, sizeof(rules_c)/sizeof(rules_c[0]) },
{ rules_d, sizeof(rules_d)/sizeof(rules_d[0]) },
{ rules_e, sizeof(rules_e)/sizeof(rules_e[0]) },
{ rules_f, sizeof(rules_f)/sizeof(rules_f[0]) },
{ rules_g, sizeof(rules_g)/sizeof(rules_g[0]) },
{ rules_h, sizeof(rules_h)/sizeof(rules_h[0]) },
{ rules_i, sizeof(rules_i)/sizeof(rules_i[0]) },
{ rules_j, sizeof(rules_j)/sizeof(rules_j[0]) },
{ rules_k, sizeof(rules_k)/sizeof(rules_k[0]) },
{ rules_l, sizeof(rules_l)/sizeof(rules_l[0]) },
{ rules_m, sizeof(rules_m)/sizeof(rules_m[0]) },
{ rules_n, sizeof(rules_n)/sizeof(rules_n[0]) },
{ rules_o, sizeof(rules_o)/sizeof(rules_o[0]) },
{ rules_p, sizeof(rules_p)/sizeof(rules_p[0]) },
{ rules_q, sizeof(rules_q)/sizeof(rules_q[0]) },
{ rules_r, sizeof(rules_r)/sizeof(rules_r[0]) },
{ rules_s, sizeof(rules_s)/sizeof(rules_s[0]) },
{ rules_t, sizeof(rules_t)/sizeof(rules_t[0]) },
{ rules_u, sizeof(rules_u)/sizeof(rules_u[0]) },
{ rules_v, sizeof(rules_v)/sizeof(rules_v[0]) },
{ rules_w, sizeof(rules_w)/sizeof(rules_w[0]) },
{ rules_x, sizeof(rules_x)/sizeof(rules_x[0]) },
{ rules_y, sizeof(rules_y)/sizeof(rules_y[0]) },
{ rules_z, sizeof(rules_z)/sizeof(rules_z[0]) },
};

// Apply letter-to-sound rules to a single word.
// Fills phonOut with space-separated MBROLA phoneme names.
// Returns number of phonemes written, or -1 on error.
static int applyRules(const char* word, char phonOut[][MAX_PHONEME_LEN], int maxPhon) {
    int wlen = strlen(word);
  int pos = 0;
  int nPhon = 0;

  while (pos < wlen && nPhon < maxPhon) {
    char c = word[pos];

    // Only handle a-z
    if (c < 'a' || c > 'z') {
      pos++;
      continue;
    }

    int ri = c - 'a';
    const RuleSet& rs = RULE_SETS[ri];
    bool matched = false;

    for (int r = 0; r < rs.count && !matched; r++) {
      const L2SRule& rule = rs.rules[r];
      int mlen = strlen(rule.match);

      // Check match string against current position
      if (strncmp(word + pos, rule.match, mlen) != 0) continue;

      // Check left context
      if (!matchLeft(word, pos, rule.lctx)) continue;

      // Check right context (starts after match)
      if (!matchRight(word + pos + mlen, 0, rule.rctx)) continue;

      // Rule matched - emit phonemes
      // Parse space-separated phoneme tokens from rule.phones
      if (rule.phones[0] != 0) {
        char buf[32];
        strncpy(buf, rule.phones, sizeof(buf)-1);
        buf[sizeof(buf)-1] = 0;
        char* tok = strtok(buf, " ");
        while (tok && nPhon < maxPhon) {
          strncpy(phonOut[nPhon], tok, MAX_PHONEME_LEN-1);
          phonOut[nPhon][MAX_PHONEME_LEN-1] = 0;
          nPhon++;
          tok = strtok(NULL, " ");
        }
      }
      pos += mlen;
      matched = true;
    }

    if (!matched) {
      // No rule matched - skip character
      pos++;
    }
  }
  return nPhon;
}

#endif // TTS_RULES_H
