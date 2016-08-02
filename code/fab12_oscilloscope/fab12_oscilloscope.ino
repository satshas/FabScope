#include "U8glib.h"

#define SIGNAL_FRAME_H 64
#define SIGNAL_FRAME_W 96
#define SIGNAL_FRAME_PROBED 94
#define PROBE_PIN A0
#define SAMPLING_RATE_PIN A5
#define BUTTON_VOLTAGE_RANGE 2
#define BUTTON_FIT 1

#define RELAY_K1_PIN 13
#define RELAY_K2_PIN 14
#define RELAY_K3_PIN 15
#define RELAY_K4_PIN 16
#define RELAY_K5_PIN 17
#define RELAY_K6_PIN 18

#define MINIMUM_DISPLAY_SIGNAL_HEIGHT 3
#define MAXIMUM_DISPLAY_SIGNAL_HEIGHT 61

uint16_t minimumAnalogRead;
uint16_t maximumAnalogRead;

#define MINIMUM_ANALOG_READ 0
#define MAXIMUM_ANALOG_READ 1023

#define BUTTON_RANGE_TIME_PRESS_AGAIN 1000

//#define SAMPLING_RATE_MAX_MICROSECONDS 100000
#define SAMPLING_RATE_MAX_MICROSECONDS 10000
#define SAMPLING_RATE_MIN_MICROSECONDS 100

const bool DEBUG  = true;

char freqPrint[9];
char voltagePrint[8];
char voltageRangePrint[9];
bool fitFunction;
long probeRate;
float probeRateMillis;
uint16_t probe1Valuea, maxVoltageProbe, minVoltageProbe;
long avgVoltageProbe;
uint16_t probedValues[SIGNAL_FRAME_PROBED];
uint8_t C,Z,probedValuesCounter;
uint8_t currentRange;
long buttonRangeTimePressed1,buttonRangeTimePressed2;
double samplingFrequency;
double maxVoltage,minVoltage,avgVoltage;
double currentVoltageRatio;
const double voltageRatio5 = 0.00497512437811; //volts per unit
const double voltageRatio50 = 0.05681818181818;//volts per unit
const double voltageRatio120 = 0.15151515151515;//volts per unit
const double voltageRatio200 = 0.38461538461538;//volts per unit

U8GLIB_ST7920_128X64 u8g(23, 21, 22, U8G_PIN_NONE);

void setup(void) {
  
  //debug init
   if(DEBUG)
    Serial.begin(9600);

  fitFunction = false;
  minimumAnalogRead = 0;
  maximumAnalogRead = 1023;
    
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

  //digital button pin inputs
  pinMode(BUTTON_VOLTAGE_RANGE,INPUT);
  pinMode(BUTTON_FIT,INPUT);
 
  //other settings & init
  probeRate = 20;
  probedValuesCounter = 0;
   
  //display init
  u8g.setColorIndex(1);

  //default position at maximum voltage
  switch_200();

  //attach interrupts for changing the voltage range
  attachInterrupt(digitalPinToInterrupt(BUTTON_VOLTAGE_RANGE), interruptFunctionRange, HIGH);

 
  if(DEBUG)
    Serial.println("init ok");
 
}

void loop(void) {

  if(digitalRead(BUTTON_FIT)){

    if(DEBUG)
      Serial.println("buttonFit");

      if(fitFunction)
        fitFunction = false;
    else
      fitFunction = true;
    
  }
  
  //do a complete probe and then refresh the screen
  //this is needed because the resfresh rate of the display is to slow

  //first calculate the probe rate according to the position of the potentiometer
  probeRate = analogRead(SAMPLING_RATE_PIN);
  probeRate = map(probeRate,MINIMUM_ANALOG_READ,MAXIMUM_ANALOG_READ,SAMPLING_RATE_MIN_MICROSECONDS,SAMPLING_RATE_MAX_MICROSECONDS);
  samplingFrequency = probeRate;
  probeRateMillis = probeRate/1000;
  samplingFrequency = 1/(samplingFrequency*0.000001);
 
  if(DEBUG)
    Serial.println(samplingFrequency);

  //now start to fill the data buffer with samples
  //reset the voltage statistics
  maxVoltageProbe = 0;
  minVoltageProbe=1500;
  avgVoltageProbe = 0;
  
  Z=0;
  
  while(true){

      //place this inside an interrupt or outside the for
      //of just simplyfy the loop and consider the time of the analog read
      probedValues[Z] = analogRead(PROBE_PIN);
        
      //wait a certain amount of time according to the sampling rate
      if(probeRate<1000 && probeRate>=SAMPLING_RATE_MIN_MICROSECONDS)
        delayMicroseconds(probeRate-100);//consider the analog read
      else
        delay(probeRateMillis);//here not consider the analogRead delay

        ++Z;
        if(Z==SIGNAL_FRAME_PROBED)
          break;
           
  }

  //read only the voltage statistics
  Z=0;
  
  for(;Z<SIGNAL_FRAME_PROBED;Z++){
            
      //do statistics about the voltage
      
      if(probedValues[Z] > maxVoltageProbe)
        maxVoltageProbe = probedValues[Z];
      
      if( probedValues[Z] < minVoltageProbe)
        minVoltageProbe = probedValues[Z];

      avgVoltageProbe += probedValues[Z];
          
  }

  //check the fit function
  if(fitFunction){

    minimumAnalogRead = minVoltageProbe;
    maximumAnalogRead = maxVoltageProbe;
    
  }else{

   minimumAnalogRead = MINIMUM_ANALOG_READ;
   maximumAnalogRead = MAXIMUM_ANALOG_READ;
    
  }

  Z=0;
  
  for(;Z<SIGNAL_FRAME_PROBED;Z++)          
      probedValues[Z] = map(probedValues[Z],minimumAnalogRead,maximumAnalogRead,MINIMUM_DISPLAY_SIGNAL_HEIGHT,MAXIMUM_DISPLAY_SIGNAL_HEIGHT);
          
  if(DEBUG){

      Serial.print("max probe: ");
      Serial.println(maxVoltageProbe);
      Serial.print("min probe: ");
      Serial.println(minVoltageProbe);
      Serial.print("voltage ratio: ");
      Serial.println(currentVoltageRatio,15);

      Serial.print("avg sum: ");
      Serial.println(avgVoltageProbe);
     
  }

  //depends on the range selected you can have different ratio to obtain the real voltages values
  maxVoltage = currentVoltageRatio*maxVoltageProbe;
  minVoltage = currentVoltageRatio*minVoltageProbe;
  avgVoltage = currentVoltageRatio*avgVoltageProbe;

  //here divide to have the average
  avgVoltage = avgVoltage / SIGNAL_FRAME_W;

  if(DEBUG){

      Serial.print("max voltage: ");
      Serial.println(maxVoltage);
      Serial.print("min voltage: ");
      Serial.println(minVoltage);
     
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

  //set micro size for all the text on the side
  u8g.setFont(u8g_font_micro);

  //draw selected voltage range
  u8g.drawStr(SIGNAL_FRAME_W+1, 58, voltageRangePrint);
  
  //draw fequency
  memset(freqPrint,'\0',9);
  dtostrf(samplingFrequency,5,0,freqPrint);
  
  if(samplingFrequency == SAMPLING_RATE_MAX_MICROSECONDS){

      freqPrint[0] = 'F';
      freqPrint[1] = ':';
      freqPrint[2] = '1';
      freqPrint[3] = '0';
      freqPrint[4] = 'K';
      freqPrint[5] = 'H';
      freqPrint[6] = 'Z';
   
  }else
  if(samplingFrequency >= 1000 && samplingFrequency < SAMPLING_RATE_MAX_MICROSECONDS){

      freqPrint[3] = freqPrint[2];
      freqPrint[2] = '.';
      freqPrint[4] = 'K';
      freqPrint[5] = 'H';
      freqPrint[6] = 'Z';

      //slide the content of the array of one positions right
      freqPrint[8] = freqPrint[7];
      freqPrint[7] = freqPrint[6];
      freqPrint[6] = freqPrint[5];
      freqPrint[5] = freqPrint[4];
      freqPrint[4] = freqPrint[3];
      freqPrint[3] = freqPrint[2];
      freqPrint[2] = freqPrint[1];

      //add the label:
      freqPrint[0] = 'F';
      freqPrint[1] = ':';
       
  }else{
   
    //write hz at the end
    freqPrint[5] = 'H';
    freqPrint[6] = 'Z';

     //add the label:
     freqPrint[0] = 'F';
     freqPrint[1] = ':';
   
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

  u8g.drawStr(SIGNAL_FRAME_W+1, 64, freqPrint);

  //print the statistic data
  memset(voltagePrint,'\0',8);
  dtostrf(minVoltage,5,2,voltagePrint);

  voltagePrint[8] = voltagePrint[7];
  voltagePrint[7] = voltagePrint[6];
  voltagePrint[6] = voltagePrint[5];
  voltagePrint[5] = voltagePrint[4];
  voltagePrint[4] = voltagePrint[3];
  voltagePrint[3] = voltagePrint[2];
  voltagePrint[2] = voltagePrint[1];
  voltagePrint[1] = ':';
  voltagePrint[0] = 'm';
  
  u8g.drawStr(SIGNAL_FRAME_W+1, 52, voltagePrint);

  memset(voltagePrint,'\0',8);
  dtostrf(avgVoltage,5,2,voltagePrint);

  voltagePrint[8] = voltagePrint[7];
  voltagePrint[7] = voltagePrint[6];
  voltagePrint[6] = voltagePrint[5];
  voltagePrint[5] = voltagePrint[4];
  voltagePrint[4] = voltagePrint[3];
  voltagePrint[3] = voltagePrint[2];
  voltagePrint[2] = voltagePrint[1];
  voltagePrint[1] = ':';
  voltagePrint[0] = 'A';

  u8g.drawStr(SIGNAL_FRAME_W+1, 46, voltagePrint);

  memset(voltagePrint,'\0',8);
  dtostrf(maxVoltage,5,2,voltagePrint);

  voltagePrint[8] = voltagePrint[7];
  voltagePrint[7] = voltagePrint[6];
  voltagePrint[6] = voltagePrint[5];
  voltagePrint[5] = voltagePrint[4];
  voltagePrint[4] = voltagePrint[3];
  voltagePrint[3] = voltagePrint[2];
  voltagePrint[2] = voltagePrint[1];
  voltagePrint[1] = ':';
  voltagePrint[0] = 'M';
  
  u8g.drawStr(SIGNAL_FRAME_W+1, 40, voltagePrint);

  //print the fit
  if(fitFunction)
    u8g.drawStr(SIGNAL_FRAME_W+1, 34, "FIT:yes");
  else
    u8g.drawStr(SIGNAL_FRAME_W+1, 34, "FIT:no");
    
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

  memset(voltageRangePrint,'\0',9);

  voltageRangePrint[0] = 'R';
  voltageRangePrint[1] = ':';
  voltageRangePrint[2] = '0';
  voltageRangePrint[3] = '-';
  voltageRangePrint[4] = '5';
  voltageRangePrint[5] = '0';
  voltageRangePrint[6] = 'V';

  currentRange = 50;
  currentVoltageRatio = voltageRatio50;
 
}

void switch_5(){

  digitalWrite(RELAY_K1_PIN,LOW);
  digitalWrite(RELAY_K2_PIN,LOW);
  digitalWrite(RELAY_K3_PIN,LOW);
  digitalWrite(RELAY_K4_PIN,LOW);
  digitalWrite(RELAY_K5_PIN,LOW);
  digitalWrite(RELAY_K6_PIN,LOW);

  memset(voltageRangePrint,'\0',9);

  voltageRangePrint[0] = 'R';
  voltageRangePrint[1] = ':';
  voltageRangePrint[2] = '0';
  voltageRangePrint[3] = '-';
  voltageRangePrint[4] = '5';
  voltageRangePrint[5] = 'V'; 

  currentRange = 5;
  currentVoltageRatio = voltageRatio5;
 
}

void switch_120(){

  digitalWrite(RELAY_K1_PIN,HIGH);
  digitalWrite(RELAY_K2_PIN,LOW);
  digitalWrite(RELAY_K3_PIN,LOW);
  digitalWrite(RELAY_K4_PIN,HIGH);
  digitalWrite(RELAY_K5_PIN,LOW);
  digitalWrite(RELAY_K6_PIN,LOW);

  memset(voltageRangePrint,'\0',9);

  voltageRangePrint[0] = 'R';
  voltageRangePrint[1] = ':';
  voltageRangePrint[2] = '0';
  voltageRangePrint[3] = '-';
  voltageRangePrint[4] = '1';
  voltageRangePrint[5] = '2';
  voltageRangePrint[6] = '0';
  voltageRangePrint[7] = 'V';

  currentRange = 120;
  currentVoltageRatio = voltageRatio120;
 
}

void switch_200(){

  digitalWrite(RELAY_K1_PIN,LOW);
  digitalWrite(RELAY_K2_PIN,HIGH);
  digitalWrite(RELAY_K3_PIN,LOW);
  digitalWrite(RELAY_K4_PIN,LOW);
  digitalWrite(RELAY_K5_PIN,HIGH);
  digitalWrite(RELAY_K6_PIN,LOW);

  memset(voltageRangePrint,'\0',9);

  voltageRangePrint[0] = 'R';
  voltageRangePrint[1] = ':';
  voltageRangePrint[2] = '0';
  voltageRangePrint[3] = '-';
  voltageRangePrint[4] = '2';
  voltageRangePrint[5] = '0';
  voltageRangePrint[6] = '0';
  voltageRangePrint[7] = 'V';

  currentRange = 200;
  currentVoltageRatio = voltageRatio200;
 
}

void interruptFunctionRange(){

  buttonRangeTimePressed1 = millis();

  if((buttonRangeTimePressed1-buttonRangeTimePressed2) > BUTTON_RANGE_TIME_PRESS_AGAIN){
   
    if(currentRange == 5)
      switch_50();
    else
    if(currentRange == 50)
      switch_120();
    else
    if(currentRange == 120)
      switch_200();
     else
      switch_5();

    buttonRangeTimePressed2 = millis();
 
  }
 
}

void interruptFunctionFit(){

  if(fitFunction)
    fitFunction = false;
  else
    fitFunction = true;

    Serial.println("interruptFunctionFit");
  
}
