
//RP2040 Synchronous On Off Keying Tone Generator

#include <stdio.h>
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include <EEPROM.h>

#define DEFAULTMESSAGE "OOK48 TEST"

#define LINK1PIN 10           //Selects Tone Keyer Mode for testing an externally generated key signal
#define LINK2PIN 11           //Selects 2 Second Encoding.
#define LINK3PIN 12
#define LINK4PIN 13           //Selects real GPS module is fitted. Stand alone operation

#define TONEOUTPIN 2
#define NOISEOUTPIN 3
#define KEYOUTPIN 6
#define KEYINPIN 7
#define PPSPIN 8

#define GPSRXPin 5
#define GPSTXPin 4

#define GPSBAUD 9600

#define TWOP32 4294967296                //2^32 Accumulator Length
#define DDSFREQ 250000.00                 //DDS update frequency
 
#define OOK_DELAY 111111                 //9 symbols per second (only the first 8 are used)

int8_t sineTable[256];

uint8_t ookBuffer[34];
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
bool realGPS = false;

uint8_t ookMessLen;

int milliseconds =0;
int seconds =0;
int minutes =0;
int hours =10;

struct repeating_timer DDStimer;
struct repeating_timer symbolTimer;

uint8_t mode;
enum modetype{GEN,KEY};

char gpsCh;
char gpsBuffer[256];                     //GPS data buffer
int gpsPointer;                          //GPS buffer pointer

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

//interrupt routine for 1 Pulse per second input
void ppsISR(void)
{
 cancel_repeating_timer(&symbolTimer);                           //Stop the symbol timer if it is running. 
 add_repeating_timer_us(-OOK_DELAY,symbol_interrupt,NULL,&symbolTimer); //restart the symbol timer
 ookActive = true;
 ookTick();
}

void setup() 
{
  Serial.begin();
  EEPROM.begin(256);
  getMessage();
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

    if(digitalRead(LINK4PIN) == 0)
  {
    realGPS = true;
  }
  else 
  {
    realGPS = false;
  }



    pinMode(KEYOUTPIN,OUTPUT);
    digitalWrite(KEYOUTPIN,0);
    pinMode(KEYINPIN,INPUT_PULLUP); 

    if(realGPS)
    {
      pinMode(PPSPIN,INPUT);
    }
    else 
    {
      pinMode(PPSPIN,OUTPUT);
      digitalWrite(PPSPIN,0); 
    }
 

  for(int i = 0; i < 256; i++)
    {
      sineTable[i] = (sin((double) i * 1.40625 * 0.017453) * 64);
    }

  DDSAcc = 0;




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

 if(realGPS)
   {
     attachInterrupt(PPSPIN,ppsISR,RISING);
   }
}


//Core 0 loop. Main encoding loop. 
void loop()
{
  char sc;
  int chCount;
  bool done;

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
  
  if(Serial.available() > 0)
   {
    while(Serial.available() > 0)         //flush out any input buffer.
     {
      sc = Serial.read();
     }
    Serial.print("\n\nCurrent Message = ");
    Serial.print(ookMessage);
    Serial.print("\nEnter new message (up to 30 characters) -->");
    chCount =0;
    sc = 0;
    done = false;
    while((!done) & (chCount < 30))
     {
      while(Serial.available() == 0);
      sc = Serial.read();
      if((sc == 13)||(sc == 10))
       {
        done = true;
       }
      else
       {
        ookMessage[chCount++]= toupper(sc);
        Serial.write(sc);
       }
     }
    if(chCount > 0) ookMessage[chCount] = 0;
    Serial.print("\nNew Message = ");
    Serial.println(ookMessage);
    ookMessLen = strlen(ookMessage);
    saveMessage();
    Serial.println("New Message Saved");
    ookInit();
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
  gpsPointer = 0;                       
}

void loop1()
{
 static unsigned long loopTimer = millis() + 100;
 if(realGPS)
  {
      if(Serial2.available() > 0)           //data received from GPS module
      {
        while(Serial2.available() >0)
          {
            gpsCh=Serial2.read();
            if(gpsCh > 31) gpsBuffer[gpsPointer++] = gpsCh;
            if((gpsCh == 13) || (gpsPointer > 255))
              {
                gpsBuffer[gpsPointer] = 0;
                processNMEA();
                gpsPointer = 0;
              }
          }

      }
  }

  else                  //simulate the GPS receiver
   {
    while(loopTimer >  millis());          //hang waiting for the next 100mS timeslot
    loopTimer = millis() + 100;

     milliseconds = milliseconds + 100;
     if(milliseconds == 1000)
      {
        digitalWrite(PPSPIN,1);                  //generate the 1PPS pulse
        delayMicroseconds(1);
        digitalWrite(PPSPIN,0);
        ppsISR();                                 //simulate an interrupt
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
}

void sendNMEA(void)
{
  char nmea[100];
  sprintf(nmea,"$GPRMC,%02d%02d%02d,A,5120.000,N,00030.000,E,000.0,000.0,010425,000.0,E",hours,minutes,seconds);
  addChecksum(nmea,100);
  Serial2.write(nmea);
}

void addChecksum(char *sentence, size_t max_len) {
    if (!sentence || sentence[0] != '$') {
        return;
    }

    uint8_t checksum = 0;
    for (size_t i = 1; sentence[i] != '\0' && sentence[i] != '*'; i++) {
        checksum ^= (uint8_t)sentence[i];
    }

    size_t len = strlen(sentence);

    if (len + 5 < max_len) {
        snprintf(sentence + len, max_len - len, "*%02X\r\n", checksum);
    }
}

void processNMEA(void)
{
  float gpsTime;

 if(strstr(gpsBuffer , "RMC") != NULL)                         //is this the RMC sentence?
  {
    if(strstr(gpsBuffer , "A") != NULL)                       // is the data valid?
      {
       int p=strcspn(gpsBuffer , ",");                         // find the first comma
       gpsTime = strtof(gpsBuffer+p+1 , NULL);                 //copy the time to a floating point number
       seconds = int(gpsTime) % 100;
       gpsTime = gpsTime / 100;
       minutes = int(gpsTime) % 100; 
       gpsTime = gpsTime / 100;
       hours = int(gpsTime) % 100;         
      }
    else
     {
       seconds = -1;                                            //GPS time not valid
       minutes = -1;
       hours = -1;
     }
  }
}

void getMessage(void)
{
   if(EEPROM.read(0)== 0x73)
    {
     EEPROM.get(1,ookMessage);
    }
   else
    {
    strcpy(ookMessage,DEFAULTMESSAGE);
    saveMessage();
    }
    
    ookMessLen = strlen(ookMessage);
}

void saveMessage(void)
 {
    EEPROM.put(1,ookMessage);
    EEPROM.write(0,0x73);
    EEPROM.commit();
 }