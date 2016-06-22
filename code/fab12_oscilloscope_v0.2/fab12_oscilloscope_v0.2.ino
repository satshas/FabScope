#include "U8glib.h"

#define SIGNAL_FRAME_H 64
#define SIGNAL_FRAME_W 96
#define PROBE_PIN A0
#define SAMPLING_RATE_PIN A5
#define BUTTON_VOLTAGE_RANGE 2

#define RELAY_K1_PIN 13
#define RELAY_K2_PIN 14
#define RELAY_K3_PIN 15
#define RELAY_K4_PIN 16
#define RELAY_K5_PIN 17
#define RELAY_K6_PIN 18

#define MINIMUM_DISPLAY_SIGNAL_HEIGHT 3
#define MAXIMUM_DISPLAY_SIGNAL_HEIGHT 61

#define MINIMUM_ANALOG_READ 0
#define MAXIMUM_ANALOG_READ 1023

#define BUTTON_RANGE_TIME_PRESS_AGAIN 1000

//#define SAMPLING_RATE_MAX_MICROSECONDS 100000
#define SAMPLING_RATE_MAX_MICROSECONDS 10000
#define SAMPLING_RATE_MIN_MICROSECONDS 100

const bool DEBUG  = true;

long probeRate;
int probe1Value;
uint8_t probedValues[SIGNAL_FRAME_W-2];
int probedValuesBuffer[SIGNAL_FRAME_W-2];
bool signalFrameBuffer[SIGNAL_FRAME_H-2][SIGNAL_FRAME_W-2];
uint8_t C,Z,probedValuesCounter;
char freqPrint[7];
char voltageRangePrint[6];
uint8_t currentRange;
bool buttonRangePressed;
long buttonRangeTimePressed1,buttonRangeTimePressed2;
float samplingFrequency;

U8GLIB_ST7920_128X64 u8g(23, 21, 22, U8G_PIN_NONE);

void setup(void) {

  //debug init
   if(DEBUG)
    Serial.begin(9600);

  //init voltageRangePrint
  voltageRangePrint[0] = ' ';
  voltageRangePrint[1] = ' ';
  voltageRangePrint[2] = ' ';
  voltageRangePrint[3] = ' ';
  voltageRangePrint[4] = ' ';
  voltageRangePrint[5] = ' ';

  //init freqPrint
  freqPrint[0] = ' ';
  freqPrint[1] = ' ';
  freqPrint[2] = ' ';
  freqPrint[3] = ' ';
  freqPrint[4] = ' ';
  freqPrint[5] = ' ';
  freqPrint[6] = ' ';

  //analog input init
  pinMode(PROBE_PIN,INPUT);
  pinMode(SAMPLING_RATE_PIN,INPUT);

  //digital relay pins init
  pinMode(RELAY_K1_PIN,OUTPUT);
  pinMode(RELAY_K2_PIN,OUTPUT);
  pinMode(RELAY_K3_PIN,OUTPUT);
  pinMode(RELAY_K4_PIN,OUTPUT);
  pinMode(RELAY_K5_PIN,OUTPUT);
  pinMode(RELAY_K6_PIN,OUTPUT);

  //init time vars
  buttonRangeTimePressed1 = 0;
  buttonRangeTimePressed2 = 0;

  //digital button pin input
  pinMode(BUTTON_VOLTAGE_RANGE,INPUT);
  buttonRangePressed = LOW;
  
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

  //default position at maximum voltage
  switch_250();

  //attach interrupts for changing the voltage range
  attachInterrupt(digitalPinToInterrupt(BUTTON_VOLTAGE_RANGE), interruptFunction, HIGH);
  
  if(DEBUG)
    Serial.println("init ok");
  
}

void loop(void) {

  //do a complete probe and then refresh the screen
  //this is needed because the resfresh rate of the display is to slow
  
  probeRate = analogRead(SAMPLING_RATE_PIN);
  probeRate = map(probeRate,MINIMUM_ANALOG_READ,MAXIMUM_ANALOG_READ,SAMPLING_RATE_MIN_MICROSECONDS,SAMPLING_RATE_MAX_MICROSECONDS);
  samplingFrequency = probeRate;
  samplingFrequency = 1/(samplingFrequency*0.000001);
  
  if(DEBUG)
    Serial.println(samplingFrequency);

  Z=0;

  for(;Z<SIGNAL_FRAME_W;Z++){

      buttonRangePressed = digitalRead(BUTTON_VOLTAGE_RANGE);
      
      probe1Value = analogRead(PROBE_PIN);
      probe1Value = map(probe1Value,MINIMUM_ANALOG_READ,MAXIMUM_ANALOG_READ,MINIMUM_DISPLAY_SIGNAL_HEIGHT,MAXIMUM_DISPLAY_SIGNAL_HEIGHT);
      probedValues[Z] = probe1Value;
      
      if(probeRate<1000 && probeRate>=SAMPLING_RATE_MIN_MICROSECONDS)
        delayMicroseconds(probeRate);
      else
      //if(probeRate < 100){}
      //else
        delay(probeRate/1000);
            
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

  //draw selected voltage range
  u8g.setFont(u8g_font_5x7);
  u8g.drawStr(SIGNAL_FRAME_W+1, 56, voltageRangePrint);

  //draw fequency
  u8g.setFont(u8g_font_5x7);
  dtostrf(samplingFrequency,5,0,freqPrint);

  if(samplingFrequency == SAMPLING_RATE_MAX_MICROSECONDS){

      freqPrint[0] = '1';
      freqPrint[1] = '0';
      freqPrint[2] = 'K';
      freqPrint[3] = 'H';
      freqPrint[4] = 'Z';
      freqPrint[5] = ' ';
      freqPrint[6] = ' ';
    
  }else
  if(samplingFrequency >= 1000 && samplingFrequency < SAMPLING_RATE_MAX_MICROSECONDS){
    
      freqPrint[3] = freqPrint[2];
      freqPrint[2] = '.';
      freqPrint[4] = 'K';
      freqPrint[5] = 'H';
      freqPrint[6] = 'Z';
  
  }else{
    
    //write hz at the end
    freqPrint[5] = 'H';
    freqPrint[6] = 'Z';
    
    } 

  //shift left the chars if there is space at the beginning
  while(freqPrint[0] == ' '){

      freqPrint[0] = freqPrint[1];
      freqPrint[1] = freqPrint[2];
      freqPrint[2] = freqPrint[3];
      freqPrint[3] = freqPrint[4];
      freqPrint[4] = freqPrint[5];
      freqPrint[5] = freqPrint[6];
      freqPrint[6] = ' ';
    
  }
  
  u8g.drawStr(SIGNAL_FRAME_W+1, 63, freqPrint);

  //draw signal data
  Z=0;
  
  for(;Z<SIGNAL_FRAME_W-2;Z++)
      
      if(probedValues[Z] > 0){

         if(Z>0){

          if(probedValues[Z] - probedValues[Z-1] > 1 || probedValues[Z] - probedValues[Z-1] < -1)
            u8g.drawLine(Z+1,(MAXIMUM_DISPLAY_SIGNAL_HEIGHT+2)-probedValues[Z-1],Z+1,(MAXIMUM_DISPLAY_SIGNAL_HEIGHT+2)-probedValues[Z]);
          else 
            u8g.drawPixel(Z+1,(MAXIMUM_DISPLAY_SIGNAL_HEIGHT+2)-probedValues[Z]);
          
         }else
          u8g.drawPixel(Z+1,(MAXIMUM_DISPLAY_SIGNAL_HEIGHT+2)-probedValues[Z]);
        
   }
   
}

void switch_50(){
  
  digitalWrite(RELAY_K1_PIN,LOW);
  digitalWrite(RELAY_K2_PIN,LOW);
  digitalWrite(RELAY_K3_PIN,HIGH);
  digitalWrite(RELAY_K4_PIN,LOW);
  digitalWrite(RELAY_K5_PIN,LOW);
  digitalWrite(RELAY_K6_PIN,HIGH);

  voltageRangePrint[0] = '0';
  voltageRangePrint[1] = '-';
  voltageRangePrint[2] = '5';
  voltageRangePrint[3] = '0';
  voltageRangePrint[4] = 'V';
  voltageRangePrint[5] = ' ';

  currentRange = 50;
  
}

void switch_5(){

  digitalWrite(RELAY_K1_PIN,LOW);
  digitalWrite(RELAY_K2_PIN,LOW);
  digitalWrite(RELAY_K3_PIN,LOW);
  digitalWrite(RELAY_K4_PIN,LOW);
  digitalWrite(RELAY_K5_PIN,LOW);
  digitalWrite(RELAY_K6_PIN,LOW);

  voltageRangePrint[0] = '0';
  voltageRangePrint[1] = '-';
  voltageRangePrint[2] = '5';
  voltageRangePrint[3] = 'V';  
  voltageRangePrint[4] = ' ';  
  voltageRangePrint[5] = ' ';

  currentRange = 5;
  
}

void switch_120(){

  digitalWrite(RELAY_K1_PIN,HIGH);
  digitalWrite(RELAY_K2_PIN,LOW);
  digitalWrite(RELAY_K3_PIN,LOW);
  digitalWrite(RELAY_K4_PIN,HIGH);
  digitalWrite(RELAY_K5_PIN,LOW);
  digitalWrite(RELAY_K6_PIN,LOW);

  voltageRangePrint[0] = '0';
  voltageRangePrint[1] = '-';
  voltageRangePrint[2] = '1';
  voltageRangePrint[3] = '2';
  voltageRangePrint[4] = '0';
  voltageRangePrint[5] = 'V';

  currentRange = 120;
  
}

void switch_250(){

  digitalWrite(RELAY_K1_PIN,LOW);
  digitalWrite(RELAY_K2_PIN,HIGH);
  digitalWrite(RELAY_K3_PIN,LOW);
  digitalWrite(RELAY_K4_PIN,LOW);
  digitalWrite(RELAY_K5_PIN,HIGH);
  digitalWrite(RELAY_K6_PIN,LOW);
 
  voltageRangePrint[0] = '0';
  voltageRangePrint[1] = '-';
  voltageRangePrint[2] = '2';
  voltageRangePrint[3] = '5';
  voltageRangePrint[4] = '0';
  voltageRangePrint[5] = 'V';

  currentRange = 250;
  
}

void interruptFunction(){

  buttonRangeTimePressed1 = millis();

  if((buttonRangeTimePressed1-buttonRangeTimePressed2) > BUTTON_RANGE_TIME_PRESS_AGAIN){
    
    if(currentRange == 5)
      switch_50();
    else
    if(currentRange == 50)
      switch_120();
    else
    if(currentRange == 120)
      switch_250();
     else
    if(currentRange == 250)
      switch_5(); 

    buttonRangeTimePressed2 = millis();
 
  }
 
}

