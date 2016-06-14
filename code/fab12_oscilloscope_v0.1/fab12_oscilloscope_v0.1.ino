
#include "U8glib.h"

#define SIGNAL_FRAME_H 64
#define SIGNAL_FRAME_W 96
#define PROBE_PIN_1 A0

int probeRate;
int probe1Value;
uint8_t probedValues[SIGNAL_FRAME_W-2];
int probedValuesBuffer[SIGNAL_FRAME_W-2];
bool signalFrameBuffer[SIGNAL_FRAME_H-2][SIGNAL_FRAME_W-2];
uint8_t C,Z,probedValuesCounter;

U8GLIB_ST7920_128X64 u8g(13, 11, 12, U8G_PIN_NONE);

void setup(void) {

  //debug init
  Serial.begin(57600);

  //analog input init
  pinMode(PROBE_PIN_1,INPUT);

  //other settings & init
  probeRate = 20;
  probedValuesCounter = 0;

  Z=0;
  for(;Z<94;Z++){
    probedValues[Z]=0;
    probedValuesBuffer[Z]=0;
  }
    
  //display init
  u8g.setColorIndex(1);

  Serial.println("init ok");
  
}

void loop(void) {

  //do a complete probe and then refresh the screen
  //this is needed because the resfresh rate of the display is to slow

  Z=0;

  for(;Z<SIGNAL_FRAME_W;Z++){

      probe1Value = analogRead(PROBE_PIN_1);
      probe1Value = map(probe1Value,0,1023,3,61);
      probedValues[Z] = probe1Value;
      delay(probeRate);
      
      //++probedValuesCounter;
  
    //if(probedValuesCounter == SIGNAL_FRAME_W)
      //probedValuesCounter = 0;
  
  //Serial.println(probe1Value);  
      
  }
    
  u8g.firstPage();

  do{
  
    draw();

  }while( u8g.nextPage() );

  
  
}

void draw(void) {

  //setup the signal display
  u8g.drawFrame(0,0,SIGNAL_FRAME_W,SIGNAL_FRAME_H);
  u8g.drawLine(0,(SIGNAL_FRAME_H/2)-1,SIGNAL_FRAME_W-1,(SIGNAL_FRAME_H/2)-1);

  u8g.setFont(u8g_font_6x13B);
  u8g.drawStr(SIGNAL_FRAME_W+1, 9, "FAB12");
  u8g.drawStr(SIGNAL_FRAME_W+1, 19, "SCOPE");

  //draw signal data
  Z=0;
  
  for(;Z<SIGNAL_FRAME_W-2;Z++)
      
      if(probedValues[Z] > 0){

         if(Z>0){

          if(probedValues[Z] - probedValues[Z-1] > 1 || probedValues[Z] - probedValues[Z-1] < -1)
            u8g.drawLine(Z+2,probedValues[Z-1],Z+2,probedValues[Z]);
          else 
            u8g.drawPixel(Z+2,probedValues[Z]);
          
         }else
          u8g.drawPixel(Z+2,probedValues[Z]);
        
      }
       
}
