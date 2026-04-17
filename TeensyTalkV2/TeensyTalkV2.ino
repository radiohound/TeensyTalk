/* TeensyTalk V2
   Text-to-Speech using eSpeak-ng derived G2P rules + MBROLA us2 phoneme WAVs.

   Type a sentence in the Serial Monitor and press Enter to hear it spoken.

   Hardware: Teensy 3.2/3.5/3.6/4.0/4.1 + Teensy Audio Shield
   Audio:    MBROLA us2 WAV files at 16000 Hz on SD card
   Library:  Frank Boesing's Teensy-WavePlayer
             https://github.com/FrankBoesing/Teensy-WavePlayer

   Branch: espeak-ng-rewrite
*/

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <play_wav.h>

#if defined(__IMXRT1062__)
  #define T4
  #include <utility/imxrt_hw.h>
#else
  #define F_I2S ((((I2S0_MCR >> 24) & 0x03) == 3) ? F_PLL : F_CPU)
#endif

AudioPlayWav         playWav1;
AudioOutputI2S       outputsound;
AudioConnection      patchCord1(playWav1, 0, outputsound, 0);
AudioConnection      patchCord2(playWav1, 0, outputsound, 1);
AudioControlSGTL5000 sgtl5000_1;

#define SDCARD_CS_PIN   10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN  14

#include "tts_phonemes.h"
#include "tts_dict.h"
#include "tts_rules.h"
#include "tts_say.h"

// ---------------------------------------------------------------
// I2S frequency control
// ---------------------------------------------------------------
#ifndef T4
uint32_t I2S_dividers(float fsamp, uint32_t nbits, uint32_t tcr2_div) {
  unsigned fract, divi;
  fract = divi = 1;
  float minfehler = 1e7;
  unsigned x = (nbits * ((tcr2_div + 1) * 2));
  unsigned b = F_I2S / x;
  for (unsigned i = 1; i < 256; i++) {
    unsigned d = round(b / fsamp * i);
    float freq = b * i / (float)d;
    float fehler = fabs(fsamp - freq);
    if (fehler < minfehler && d < 4096) {
      fract = i; divi = d; minfehler = fehler;
      if (fehler == 0.0f) break;
    }
  }
  return I2S_MDR_FRACT((fract - 1)) | I2S_MDR_DIVIDE((divi - 1));
}
#endif

void setI2SFreq(int freq) {
#if defined(T4)
  int n1 = 4;
  int n2 = 1 + (24000000 * 27) / (freq * 256 * n1);
  double C = ((double)freq * 256 * n1 * n2) / 24000000;
  int c0 = C;
  int c2 = 10000;
  int c1 = C * c2 - (c0 * c2);
  set_audioClock(c0, c1, c2, true);
  CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK))
               | CCM_CS1CDR_SAI1_CLK_PRED(n1 - 1)
               | CCM_CS1CDR_SAI1_CLK_PODF(n2 - 1);
#else
  unsigned tcr5 = I2S0_TCR5;
  unsigned word0width = ((tcr5 >> 24) & 0x1f) + 1;
  unsigned wordnwidth = ((tcr5 >> 16) & 0x1f) + 1;
  unsigned framesize = ((I2S0_TCR4 >> 16) & 0x0f) + 1;
  unsigned nbits = word0width + wordnwidth * (framesize - 1);
  unsigned tcr2div = I2S0_TCR2 & 0xff;
  uint32_t MDR = I2S_dividers(freq, nbits, tcr2div);
  if (MDR > 0) {
    while (I2S0_MCR & I2S_MCR_DUF) { ; }
    I2S0_MDR = MDR;
  }
#endif
}

// ---------------------------------------------------------------
// setup()
// ---------------------------------------------------------------
void setup() {
  Serial.begin(9600);

  AudioMemory(10);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.6);

#if defined(ARDUINO_TEENSY41) || defined(ARDUINO_TEENSY36)
  if (!SD.begin(BUILTIN_SDCARD)) {
    while (1) { Serial.println("SD card failed"); delay(500); }
  }
#elif defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY32)
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!SD.begin(SDCARD_CS_PIN)) {
    while (1) { Serial.println("SD card failed"); delay(500); }
  }
#endif

  setI2SFreq(16000);

  Serial.println("TeensyTalk V2 ready. Type something and press Enter.");
}

// ---------------------------------------------------------------
// loop() - speak text typed into Serial Monitor
// ---------------------------------------------------------------

static char serialBuf[256];
static int  serialLen = 0;

void loop() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialLen > 0) {
        serialBuf[serialLen] = 0;
        Serial.print(">> ");
        Serial.println(serialBuf);
        say(serialBuf, playWav1);
        serialLen = 0;
      }
    } else {
      if (serialLen < (int)sizeof(serialBuf) - 1)
        serialBuf[serialLen++] = c;
    }
  }
}
