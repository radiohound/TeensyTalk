// test_regression.cpp  —  G2P regression test for TeensyTalk V2
// Compile: g++ -std=c++11 -I TeensyTalkV2 -o test_regression test_regression.cpp
// Run:     ./test_regression

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "tts_dict.h"
#include "tts_rules.h"

// ---------------------------------------------------------------------------
// Expected-output table.  Add new words here as you discover good/bad cases.
// Format: { "word", "expected phoneme string" }
// ---------------------------------------------------------------------------
struct TC { const char* word; const char* expected; };

static const TC TESTS[] = {

    // --- voiced / voiceless TH ---
    { "the",        "D @"           },
    { "this",       "D I s"         },
    { "that",       "D { t"         },
    { "there",      "D E r"         },
    { "think",      "T I N k"       },
    { "three",      "T r i"         },
    { "through",    "T r u"         },
    { "both",       "b @U T"        },
    { "month",      "m V n T"       },
    { "north",      "n O r T"       },

    // --- sh / zh / ng ---
    { "she",        "S i"           },
    { "short",      "S O r t"       },
    { "should",     "S U d"         },
    { "vision",     "v I Z @ n"      },
    { "sing",       "s I N"         },
    { "ring",       "r I N"         },
    { "long",       "l O N"         },
    { "coming",     "k V m I N"     },
    { "finger",     "f I N g r="    },

    // --- strut vowel (V) ---
    { "but",        "b V t"         },
    { "cut",        "k V t"         },
    { "run",        "r V n"         },
    { "under",      "V n d r="      },

    // --- short oo (U) ---
    { "book",       "b U k"         },
    { "look",       "l U k"         },
    { "could",      "k U d"         },
    { "would",      "w U d"         },
    { "good",       "g U d"         },

    // --- short I (IH) ---
    { "bit",        "b I t"         },
    { "big",        "b I g"         },
    { "it",         "I t"           },
    { "in",         "I n"           },
    { "if",         "I f"           },

    // --- short E (CVC words must not get long-E) ---
    { "red",        "r E d"         },
    { "bed",        "b E d"         },
    { "set",        "s E t"         },
    { "let",        "l E t"         },
    { "get",        "g E t"         },
    { "bullet",     "b V l E t"     },

    // --- silent-E long vowels ---
    { "make",       "m EI k"        },
    { "cake",       "k EI k"        },
    { "bike",       "b AI k"        },
    { "fine",       "f AI n"        },
    { "kite",       "k AI t"        },
    { "hope",       "h @U p"        },
    { "cute",       "k j u t"       },
    { "hive",       "h AI v"        },

    // --- word-final O ---
    { "hello",      "h E l @U"      },
    { "zero",       "z r= @U"       },
    { "echo",       "E k @U"        },

    // --- word-final Y ---
    { "happy",      "h { p i"       },
    { "baby",       "b EI b i"      },
    { "city",       "s I 4 i"       },

    // --- er / bird ---
    { "butter",     "b V t r="      },
    { "bird",       "b r= d"        },
    { "her",        "h r="          },
    { "fern",       "f r= n"        },

    // --- syllabic L ---
    { "bottle",     "b A t l="      },
    { "little",     "l I 4 @l"      },

    // --- flapped T ---
    { "butter",     "b V t r="      },
    { "city",       "s I 4 i"       },
    { "later",      "l EI 4 r="     },

    // --- or / Or split ---
    { "more",       "m O r"         },
    { "four",       "f O r"         },
    { "your",       "j O r"         },
    { "before",     "b I f O r"     },
    { "order",      "O r d r="      },
    { "short",      "S O r t"       },
    { "important",  "I m p O r t @ n t" },
    { "or",         "O r"           },

    // --- schwa / OW / AW ---
    { "the",        "D @"           },
    { "about",      "{ b aU t"      },
    { "over",       "@U v r="       },
    { "go",         "g @U"          },
    { "no",         "n @U"          },
    { "know",       "n @U"          },

    // --- digraphs ---
    { "rain",       "r EI n"        },
    { "play",       "p l EI"        },
    { "light",      "l AI t"        },
    { "catch",      "k { tS"        },
    { "judge",      "dZ V dZ"       },
    { "change",     "tS EI n dZ"    },
    { "phone",      "f @U n"        },

    // --- nge at word end ---
    { "range",      "r EI n dZ"     },
    { "strange",    "s t r EI n dZ" },

    // --- live / alive ---
    { "live",       "l I v"         },
    { "alive",      "@ l AI v"      },
    { "give",       "g I v"         },
    { "drive",      "d r AI v"      },

    // --- numbers (via say) ---
    { "one",        "w V n"         },
    { "two",        "t u"           },
    { "three",      "T r i"         },
    { "four",       "f O r"         },
    { "five",       "f AI v"        },
    { "six",        "s I k s"       },
    { "seven",      "s E v @ n"      },
    { "eight",      "EI t"          },
    { "nine",       "n AI n"        },
    { "ten",        "t E n"         },
    { "hundred",    "h V n d r @d"  },
    { "thousand",   "T aU z @ n d"   },

    { NULL, NULL }
};

// ---------------------------------------------------------------------------

static void getPhonemes(const char* word, char out[MAX_PHONEMES][MAX_PHONEME_LEN],
                        int* nPhon, bool* fromDict) {
    *nPhon = 0;
    const char* d = dictLookup(word);
    *fromDict = (d != NULL);
    if (d) {
        char tmp[128];
        strncpy(tmp, d, 127); tmp[127] = 0;
        char* tok = strtok(tmp, " ");
        while (tok && *nPhon < MAX_PHONEMES) {
            strncpy(out[*nPhon], tok, MAX_PHONEME_LEN-1);
            out[*nPhon][MAX_PHONEME_LEN-1] = 0;
            (*nPhon)++;
            tok = strtok(NULL, " ");
        }
    } else {
        *nPhon = applyRules(word, out, MAX_PHONEMES);
    }
}

static void phonemesToStr(char out[MAX_PHONEMES][MAX_PHONEME_LEN], int n,
                          char* buf, int bufsz) {
    buf[0] = 0;
    for (int i = 0; i < n; i++) {
        if (i > 0) strncat(buf, " ", bufsz-1);
        strncat(buf, out[i], bufsz-1);
    }
}

int main() {
    int pass = 0, fail = 0;

    for (int i = 0; TESTS[i].word != NULL; i++) {
        char phonBuf[MAX_PHONEMES][MAX_PHONEME_LEN];
        int nPhon; bool fromDict;
        getPhonemes(TESTS[i].word, phonBuf, &nPhon, &fromDict);

        char got[256] = "";
        phonemesToStr(phonBuf, nPhon, got, sizeof(got));

        bool ok = (strcmp(got, TESTS[i].expected) == 0);
        if (ok) {
            pass++;
        } else {
            fail++;
            printf("FAIL  %-16s  got: %-30s  expected: %s  [%s]\n",
                   TESTS[i].word, got, TESTS[i].expected,
                   fromDict ? "dict" : "rules");
        }
    }

    printf("\n%d passed, %d failed out of %d tests.\n",
           pass, fail, pass + fail);
    return fail > 0 ? 1 : 0;
}
