// test_rules.cpp
// Compile: g++ -std=c++11 -o test_rules test_rules.cpp
// Run:     ./test_rules

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// Stub out Arduino dependencies not needed for G2P logic
#define pgm_read_byte(x) (*(x))

#include "tts_dict.h"
#include "tts_rules.h"

static void testWord(const char* word) {
    char lower[64];
    strncpy(lower, word, sizeof(lower)-1);
    lower[sizeof(lower)-1] = 0;
    for (int i = 0; lower[i]; i++)
        if (lower[i] >= 'A' && lower[i] <= 'Z') lower[i] += 32;

    char phonBuf[MAX_PHONEMES][MAX_PHONEME_LEN];
    int nPhon = 0;

    const char* dictPhones = dictLookup(lower);
    const char* source;

    if (dictPhones) {
        source = "dict";
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
        source = "rules";
        nPhon = applyRules(lower, phonBuf, MAX_PHONEMES);
    }

    printf("%-20s [%s]  ", word, source);
    for (int i = 0; i < nPhon; i++) {
        printf("%s ", phonBuf[i]);
    }
    printf("\n");
}

int main() {
    printf("%-20s %-8s  %s\n", "WORD", "SOURCE", "PHONEMES");
    printf("------------------------------------------------------------\n");

    // Dict words - voiced/voiceless th, sh, zh, ng, strut, short-oo
    testWord("the");
    testWord("think");
    testWord("she");
    testWord("vision");
    testWord("sing");
    testWord("book");
    testWord("but");

    printf("\n");

    // Rules - silent E long vowels
    testWord("make");
    testWord("cake");
    testWord("time");
    testWord("hope");
    testWord("cute");
    testWord("hello");
    testWord("go");
    testWord("no");

    printf("\n");

    // Rules - digraphs and clusters
    testWord("rain");
    testWord("play");
    testWord("light");
    testWord("bird");
    testWord("bottle");
    testWord("butter");
    testWord("city");
    testWord("catch");
    testWord("judge");

    printf("\n");

    // Dict - tricky words
    testWord("north");
    testWord("there");
    testWord("through");
    testWord("change");
    testWord("before");
    testWord("course");
    testWord("more");
    testWord("four");
    testWord("your");
    testWord("order");
    testWord("short");
    testWord("important");

    printf("\n");

    // Rules - word-final patterns
    testWord("happy");
    testWord("city");
    testWord("baby");
    testWord("zero");
    testWord("echo");

    printf("\n");

    // Rules - ng variations
    testWord("ring");
    testWord("finger");
    testWord("range");
    testWord("long");
    testWord("coming");

    return 0;
}
