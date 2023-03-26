# TeensyTalk
This sketch does Text to Speech (TTS) conversion using a 51 phoneme short wav files to create words on the fly, based on English pronounciation rules. Not all words are pronounced correctly, though many are. Some of the errors are due to the English speech often not following rules. This uses Frank Boesing's Teensy-WavePlayer https://github.com/FrankBoesing/Teensy-WavePlayer, which is capable of playing 22050 hertz audio files. 22050 hertz sound files happen to be what espeak-ng TTS software uses. It works with Teensy 3.2, (probably 3.5), 3.6, 4.0, and 4.1 using the appropriate version of the audio sound shield. All sound files must be placed on a readable micro SD card, in the base director. Speech will play on the headphone jack. 

The speech played out is rather robotic. It is basically a modern version of a S.A.M. (Speech Automated Mouth) chip. Like S.A.M., this project used the Arpabet https://en.wikipedia.org/wiki/ARPABET to name all the phoneme files. The audio wav files for this project were generated from espeak-ng TTS. 

The pronounciation of words is best when single words are used with say(), like:
say("do") , but several words can also be put inside the say() function. However, some of the pronunciation rules do not work correctly when there are multiple words used in the same say() function. Right now, only lowercase letters, and 0,1,2,3,4,5,6,7,8,9 are accepted (with the exception of "I"). There is also a sayNumber() function that can say variables, and for a number like 2435 will say "two thousdand four hundred and thirty five".

The 51 sound files are very small and only take up a total of about 350Kb. The sketch uses about 27% of flash memory on the (smallest) Teensy 3.2, and about 13% of its RAM.  

Type_in_words lets you test the output of different words you type in directly. It is helpful for debuging pronounciation problems, with help from the serial output of the phonemes used.
