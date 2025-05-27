
//RP2040 Synchronous On Off Keying Tone Generator

#include <stdio.h>
#include "hardware/pwm.h"
#include "pico/stdlib.h"

#define MESSAGE "OOK48 TEST"

#define LINK1PIN 10
#define LINK2PIN 11
#define LINK3PIN 12
#define LINK4PIN 13

#define TONEOUTPIN 2
#define NOISEOUTPIN 3
#define KEYOUTPIN 6
#define KEYINPIN 7
#define PPSOUTPUT 8


#define TWOP32 4294967296                //2^32 Accumulator Length
#define DDSFREQ 250000.00                 //DDS update frequency
 
#define OOK_DELAY 111111                 //9 symbols per second (only the first 8 are used)

int8_t sineTable[256];

uint8_t ookBuffer[30];
char ookMessage[34];              //allow for adding sync chars if needed

uint32_t DDSAcc;                     //32 bit accumulator
uint32_t DDSInc;                     //incremental value

uint tonegpioslice;
uint tonegpiochan;
uint noisegpioslice;
uint noisegpiochan;

bool ookKey;
bool ookActive;
int ookTime;

uint8_t ookMessLen;

int seconds =0;
int milliseconds =0;

struct repeating_timer DDStimer;
struct repeating_timer symbolTimer;

uint8_t mode;
enum modetype{GEN,KEY};

//Interrupt called every 4us (250 KHz) to update the DDS and PWM outputs. 
bool DDS_interrupt(struct repeating_timer *t)
{
  uint8_t newval;
  int8_t noise;

  DDSAcc = DDSAcc + DDSInc;
  newval = DDSAcc >> 24;
  noise = random(256) - 128;
  if(ookKey)
  {
    pwm_set_chan_level(tonegpioslice, tonegpiochan, sineTable[newval] + 128);  
  }
  else
  {
    pwm_set_chan_level(tonegpioslice, tonegpiochan, 128);  
  }
  pwm_set_chan_level(noisegpioslice, noisegpiochan, noise + 128);
  return true;
}

//Interrupt called every symbol time to update the symbol tone. 
bool symbol_interrupt(struct repeating_timer *t)
{
  ookTick();
  return true;
}

void setup() 
{
  Serial.begin();
  analogWriteFreq(200000);                 //PWM frequency is 200KHz
  analogWriteRange(256);                  //8 bit PWM range
  analogWrite(TONEOUTPIN,128);                //initially set 50%
  analogWrite(NOISEOUTPIN, 128);


  pinMode(LINK1PIN,INPUT_PULLUP);
  pinMode(LINK2PIN,INPUT_PULLUP);
  pinMode(LINK3PIN,INPUT_PULLUP);
  pinMode(LINK4PIN,INPUT_PULLUP);

  if(digitalRead(LINK1PIN) == 0)
  {
    mode = KEY;
  }
  else 
  {
    mode = GEN;
  }

    pinMode(KEYOUTPIN,OUTPUT);
    digitalWrite(KEYOUTPIN,0);
    pinMode(PPSOUTPUT,OUTPUT);
    digitalWrite(PPSOUTPUT,0);
    pinMode(KEYINPIN,INPUT_PULLUP);   

  for(int i = 0; i < 256; i++)
    {
      sineTable[i] = (sin((double) i * 1.40625 * 0.017453) * 64);
    }

  DDSAcc = 0;
  
  strcpy(ookMessage , MESSAGE);
  ookMessLen = strlen(ookMessage);


  //preset the parameters fro the PWM update here, to save time in the interrupt routine. 

  tonegpioslice = pwm_gpio_to_slice_num(TONEOUTPIN);
  tonegpiochan = pwm_gpio_to_channel(TONEOUTPIN);
  noisegpioslice = pwm_gpio_to_slice_num(NOISEOUTPIN);
  noisegpiochan = pwm_gpio_to_channel(NOISEOUTPIN);
  
    ookInit();
    ookActive = false;
    ookTime = 1;
    seconds = -1;
    DDSInc = getInc(800);             //DDS increment for 1 KHz 



// start a repeating timer interrupt every 4 us (250 KHz) for the DDS update
 add_repeating_timer_us(-4,DDS_interrupt,NULL,&DDStimer);

// start a repeating timer interrupt every for the Symbol Rate
 add_repeating_timer_us(-OOK_DELAY,symbol_interrupt,NULL,&symbolTimer);
}


//Core 0 loop. Main timing loop. 
void loop()
{
  static unsigned long loopTimer = micros() + 1000;

  while(loopTimer == millis());          //hang waiting for the next 1mS timeslot
  loopTimer = millis();

  

     milliseconds++;
     if(milliseconds == 1000)
      {
        digitalWrite(PPSOUTPUT,1);                  //generate the 1PPS pulse
        delayMicroseconds(1);
        digitalWrite(PPSOUTPUT,0);
        ookActive = true;
        seconds++;
        milliseconds = 0;
        if(seconds == 60)
          {
            seconds = 0;
          }
      }

  if(mode == KEY)
   {
    if(digitalRead(KEYINPIN))
    {
      ookKey = 0;
    }
    else 
    {
    ookKey = 1;
    }
   }


   if(ookKey ==1)
    {
      digitalWrite(KEYOUTPIN,1);
    }
  else
    {
      digitalWrite(KEYOUTPIN,0);
    }

}

//returns the DDS accumulator increment requred for the given frequency
uint32_t getInc(double freq)
{
  return TWOP32/(DDSFREQ / freq);
}


