/*
  TeensyTalk.ino Rev5

  Example of using phonemes for Text To Speech sythesis

  Open the Serial Monitor and type in words to hear see the results.

  created March 15 2023
  Rev5 March 26 2023
  by Walter Dunckel

  This uses Frank Boesing's WavePlayer library in order to play 22050 Hertz wav files
  


*/
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <play_wav.h>       //https://github.com/FrankBoesing/Teensy-WavePlayer

#if defined(__IMXRT1062__)
#define T4
#include <utility/imxrt_hw.h> // make available set_audioClock() for setting I2S freq on Teensy 4
#else
#define F_I2S ((((I2S0_MCR >> 24) & 0x03) == 3) ? F_PLL : F_CPU) // calculation for I2S freq on Teensy 3
#endif

// GUItool: begin automatically generated code
AudioPlayWav             playWav1;     //xy=210,161
AudioOutputI2S           outputsound;       //xy=417,124
AudioConnection          patchCord1(playWav1, 0, outputsound, 0);
AudioConnection          patchCord2(playWav1, 0, outputsound, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=413,177
// GUItool: end automatically generated code

#define SDCARD_CS_PIN    10 //10
#define SDCARD_MOSI_PIN  7  //11
#define SDCARD_SCK_PIN   14 //13

String txtMsg = "";                               // a string for incoming text

/* Say any number between -999,999 and 999,999 */
void sayNumber(long n) {
  if (n<0) {
    say("negative");
    while (playWav1.isPlaying()){
      delay(1);  
    } 
    //delay(49);
    sayNumber(-n);
    while (playWav1.isPlaying()){
      delay(1);  
    } 
  } else if (n==0) {
    say("zero");
  } else {
    if (n>=1000) {
      int thousands = n / 1000;
      sayNumber(thousands);
      while (playWav1.isPlaying()){
        delay(1);  
      } 
      //delay(49);
      say("thousand");
      while (playWav1.isPlaying()){
        delay(1);  
      } 
      //delay(49);
      n %= 1000;
      if ((n > 0) && (n<100)) say("and");
    }
    if (n>=100) {
      int hundreds = n / 100;
      sayNumber(hundreds);
      while (playWav1.isPlaying()){
        delay(1);  
      } 
      //delay(49);
      say("hundred");
      while (playWav1.isPlaying()){
        delay(1);  
      } 
      //delay(49);
      n %= 100;
      if (n > 0) say("and");
      while (playWav1.isPlaying()){
        delay(1);  
      } 
      //delay(49);
    }
    if (n>19) {
      int tens = n / 10;
      switch (tens) {
        case 2: say("twenty"); break;
        case 3: say("thirty"); break;
        case 4: say("fourty"); break;
        case 5: say("fifty"); break;
        case 6: say("sixty"); break;
        case 7: say("seventy"); break;
        case 8: say("aytee"); break;
        case 9: say("ninety"); break;
      }
      while (playWav1.isPlaying()){
        delay(1);  
      } 
      //delay(49);
      n %= 10;
    }
    switch(n) {
      case 1: say("1"); break;
      case 2: say("2"); break;
      case 3: say("3"); break;
      case 4: say("4"); break;
      case 5: say("5"); break;
      case 6: say("6"); break;
      case 7: say("7"); break;
      case 8: say("8"); break;
      case 9: say("9"); break;
      case 10: say("ten"); break;
      case 11: say("eleven"); break;
      case 12: say("twelve"); break;
      case 13: say("thirteen"); break;
      case 14: say("fourteen"); break;
      case 15: say("fifteen"); break;
      case 16: say("sixteen"); break;
      case 17: say("7teen"); break;
      case 18: say("8teen"); break;
      case 19: say("nineteen"); break;
    }
    while (playWav1.isPlaying()){
      delay(1);  
    } 
    //delay(49);
  }
}

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  //while (!Serial) {
  //  ;  // wait for serial port to connect. Needed for native USB port only
  //}

  AudioMemory(10); //was 50
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.6);

#if defined(ARDUINO_TEENSY41)
  //Teensy 4.1 code using build in SDCARD
  if (!(SD.begin(BUILTIN_SDCARD))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the built in SD card");
      delay(500);
    }
  }
#elif defined(ARDUINO_TEENSY40)
  //Teensy 4.0 code using the Audio Shield SD CARD
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the Audio Shield SD card");
      delay(500);
    }
  } 
#elif defined(ARDUINO_TEENSY32) 
  //For use with Teensy 3.2 with SDCard on audio shield
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the Audio Shield SD card");
      delay(500);
    }
  } 
#elif defined(ARDUINO_TEENSY36)
  //Teensy 3.6 using built in SDCARD  
  if (!(SD.begin(BUILTIN_SDCARD))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the built in SD card");
      delay(500);
    }
  }
#endif
  // default normal sounding tone for speech. Raise to raise frequency
  setI2SFreq(22050); 
}

#ifdef T4
#else
// calculate I2S dividers for Teensy 3
uint32_t I2S_dividers( float fsamp, uint32_t nbits, uint32_t tcr2_div )
{

  unsigned fract, divi;
  fract = divi = 1;
  float minfehler = 1e7;

  unsigned x = (nbits * ((tcr2_div + 1) * 2));
  unsigned b = F_I2S / x;

  for (unsigned i = 1; i < 256; i++) {

    unsigned d = round(b / fsamp * i);
    float freq = b * i / (float)d ;
    float fehler = fabs(fsamp - freq);

    if ( fehler < minfehler && d < 4096 ) {
      fract = i;
      divi = d;
      minfehler = fehler;
      if (fehler == 0.0f) break;
    }

  }

  return I2S_MDR_FRACT( (fract - 1) ) | I2S_MDR_DIVIDE( (divi - 1) );
}
#endif

// set I2S samplerate
void setI2SFreq(int freq) {
#if defined(T4)
  // PLL between 27*24 = 648MHz und 54*24=1296MHz
  int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
  int n2 = 1 + (24000000 * 27) / (freq * 256 * n1);
  double C = ((double)freq * 256 * n1 * n2) / 24000000;
  int c0 = C;
  int c2 = 10000;
  int c1 = C * c2 - (c0 * c2);
  set_audioClock(c0, c1, c2, true);
  CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK))
       | CCM_CS1CDR_SAI1_CLK_PRED(n1-1) // &0x07
       | CCM_CS1CDR_SAI1_CLK_PODF(n2-1); // &0x3f
#else
  unsigned tcr5 = I2S0_TCR5;
  unsigned word0width = ((tcr5 >> 24) & 0x1f) + 1;
  unsigned wordnwidth = ((tcr5 >> 16) & 0x1f) + 1;
  unsigned framesize = ((I2S0_TCR4 >> 16) & 0x0f) + 1;
  unsigned nbits = word0width + wordnwidth * (framesize - 1 );
  unsigned tcr2div = I2S0_TCR2 & 0xff; //bitclockdiv
  uint32_t MDR = I2S_dividers(freq, nbits, tcr2div);
  if (MDR > 0) {
    while (I2S0_MCR & I2S_MCR_DUF) {
      ;
    }
    I2S0_MDR = MDR;
  }
#endif
}

void loop() {
  delay(1000);  
  long altitude = random(1, 9999);
  setI2SFreq(25050);
  say("resistance is fuetiel.");
  setI2SFreq(22050);
  delay(200);
  say("thee,");
  say("height,");
  say("is,,,,");
  sayNumber(altitude);
  delay(75);
  say("feet,,,,,,");
  say("or,,,,");
  sayNumber(altitude/3.03);
  delay(75);
  say("meters,,,,,,,,,");
  say("north");
  say("west,,,,,");
  say("south east,,,,,");
  say("up, down, turn left, turn right,,,on,,,,off,,");
  delay(2000);
  setI2SFreq(28050);  // Raised to raise frequency of voice
  say("your off to great places, today is your day, your mountain is waiting .. so ,, get on your way");
  delay(10000);
  
}

void say(String txtMsg) {
  String vowels = "aeiouy";
  String sisz = "bdglmnhv"; //for rule if words end in this letter, followed by an s, then it sounds like a z
  String ciss = "iey";      //when c is followed by i, e or y it will sound like s 
  //setI2SFreq(22050);
  //Serial.println(txtMsg);
  txtMsg += "\r\n"; //this is used in code below, so need to use it here untill I edit the rest of the code
  //todo : change to char string, not String for txtMsg
    for (unsigned int i=0; i<(txtMsg.length()-2); i++) {  //read each character without cariage return
      int letter = txtMsg.charAt(i);
      switch (letter) {
         case '0':
            //Serial.print("Z");
            playWav1.play("Z.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("IY");
            playWav1.play("IY.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("R");
            playWav1.play("R.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            }   
            //Serial.print("OW");
            playWav1.play("OW.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            }    
        break;       

        case '1':
            //Serial.print("W");
            playWav1.play("W.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("AH");
            playWav1.play("AH.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("N");
            playWav1.play("N.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            }         
        break;

        case '2':
            //Serial.print("T");
            playWav1.play("T.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("UW");
            playWav1.play("UW.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
        break;
        
        case '3':
            //Serial.print("TH");
            playWav1.play("TH.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("R");
            playWav1.play("R.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("IY");
            playWav1.play("IY.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            }           
        break;

        case '4':
            //Serial.print("F");
            playWav1.play("F.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("OH");
            playWav1.play("OH.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("R");
            playWav1.play("R.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            }         
        break;

        case '5':
            //Serial.print("F");
            playWav1.play("F.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("AY");
            playWav1.play("AY.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("V");
            playWav1.play("V.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            }           
        break;

        case '6':
            //Serial.print("S");
            playWav1.play("S.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("IH");
            playWav1.play("IH.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("K");
            playWav1.play("K.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            }        
            //Serial.print("S");
            playWav1.play("S.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
        break;

        case '7':
            //Serial.print("S");
            playWav1.play("S.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("EH");
            playWav1.play("EH.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("V");
            playWav1.play("V.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            }       
            //Serial.print("EH");
            playWav1.play("EH.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("N");
            playWav1.play("N.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
        break;
        
        case '8':
            //Serial.print("EY");
            playWav1.play("EY.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("T");
            playWav1.play("T.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
        break;

        case '9':
            //Serial.print("N");
            playWav1.play("N.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("AY");
            playWav1.play("AY.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("N");
            playWav1.play("N.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            }           
        break;       
  
        case 'a':
          if (txtMsg.charAt(i+1)=='u')  {  //baud, laud, maud
              i++; 
              //Serial.print("AA");
              playWav1.play("AA.wav");
            
          } else if (vowels.indexOf(txtMsg.charAt(i+1)) != -1) { //if next letter is a vowel, do long vowel sound and skip sound of next vowel
            i++; //skip i sound, because it is included in phoneme below
            //Serial.print("EY");
            playWav1.play("EY.wav");
          } else if ((txtMsg.charAt(i+1)=='n') && (txtMsg.charAt(i+2)=='y')) { 
              //Serial.print("EH");
              playWav1.play("EH.wav");
          } else if ((vowels.indexOf(txtMsg.charAt(i+1)) == -1) && (vowels.indexOf(txtMsg.charAt(i+2)) != -1))  { // if there is a consonant, then a vowel
              //Serial.print("EY");
              playWav1.play("EY.wav");
          } else if ((txtMsg.charAt(i+1)=='c') && ((txtMsg.charAt(i+2)=='h') || (txtMsg.charAt(i+3)=='e'))) {
              //Serial.print("EY");
              playWav1.play("EY.wav"); 
          } else if ((txtMsg.charAt(i-1)=='g') && (txtMsg.charAt(i+1)=='l')) {
              //Serial.print("AX");
              playWav1.play("AX.wav");
          } else if (txtMsg.charAt(i+1)=='l') { 
              //Serial.print("AO");
              playWav1.play("AO.wav");
          } else {
              //Serial.print("AE"); 
              playWav1.play("AE.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          } 
        break;

        case 'b':
          if (txtMsg.charAt(i+1)=='b') {
            //skip
          } else {
              //Serial.print("B");
              playWav1.play("B.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          }  
        break;

        case 'c':
          if ((txtMsg.charAt(i+1)=='i') && (txtMsg.charAt(i+2)=='a') && (txtMsg.charAt(i+3)=='l'))  {
              i=i+2;
              //Serial.print("SH");
              playWav1.play("SH.wav");
              while (playWav1.isPlaying()){
                delay(1);  
              } 
              //Serial.print("UL");
              playWav1.play("UL.wav"); 
          } else if ((txtMsg.charAt(i+1)=='i') && ((txtMsg.charAt(i+2)=='e') || (txtMsg.charAt(i+2)=='a'))) { 
              //Serial.print("SH");
              playWav1.play("SH.wav");   
          } else if ((txtMsg.charAt(i-1)=='a') && ((txtMsg.charAt(i+1)=='h') && (txtMsg.charAt(i+2)=='e'))) { //ache
              i++;
              //Serial.print("K");
              playWav1.play("K.wav");      
          } else if (txtMsg.charAt(i-1)=='x') {  
              delay(1);
          } else if ((txtMsg.charAt(i+1)=='e') || (txtMsg.charAt(i+1)=='i')) {  
            //Serial.print("S");
            playWav1.play("S.wav");
          } else if (txtMsg.charAt(i+1)=='h') {  
              i++;
              //Serial.print("CH");
              playWav1.play("CH.wav");      
          } else if (txtMsg.charAt(i+1)=='y') {  
              //Serial.print("S");
              playWav1.play("S.wav");                        
          } else {  
              //Serial.print("K"); 
              playWav1.play("K.wav"); 
          }
          while (playWav1.isPlaying()){
            delay(1);  
          } 
        break;

        case 'd':
          if (txtMsg.charAt(i+1)=='d') { 
            //skip first sound
          } else {         
              //Serial.print("D");
              playWav1.play("D.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          } 
        break;

        case 'e':
          if ((txtMsg.charAt(i+1)=='o') && (txtMsg.charAt(i+2)=='u')) { 
            i=i+2; //skip sound, because it is included in phoneme below
            //Serial.print("IY(e1)");
            playWav1.play("IY.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
              //Serial.print("AX(e1)");
              playWav1.play("AX.wav");
          } else if ((txtMsg.charAt(i-3)=='t') && (txtMsg.charAt(i-2)=='e') && (txtMsg.charAt(i-1)=='l')) { //'tele'phone (takes care of second e)
              //Serial.print("EH(e2)");
              playWav1.play("EH.wav");  
          } else if ((txtMsg.charAt(i-1)=='t') && (txtMsg.charAt(i+1)=='l') && (txtMsg.charAt(i+2)=='e')) { //'tele'phone (takes care of first e)
              //Serial.print("EH(e3)");
              playWav1.play("EH.wav");   
          } else if ((txtMsg.charAt(i-2)=='c') && (txtMsg.charAt(i-1)=='r')  && (txtMsg.charAt(i+1)=='a') && (txtMsg.charAt(i+2)=='t') && (txtMsg.charAt(i+3)=='e')) {
              i++; //create 
              //Serial.print("IY(e5)");
              playWav1.play("IY.wav");  
              while (playWav1.isPlaying()){
                delay(1); 
              }
              delay(30); //add delay between syllables 
              //Serial.print("EY(e5)");
              playWav1.play("EY.wav");   
          } else if ((txtMsg.charAt(i-1)=='r') && (txtMsg.charAt(i+1)=='a') && (txtMsg.charAt(i+2)=='c') && (txtMsg.charAt(i+3)=='t')) {
              i++;
              //Serial.print("IY(e6)");
              playWav1.play("IY.wav");
              while (playWav1.isPlaying()){
                delay(1); 
              }
              //Serial.print("AE(e6)");
              playWav1.play("AE.wav");
          } else if (((txtMsg.charAt(i-1)=='l') || (txtMsg.charAt(i-1)=='n')) && ((txtMsg.charAt(i+1)=='s') && (txtMsg.charAt(i+2)=='s'))) {    
              //Serial.print("EH(e7)");
              playWav1.play("EH.wav");
          } else if (((txtMsg.charAt(i-2)=='o') && (txtMsg.charAt(i-1)=='v')) && ((txtMsg.charAt(i+1)=='n') || (txtMsg.charAt(i+1)=='r'))) {    
              //Serial.print("EH(e8)");
              playWav1.play("EH.wav");  
          } else if (((txtMsg.charAt(i-2)=='n') && ((txtMsg.charAt(i-1)=='d') || (txtMsg.charAt(i-1)=='t')))) {    
              //Serial.print("EH(e8b)");
              playWav1.play("EH.wav");  
          } else if ((txtMsg.charAt(i-2) == 'd') && (txtMsg.charAt(i-1) == 'r'))  { //exception for hun'dred'
              //Serial.print("EH(e8d)");
              playWav1.play("EH.wav");            
          } else if ((vowels.indexOf(txtMsg.charAt(i-2)) == -1) && (vowels.indexOf(txtMsg.charAt(i-1)) == -1) &&  (txtMsg.charAt(i+1)=='d') && (txtMsg.length() - 4)==i)  {
              //silent E when used with words like missed, rushed, tipped linked stuffed - makes T sound
              i++;
              //Serial.print("T(e8c)");
              playWav1.play("T.wav");
          } else if ((vowels.indexOf(txtMsg.charAt(i-2)) == -1) && (txtMsg.charAt(i-1)=='v') && (txtMsg.charAt(i+1)=='s') && (txtMsg.length() - 4)==i)  {
              // silent E for calves drarves starves
          } else if ((vowels.indexOf(txtMsg.charAt(i-2)) != -1) && ((txtMsg.charAt(i-1)=='s') || (txtMsg.charAt(i-1)=='c') || (txtMsg.charAt(i-1)=='v') || (txtMsg.charAt(i-1)=='z') || (txtMsg.charAt(i-1)=='k')) && (txtMsg.charAt(i+1)=='d') && (txtMsg.length() - 4)==i)  {
              //silent E when used with words like laced spaced, lazed, graced praised 
              //Serial.print("-(e9a)");      
          } else if ((vowels.indexOf(txtMsg.charAt(i-2)) != -1) && ((txtMsg.charAt(i-1)!='s') && (txtMsg.charAt(i-1)!='c') && (txtMsg.charAt(i-1)!='z')) && (txtMsg.charAt(i+1)=='s') && (txtMsg.length() - 4)==i)  { // vowel, then e then s with e silent if not after s,c or z
              //Serial.print("-(e9)"); 
          } else if ((txtMsg.length() - 3 == i) && (txtMsg.length() < 6) && ((txtMsg.charAt(i-1)=='h') || (txtMsg.charAt(i-1)=='b') || (txtMsg.charAt(i-1)=='m'))) {  //he or be she
              //Serial.print("IY(e10)");
              playWav1.play("IY.wav");
          } else if ((txtMsg.charAt(i-1)=='h') && (txtMsg.charAt(i+1)=='i') && (txtMsg.charAt(i+2)=='g') && (txtMsg.charAt(i+3)=='h')) { 
              i=i+3; 
              //Serial.print("AY(e15)");
              playWav1.play("AY.wav");    
          } else if ((txtMsg.charAt(i-1)=='c') && (txtMsg.charAt(i+1)=='i')) { //receipt
              i++;
              //Serial.print("IY(e11)");
              playWav1.play("IY.wav");  
          } else if ((txtMsg.length() == 5) && (txtMsg.length() -3 == i) && (txtMsg.charAt(i-2)=='t') && (txtMsg.charAt(i-1)=='h')) {
                //Serial.print("UH(e12)");
                playWav1.play("UH.wav"); 
          } else if ((txtMsg.charAt(i+1)=='t') && (txtMsg.charAt(i-1)=='i') && (txtMsg.charAt(i+2)=='y')) { 
              i=i+2; //skip sound, because it is included in phoneme below
              //Serial.print("IX(e13)");
              playWav1.play("IX.wav");
              while (playWav1.isPlaying()){
                delay(1);  
              } 
              //Serial.print("T(e13)");
              playWav1.play("T.wav");
              while (playWav1.isPlaying()){
                delay(1);  
              } 
              //Serial.print("IY(e13)");
              playWav1.play("IY.wav");
          } else if ((txtMsg.charAt(i+1)=='t') && (txtMsg.charAt(i+2)=='y')) { 
              i=i+2; 
              //Serial.print("T(e14)");
              playWav1.play("T.wav");
              while (playWav1.isPlaying()){
                delay(1);  
              } 
              //Serial.print("IY(e14)");
              playWav1.play("IY.wav");
          } else if ((txtMsg.charAt(i+1)=='a') && ((txtMsg.charAt(i+2)==' ') || (txtMsg.charAt(i+2)=='\r') || (txtMsg.charAt(i+2)=='s'))) { 
              i++; 
              //Serial.print("IY(e15)");
              playWav1.play("IY.wav");
          } else if ((txtMsg.charAt(i+1)=='c') && (txtMsg.charAt(i+2)=='i')) { 
              //Serial.print("EH(e16)");
              playWav1.play("EH.wav");
          } else if (txtMsg.charAt(i+1)=='o') { 
              i++; 
              //Serial.print("IY(e16)");
              playWav1.play("IY.wav");
              while (playWav1.isPlaying()){
                delay(1);  
              } 
              //Serial.print("OW(e16)");
              playWav1.play("OW.wav");
          } else if (txtMsg.charAt(i+1)=='w') { 
              i++; 
              //Serial.print("UW(e16)");
              playWav1.play("UW.wav") ;
          } else if (vowels.indexOf(txtMsg.charAt(i+2)) != -1) {
              //Serial.print("IY(e17)");
              playWav1.play("IY.wav");
          } else if ((txtMsg.charAt(i-1)=='h') && (txtMsg.charAt(i+1)=='a')  && (txtMsg.charAt(i+2)=='d')) {
              i++;
              //Serial.print("EH(e17)");
              playWav1.play("EH.wav"); 
          } else if (((txtMsg.charAt(i-2)=='d') || (txtMsg.charAt(i-2)=='t') || (txtMsg.charAt(i-2)=='p') || (txtMsg.charAt(i-2)=='b')) && (txtMsg.charAt(i-1)=='r') && (txtMsg.charAt(i+1)=='a')  && (txtMsg.charAt(i+2)=='d')) {
              i++; // bread, spread, dread, tread
              //Serial.print("EH(e18)");
              playWav1.play("EH.wav");  
          } else if (((txtMsg.charAt(i-1)=='b') || (txtMsg.charAt(i-1)=='t') || (txtMsg.charAt(i-1)=='w')) && (txtMsg.charAt(i+1)=='a')  && (txtMsg.charAt(i+2)=='r')) {
              i++; //bear tear wear
              //Serial.print("EY(e19)");
              playWav1.play("EY.wav");  
          } else if ((txtMsg.charAt(i-2)=='s') && (txtMsg.charAt(i-1)=='t') && (txtMsg.charAt(i+1)=='a') && (txtMsg.charAt(i+2)=='k')) {
              i++; //steak
              //Serial.print("EY(e20)");
              playWav1.play("EY.wav"); 
          } else if ((txtMsg.charAt(i-2)=='b') && (txtMsg.charAt(i-1)=='r') && (txtMsg.charAt(i+1)=='a') && (txtMsg.charAt(i+2)=='k')) {
              i++; //break
              //Serial.print("EY(e21)");
              playWav1.play("EY.wav"); 
          } else if ((txtMsg.charAt(i+1)=='e') || (txtMsg.charAt(i+1)=='a')) {  // if ea or ee this is default ea sound. Make sure exceptions are above
              i++;
              //Serial.print("IY(e22)");
              playWav1.play("IY.wav");              
          } else if (txtMsg.charAt(i+1)=='y') { 
              i++;
              //Serial.print("EY(e23)");
              playWav1.play("EY.wav");  
          } else if (vowels.indexOf(txtMsg.charAt(i+1)) != -1) {
              i++;
              //Serial.print("EH(e24)");
              playWav1.play("EH.wav");   
          } else if (txtMsg.charAt(i+1)=='r') { 
              //Serial.print("E(e25)");
              playWav1.play("E.wav");
          } else if ((txtMsg.length() == 5) && (txtMsg.length() -3 == i) && (txtMsg.charAt(i-2)=='t') && (txtMsg.charAt(i-1)=='h')) {
              //Serial.print("UH(e26)");
              playWav1.play("UH.wav");
          } else if (txtMsg.charAt(i+1)=='w') { 
              i++; 
              //Serial.print("UW(e27)");
              playWav1.play("UW.wav");
          } else if (txtMsg.charAt(i+1)=='\r') { 
              //Serial.print(" (e28)");
              while (playWav1.isPlaying()){
                delay(1);  
              } 
          } else if (txtMsg.charAt(i+1)==' ') { 
              //Serial.print(" (e29)");
          } else { 
              //Serial.print("EH(e30)");
              playWav1.play("EH.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          }  
        break;

        case 'f':
          if (txtMsg.charAt(i+1)=='f') { 
            //skip first sound
          } else { 
              //Serial.print("F");
              playWav1.play("F.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          } 
        break;         

        case 'g':
          if (txtMsg.charAt(i+1)=='g')  {
            i++; //skip letter
            //Serial.print("G"); 
            playWav1.play("G.wav");
          } else if ((txtMsg.charAt(i+1)=='h') && (txtMsg.charAt(i+1)=='t'))  { 
              i=i+2; //skip sound for gh
          } else if ((i==0) && (txtMsg.charAt(i+1)=='n')) {  // gnome dont make a sound
          } else if ((txtMsg.charAt(i+1)=='e') && (txtMsg.charAt(i+2)=='t')) {
              //Serial.print("G"); 
              playWav1.play("G.wav");
          } else if ((txtMsg.charAt(i+1)=='e') && (txtMsg.charAt(i+2)=='l')){ 
            Serial.print("J");
            playWav1.play("J.wav");
          } else if (txtMsg.charAt(i+1)=='y') { 
              i++; //
              //Serial.print("J");
              playWav1.play("J.wav");
              while (playWav1.isPlaying()){
                delay(1);  
              } 
              //Serial.print("IY");
              playWav1.play("IY.wav"); 
          } else { 
              //Serial.print("G"); 
              playWav1.play("G.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          } 
        break;
        
        case 'h':
          //Serial.print("_H");
          playWav1.play("_H.wav");
          while (playWav1.isPlaying()){
            delay(1);  
          } 
        break;   

        case 'I':
          //Serial.print("AY");
          playWav1.play("AY.wav");
          while (playWav1.isPlaying()){
            delay(1);  
          } 
        break;   

        case 'i':
          if ((txtMsg.charAt(i+1)=='e') && (txtMsg.charAt(i+2)=='n') && (txtMsg.charAt(i+3)=='t'))  {  //efficient
              //Serial.print("IY");
              playWav1.play("IY.wav");    
            
          } else if ((txtMsg.charAt(i-1)=='t') && (txtMsg.charAt(i+1)=='v') && (txtMsg.charAt(i+2)=='e'))  { //tive exception
              //Serial.print("IX");
              playWav1.play("IX.wav");        
          } else if ((txtMsg.charAt(i-1)=='n') && (txtMsg.charAt(i+1)=='o') && (txtMsg.charAt(i+2)=='n'))  { //onion exception
              //Serial.print("Y");
              playWav1.play("Y.wav");  
          } else if (((txtMsg.charAt(i-1)=='k')|| (txtMsg.charAt(i-1)=='l') || (txtMsg.charAt(i-1)=='p') || (txtMsg.charAt(i-1)=='t')) && (txtMsg.charAt(i+1)=='e'))  { //lies,ties,skies,pies
              i++;
              //Serial.print("AY");
              playWav1.play("AY.wav");          
          } else if ((i > 1) && (txtMsg.charAt(i+1) =='o'))  { //radio
              i++;
              //Serial.print("IY");
              playWav1.play("IY.wav"); 
              while (playWav1.isPlaying()){
                delay(1);  
              }
              //Serial.print("OH");
              playWav1.play("OH.wav");              
          } else if ((vowels.indexOf(txtMsg.charAt(i+1)) == -1) && (vowels.indexOf(txtMsg.charAt(i+2)) != -1))  { // if there is a consonant, then a vowel
              //Serial.print("AY");
              playWav1.play("AY.wav");
          } else if (((txtMsg.charAt(i-2)=='c') || (txtMsg.charAt(i-2)=='t') || (txtMsg.charAt(i-2)=='f')) && ((txtMsg.charAt(i-1)=='r') && (txtMsg.charAt(i+1)=='e')))  { //trie,crie,frie
              i++;
              //Serial.print("AY");
              playWav1.play("AY.wav");      
          } else if ((txtMsg.charAt(i+1)=='e') && (txtMsg.charAt(i+2)=='c'))  { // iec
              i++;              
              //Serial.print("IY");
              playWav1.play("IY.wav");
          } else if ((txtMsg.charAt(i+1)=='e') && (txtMsg.charAt(i+2)=='f')) { //grief
              i++;
              //Serial.print("IY");
              playWav1.play("IY.wav");  
          } else if ((txtMsg.charAt(i+1)=='e') && (txtMsg.charAt(i+2)=='w')) {  //view
              //Serial.print("Y");
              playWav1.play("Y.wav");      
          } else if (vowels.indexOf(txtMsg.charAt(i+1)) != -1) {  // vowel after 
              //Serial.print("AY");
              playWav1.play("AY.wav"); 
          } else if (txtMsg.charAt(i+1)=='r') { 
              i++; 
              //Serial.print("ER");
              playWav1.play("ER.wav");
          } else if ((txtMsg.charAt(i+1)=='g') && (txtMsg.charAt(i+2)=='h')) { 
              i=i+2;
              //Serial.print("AY");
              playWav1.play("AY.wav");     
          } else { 
              //Serial.print("IH");
              playWav1.play("IH.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          } 
        break;

        case 'j':
          //Serial.print("J");
          playWav1.play("J.wav");
          while (playWav1.isPlaying()){
            delay(1);  
          } 
        break;   

        case 'k':
          if ((i==0) && (txtMsg.charAt(i+1)=='n')) {  // knock dont make a sound

          } else if (txtMsg.charAt(i-1)=='c') {
            //skip playing a k sound  
          } else {
            //Serial.print("K");
            playWav1.play("K.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          } 
        break;   

        case 'l':
          if (txtMsg.charAt(i-1)=='l') {
            //skip playing a l sound   
          } else {
          //Serial.print("L");
          playWav1.play("L.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          } 
        break;   

        case 'm':
          if (txtMsg.charAt(i+1)=='m')  {
            i++; //next m
          } else   {
              //Serial.print("M");
              playWav1.play("M.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          } 
        break;           

        case 'n':
          if ((txtMsg.charAt(i+1)=='g') && ((txtMsg.charAt(i+2)=='e') || (txtMsg.charAt(i+2)=='i')) && (txtMsg.charAt(i-1)!='i'))  { 
            i++; //skip sound, because it is included in phoneme below
            //Serial.print("N");
            playWav1.play("N.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            } 
            //Serial.print("J");
            playWav1.play("J.wav");
          } else if (txtMsg.charAt(i+1)=='n') { 
            //skip first sound
          } else if (txtMsg.charAt(i+1)=='g') { 
            i++; 
            //Serial.print("NX");
            playWav1.play("NX.wav"); 
          } else { 
            //Serial.print("N");
            playWav1.play("N.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          } 
        break;   

        case 'o':
          if ((txtMsg.charAt(i-1)=='y') && (txtMsg.charAt(i+1)=='u'))  { //
              i++; 
              //Serial.print("UW"); 
              playWav1.play("UW.wav"); 
          } else if ((txtMsg.charAt(i-1)=='v') && (txtMsg.charAt(i+1)=='i'))  { //
              i++;
              //Serial.print("OY"); 
              playWav1.play("OY.wav");  
          } else if ((txtMsg.length() == 4) && (txtMsg.charAt(i-1)=='g'))  { //
              //Serial.print("OH"); 
              playWav1.play("OH.wav");
          } else if (txtMsg.charAt(i+1)=='w')   { //
              i++; 
              //Serial.print("OW"); 
              playWav1.play("OW.wav");
          } else if ((txtMsg.charAt(i-2)=='s') && (txtMsg.charAt(i-1)=='h') && (txtMsg.charAt(i+1)=='e'))  { //tive exception
              i++;
              //Serial.print("UW");
              playWav1.play("UW.wav"); 
          } else if ((txtMsg.charAt(i-1)=='d') && (txtMsg.charAt(i+1)=='o') && (txtMsg.charAt(i+2)=='r'))  { //tive exception
              i++;
              //Serial.print("OH");
              playWav1.play("OH.wav");  
          } else if ((txtMsg.charAt(i-2)=='n') && (txtMsg.charAt(i-1)=='i') && (txtMsg.charAt(i+1)=='n'))  { //tive exception
              i++;
              //Serial.print("UN");
              playWav1.play("UN.wav");     
          } else if ((txtMsg.length() == 4) && (txtMsg.length() -3 == i))  {  //o is the last letter of a two letter word
              //Serial.print("UW"); 
              playWav1.play("UW.wav"); 
          } else if ((txtMsg.charAt(i+1)=='u') && ((txtMsg.charAt(i+2)=='g') && (txtMsg.charAt(i+3)=='h'))) { 
              i=i+3;
              //Serial.print("AA");
              playWav1.play("AA.wav");    
          } else if (txtMsg.charAt(i+1)=='w') {
              i++;
              //Serial.print("AW");
              playWav1.play("AW.wav");          
          } else if ((vowels.indexOf(txtMsg.charAt(i+1)) == -1) && (vowels.indexOf(txtMsg.charAt(i+2)) != -1))  { // if there is a consonant, then a vowel
              //Serial.print("OW");
              playWav1.play("OW.wav");
          } else if (txtMsg.charAt(i+1)=='u') { 
            i++; 
            //Serial.print("OW");
            playWav1.play("OW.wav");
          } else if ((txtMsg.charAt(i-1)=='l') && (txtMsg.charAt(i+1)=='w')){ 
              i++; 
              //Serial.print("OW");
              playWav1.play("OW.wav");
          } else if ((txtMsg.charAt(i-1)=='m') && (txtMsg.charAt(i+1)=='w')){ 
              i++; 
              //Serial.print("OW");
              playWav1.play("OW.wav");
          } else if ((txtMsg.charAt(i-1)=='h') && (txtMsg.charAt(i+1)=='w')){ 
              i++; 
              //Serial.print("AW"); 
              playWav1.play("AW.wav");
          } else if ((txtMsg.charAt(i-1)=='w') && ((txtMsg.charAt(i+1)==' ') || (txtMsg.charAt(i+1)=='\r'))) { 
              //Serial.print("UW"); 
              playWav1.play("UW.wav");   
          } else if (txtMsg.charAt(i+1)=='y'){ 
              i++;
              //Serial.print("OY");
              playWav1.play("OY.wav");
          } else if ((txtMsg.charAt(i-1)=='g') && ((txtMsg.charAt(i+1)==' ') || (txtMsg.charAt(i+1)=='\r'))) { 
              //Serial.print("OH");
              playWav1.play("OH.wav");
          } else if ((txtMsg.charAt(i-1)=='t') && ((txtMsg.charAt(i+1)==' ') || (txtMsg.charAt(i+1)=='\r'))) { 
              //Serial.print("UX");
              playWav1.play("UX.wav");
          } else if ((txtMsg.charAt(i+1)=='o') && (txtMsg.charAt(i+2)=='k')){ 
              i++; 
              //Serial.print("UH");
              playWav1.play("UH.wav");
          } else if (txtMsg.charAt(i+1)=='o') { 
              i++; 
              //Serial.print("UX");
              playWav1.play("UX.wav");
          } else if ((txtMsg.charAt(i+1)=='n') && (txtMsg.charAt(i+2)!='e')){ 
              //Serial.print("AA");
              playWav1.play("AA.wav");
          } else if (txtMsg.charAt(i+1)=='m') { 
              //Serial.print("AH"); 
              playWav1.play("AH.wav");
          } else if ((txtMsg.charAt(i+1)==' ') || (txtMsg.charAt(i+1)=='\r'))  { 
              //Serial.print("OW");
              playWav1.play("OW.wav");
          } else if (txtMsg.charAt(i+1)=='a') {
              i++; 
              //Serial.print("OH");
              playWav1.play("OH.wav");
          } else {
              //Serial.print("AA"); 
              playWav1.play("AA.wav"); 
          }
          while (playWav1.isPlaying()){
            delay(1);  
          } 
        break;

        case 'p':
          if (txtMsg.charAt(i+1)=='h') { 
              i++; 
              //Serial.print("F");
              playWav1.play("F.wav");
          } else if (txtMsg.charAt(i+1)=='p') { 
            //skip first sound 
          } else {
              //Serial.print("P");              
              playWav1.play("P.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          } 
        break;

        case 'q':
          if ((txtMsg.charAt(i+1)=='u') && (txtMsg.charAt(i+2)=='e') && (txtMsg.charAt(i+3)=='a'))  {
            i=i+3;
            //Serial.print("K");
            playWav1.play("K.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            }  
            //Serial.print("W");
            playWav1.play("W.wav");
            while (playWav1.isPlaying()){
              delay(1);  
            }             
            //Serial.print("IY");
            playWav1.play("IY.wav");
          }else {
            //Serial.print("K");
            playWav1.play("K.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          }  
        break; 

        case 'r':
          if (txtMsg.charAt(i+1)=='r') { 
            //skip first r sound
          } else {   
              //Serial.print("R");
              playWav1.play("R.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          } 
        break;  

        case 's':
          if (txtMsg.charAt(i+1)=='h') { 
            i++; 
            //Serial.print("SH");
            playWav1.play("SH.wav");
          } else if ((i > 0) && (vowels.indexOf(txtMsg.charAt(i-1)) != -1) && (vowels.indexOf(txtMsg.charAt(i+1)) != -1))  {  //between two vowels
            //Serial.print("Z");
            playWav1.play("Z.wav");
          } else if (txtMsg.charAt(i+1)=='s') { 
            //skip first sound
          } else if ((i > 0) && (sisz.indexOf(txtMsg.charAt(i-1)) != -1))  { // if prior letter is in s is z sounds
              //Serial.print("Z");
              playWav1.play("Z.wav");
          } else {
              //Serial.print("S");
              playWav1.play("S.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          }  
        break;    

        case 't':
          if ((txtMsg.charAt(i+1)=='i') && (txtMsg.charAt(i+2)=='o') && (txtMsg.charAt(i+3)=='n')){ 
              i=i+3; 
              //Serial.print("SH");
              playWav1.play("SH.wav");
              while (playWav1.isPlaying()){
                delay(1);  
              } 
              //Serial.print("UN");
              playWav1.play("UN.wav");
          } else if (txtMsg.charAt(i+1)=='t')  {
              //skip sound
          } else if ((txtMsg.charAt(i+1)=='i') && ((txtMsg.charAt(i+2)=='o'))) { 
              //Serial.print("SH");
              playWav1.play("SH.wav");
          } else if ((txtMsg.charAt(i+1)=='u') && (txtMsg.charAt(i+2)=='a')) { 
              i++;
              //Serial.print("CH");
              playWav1.play("CH.wav"); 
              while (playWav1.isPlaying()){
                delay(1);  
              } 
              //Serial.print("EW");
              playWav1.play("EW.wav");  

              } else if ((txtMsg.charAt(i+1)=='i') && (txtMsg.charAt(i+2)=='a')) { 
              i++;
              //Serial.print("SH");
              playWav1.play("SH.wav"); 
              while (playWav1.isPlaying()){
                delay(1);  
              } 
              //Serial.print("IY");
              playWav1.play("IY.wav");    
          } else if (txtMsg.charAt(i+1)=='h') { 
              i++; 
              //Serial.print("TH");
              playWav1.play("TH.wav"); 
          } else {
              //Serial.print("T");
              playWav1.play("T.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          } 
        break;   

        case 'u':
          if ((txtMsg.charAt(i-1)=='q') && (txtMsg.charAt(i+1)=='a'))  { 
              i++; 
              //Serial.print("W");
              playWav1.play("W.wav");
              while (playWav1.isPlaying()){
                delay(1);  
              } 
              //Serial.print("AA");
              playWav1.play("AA.wav");
          } else if ((txtMsg.charAt(i-1)=='q') && (txtMsg.charAt(i+1)=='e'))  { 
              i++; 
              //Serial.print("W");
              playWav1.play("W.wav");
              while (playWav1.isPlaying()){
                delay(1);  
              } 
              //Serial.print("EH");
              playWav1.play("EH.wav");
          } else if ((txtMsg.charAt(i-1)=='q') && (txtMsg.charAt(i+1)=='i'))  { 
              i++; 
              //Serial.print("W");
              playWav1.play("W.wav");
              while (playWav1.isPlaying()){
                delay(1);  
              }             
              //Serial.print("IH");
              playWav1.play("IH.wav");  
          } else if ((txtMsg.charAt(i-1)=='q') && (txtMsg.charAt(i+1)=='o'))  { 
              i++; 
              //Serial.print("W");
              playWav1.play("W.wav");
              while (playWav1.isPlaying()){
                delay(1);  
              }            
              //Serial.print("OW");
              playWav1.play("OW.wav"); 
          } else if (((txtMsg.charAt(i-1)=='b') && (txtMsg.charAt(i+1)=='s')) && ((txtMsg.charAt(i+2)=='y') || (txtMsg.charAt(i+2)=='i'))) { //buisiness busy
              i=i+2;
              //Serial.print("IH");
              playWav1.play("IH.wav"); 
              while (playWav1.isPlaying()){
                delay(1);  
              } 
              //Serial.print("Z");
              playWav1.play("Z.wav");  
              while (playWav1.isPlaying()){
                delay(1);  
              } 
              //Serial.print("Y");
              playWav1.play("Y.wav");  
          } else if ((txtMsg.charAt(i+1)=='s') && (vowels.indexOf(txtMsg.charAt(i+2)) != -1)) { //use
              //Serial.print("Y");
              playWav1.play("Y.wav");                 
              
              while (playWav1.isPlaying()){
                delay(1);  
              } 
              //Serial.print("UW");
              playWav1.play("UW.wav");   
          } else if (((txtMsg.charAt(i-1)=='t') || (txtMsg.charAt(i-1)=='b')) && (txtMsg.charAt(i+1)=='r')) { //'bur'n or 'tur'n
              //Serial.print("ER");
              playWav1.play("ER.wav");              
          } else if ((txtMsg.charAt(i+1)=='e') || (txtMsg.charAt(i+1)=='i')){ 
              i++;
              //Serial.print("Y");
              playWav1.play("Y.wav");  
              while (playWav1.isPlaying()){
                delay(1);  
              } 
              //Serial.print("UW");
              playWav1.play("UW.wav");  
          } else {
              //Serial.print("UH");
              playWav1.play("UH.wav");   
          }            
          while (playWav1.isPlaying()){
            delay(1);  
          }           
        break;  

        case 'v':
          //Serial.print("V");
          playWav1.play("V.wav");
          while (playWav1.isPlaying()){
            delay(1);  
          }         
        break;   

        case 'w':
          if (txtMsg.charAt(i+1)=='h') {
            i++;
            //Serial.print("WH");
            playWav1.play("WH.wav");
          } else {
              //Serial.print("W");
              playWav1.play("W.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          }        
        break;

        case 'x':
          if (i==0) {
            //Serial.print("Z");
            playWav1.play("Z.wav");
          } else {
              //Serial.print("K");
              playWav1.play("K.wav");
              while (playWav1.isPlaying()){
                delay(1);  
              }
              //Serial.print("S");
              playWav1.play("S.wav");
          }  
          while (playWav1.isPlaying()){
            delay(1);  
          }        
        break;

        case 'y':
          if (txtMsg.charAt(i-1)=='n') {
            //Serial.print("IY");
            playWav1.play("IY.wav"); 
          } else if ((txtMsg.charAt(i-1)=='m') || (txtMsg.charAt(i-1)=='b'))  { //by or my or bye
              //Serial.print("AY");
              playWav1.play("AY.wav");  
          } else if ((txtMsg.length() < 6) && (txtMsg.length() -3 == i)) {  // if word is 3 char or less long, and ends in y then
              //Serial.print("AY");
              playWav1.play("AY.wav");  
          } else if ((vowels.indexOf(txtMsg.charAt(i-2)) == -1) && (vowels.indexOf(txtMsg.charAt(i-1)) == -1))  { // if there are two consonant prior to, then sound like IY
              //Serial.print("IY");
              playWav1.play("IY.wav"); 
          } else if ((vowels.indexOf(txtMsg.charAt(i-2)) != -1) && (vowels.indexOf(txtMsg.charAt(i-1)) == -1))  { // if there is a vowel, then a consonant {
              //Serial.print("IY"); 
              playWav1.play("IY.wav"); 
          } else {
              //Serial.print("Y");
              playWav1.play("Y.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          }           
        break;

        case 'z':
          if (txtMsg.charAt(i+1)=='z') { 
            //skip first sound
          } else {
          //Serial.print("Z");
          playWav1.play("Z.wav");
          }
          while (playWav1.isPlaying()){
            delay(1);  
          }           
        break; 

        case '.': //period =  short delay between sounds
          delay(200);
        break;   

        case ',': //short added delay
          delay(75);
        break;

        case '\r':  // cariage return
          //Serial.print(" ");
          delay(200);
        break;    

        case ' ':  //space between words
          //Serial.print(" ");
          delay(50);  //delay between words in a sentence
        break;   

        default:
          //Serial.print("Character: ");
          //Serial.print(letter);
          Serial.println(" could not be found!");  
          letter = char(128);          
          delay(100);      
        break;               
      }
      delay(5); //delay between each letter of the word
    }


  //}
  txtMsg="";
  
  //delay between each sound
  while (playWav1.isPlaying()){
    delay(1);  
  } 
delay(50); //between words if they were entered seperately like sayNumber, or by separate say statements
}
