/* 
 * Sends a synchronous on off keying encoded message.
 *
*/ 


int ookPointer = 0;
uint8_t ookBitPointer = 0;

//all valid 4 from 8 values. 
uint8_t encode[70] = {15,23,27,29,30,39,43,45,46,51,
                      53,54,57,58,60,71,75,77,78,83,
                      85,86,89,90,92,99,101,102,105,106,
                      108,113,114,116,120,135,139,141,142,147,
                      149,150,153,154,156,163,165,166,169,170,
                      172,177,178,180,184,195,197,198,201,202,
                      204,209,210,212,216,225,226,228,232,240};


void ookInit(void)
{
  ookMessLen = ook_encode(ookMessage , ookMessLen, ookBuffer);
  ookPointer = 0;
  ookBitPointer = 0;
}

void ookTick(void)
{
  if((ookActive)&(mode == GEN))
  {
    if(ookPointer == ookMessLen) 
      {
        ookPointer =  0;
        ookBitPointer = 0;
      }
      if(ookBitPointer == 8)
        {
          ookKey = 0;
          ookActive = false;              //wait for the next 1PPS
        }
      else
        {
          ookKey = (ookBuffer[ookPointer] << ookBitPointer) & 0x80 ;       
        }
      ookBitPointer++;
      if(ookBitPointer > 8)
        {
          ookBitPointer = 0;
          if((halfRate == false ) || (halfRate & (seconds & 0x01) )) ookPointer++; 
        }
  }
}


//

uint8_t ook_encode(const char * msg, uint8_t len, uint8_t * symbols)
{
  uint8_t v;
  for(int i = 0;i<len;i++)
   {
    v=69;                            //default to null 
    switch(msg[i])
      {
         case 32 ... 95:
          v= msg[i] - 31;           //all upper case Letters, numbers and punctuation encoded to 1 - 64
          break;
         
         case 97 ... 122:
           v= msg[i] - 63;          //lower case letters map to upper case. 
      }
    symbols[i] = encode[v];
   } 

   symbols[len] = encode[0];               //add end of message code
   return len+1;
}



