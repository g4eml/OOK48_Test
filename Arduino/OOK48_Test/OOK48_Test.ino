
//RP2040 Synchronous On Off Keying Tone Generator

#include <stdio.h>
#include "hardware/pwm.h"
#include "pico/stdlib.h"

#define MESSAGE "OOK48 TEST"

#define LINK1PIN 10           //Selects Tone Keyer Mode for testing an externally generated key signal
#define LINK2PIN 11           //Selects 2 Second Encoding.
#define LINK3PIN 12
#define LINK4PIN 13

#define TONEOUTPIN 2
#define NOISEOUTPIN 3
#define KEYOUTPIN 6
#define KEYINPIN 7
#define PPSOUTPUT 8

#define GPSRXPin 5
#define GPSTXPin 4

#define GPSBAUD 9600

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

bool halfRate = false;

uint8_t ookMessLen;

int milliseconds =0;
int seconds =0;
int minutes =0;
int hours =10;

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


  //preset the parameters for the PWM update here, to save time in the interrupt routine. 

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


//Core 0 loop. Main encoding loop. 
void loop()
{
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

  if(digitalRead(LINK2PIN) == 0)
  {
    halfRate = true;
  }
  else 
  {
    halfRate = false;
  }

}

//returns the DDS accumulator increment requred for the given frequency
uint32_t getInc(double freq)
{
  return TWOP32/(DDSFREQ / freq);
}

//Core 1 handles the Clock and GPS simulation
void setup1()
{
  Serial2.setRX(GPSRXPin);              //Configure the GPIO pins for the GPS module
  Serial2.setTX(GPSTXPin);
  Serial2.begin(GPSBAUD);                        
}

void loop1()
{
  static unsigned long loopTimer = millis() + 100;
  while(loopTimer >  millis());          //hang waiting for the next 100mS timeslot
  loopTimer = millis() + 100;

     milliseconds = milliseconds + 100;
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
            minutes++;
            if(minutes ==60)
             {
              hours++;
               if(hours == 24)
                {
                  hours = 0;
                }
             }
          }
      }

      if(milliseconds == 100)
       {
        sendNMEA();
       }
}

void sendNMEA(void)
{
  char nmea[100];
  sprintf(nmea,"$GPRMC,%02d%02d%02d,A,5120.000,N,00030.000,E,000.0,000.0,010425,000.0,E*00\n\r",hours,minutes,seconds);
  Serial2.write(nmea);
}