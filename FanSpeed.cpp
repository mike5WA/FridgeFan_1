/*********************************************************************
 10/7/19 
 Added routines to display fridge temp on 12 seg led bar graph with 2 * 74HC595 shift registers
 NB the led bar graph when all on draws ~0.07A.
 Led and shift registers therfore require separate 5V supply as Arduino via USB not enough at 0.025A
 
 Hardware:
 * 12 segment led KWL-R1230CDUGB 14 pin.Common cathode 
 * 2 led's /segment Red + Green = 24 total + both on = yellow
 * 2 * 74HC595 shift register daisy chained.
 * Register 1 connected to anodes (8) pins (R/G) 2/13;3/12;4/11;5/10 via 330 ohm R
 * Register 2 Q0, Q1 & Q2 connected to base of npn transistors as switches
 * Led cathodes pins 1/14,6/9,7/8 connected to collector of npn transistor via 10K ohm R
 * LEDs attached to each of the outputs of the shift register (8) * 3 = 24
 * Only 3 outputs are needed for cathode register so only MSB bytes 1-7 required
 * Cathode byte 1 = 100; segments A-D
 * Cathode byte 2 = 010; segments E-H
 * Cathode byte 3 = 110; segments A-H
 * Cathode byte 4 = 001; segments I-L
 * Cathode byte 5 = 101; segments A-D & I-L
 * Cathode byte 6 = 011; segments E-L
 * Cathode byte 7 }= 111; segments A-L
 * ie for all on "Yellow" = Cathode byte 7 & Anode Byte 255 "111 & 11111111"
 * Cathode & Anode data stored in arrays
 
NB Fans run at 12V don't mix up!!!

18/7/19
Get lux levels so led's can be turned down at night
Deleted weathershield data so this now only recording Fridge temp

2/8/19
Amended to get temperature from DHT22 sensor, previously I2C DSB180

8/8/19  **** Full working program for installation ******
Amended program to get cathode & anode values from arrays.
Added error routine to fash led and put fan at full if temp sensor fails.

 Arduino Pins:
 A1   Light sensor
 P2   DHT22 temp & humidity sensor.
 P3   PWM pin to 74HC595 P13 OE for display brightness
 P4   To P14 SER (Data) of 74HC595
 D5   Latch pin to RCLK or ST_CP of 595 P12
 D6   To P11 of 74HC595 SRCLK Clock
 D9   PWM To base of fan transistor
 D11  connected to DS of 74HC595
 D12  connected to SH_CP of 74HC595
*/
 
//Libraries
#include <DHT.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#define DHTTYPE DHT22
#define DHTPIN 2                  // digital pin DHT sensor connected to
DHT dht(DHTPIN, DHTTYPE);

//Hardware pin definitions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
const byte LIGHT = A1;            //Analog pin for light sensor data
const byte PWM_595 = 3;           //PWM pin for 74HC595 connected to OE

//Global Variables ****************************************************************

long lastSecond;        //The millis counter to see when a second rolls by

//Fridge temp & control variables

int fan = false;        //Set fan to off "0"
float fanSpeed;         
float fanPercent;
int tempMin = 5;        //temperature at which fan starts
int tempMax = 40;       //temperature at which fan at 100%//Also put fan at 100%


int fanPin = 9;         //pin for fan control

//Shift Register variables
//internal function setup P13 (OE)to GND; P10 (SRCLR) to 5V
int i=0;
int pinState;
int latchPin = 5;       //Pin connected to RCLK (ST_CP) Latch P12 of 74HC595
int dataPin = 4;        //Pin connected to DS (SER) P14 of 74HC5952
int clockPin = 6;       //Pin connected to SH_CP (SRCLK) P11 of 74HC595

int reg_A = 0;          //Anode register 595
int reg_C = 0;          //Cathode register 595
int ledDisplay;         //Case number
byte dataAnode[21];
byte dataCathode[21];
int j;                  //Counter
int ledLvl;             //Variable to map light level for PWM
int lightSensor;        //Variable for light sensor

//Other sensor variables
float temp_F = 0;       //variable for fridge temp

//***********************************************************************************

void setup()
{
// start serial port
  Serial.begin(9600);
  pinMode(dataPin, OUTPUT);   //added
  pinMode(latchPin, OUTPUT);  //595 shift register
  pinMode(clockPin, OUTPUT);  //added

//Shift register data
//Array of anode data
  dataAnode[0] = 0x5F;  //ob01011111    D95   RYRY  Error
  dataAnode[1] = 0x10;  //ob00010000    D16   ***G  0 - 3
  dataAnode[2] = 0x10;  //ob00010000    D16   ***G  3 - 9
  dataAnode[3] = 0x10;  //ob00010000    D16   ***G  9 - 12
  dataAnode[4] = 0x30;  //ob00110000    D48   **GG  12 - 15
  dataAnode[5] = 0x30;  //ob00110000    D48   **GG  15 - 18
  dataAnode[6] = 0x70;  //ob01110000    D112  *GGG  18 - 21
  dataAnode[7] = 0x70;  //ob01110000    D112  *GGG  21 - 24
  dataAnode[8] = 0xF0;  //ob11110000    D240  GGGG  24 - 27
  dataAnode[9] = 0xF0;  //ob11111000    D240  GGGG  27 - 30
  dataAnode[10] = 0xF0; //ob11110000    D240  GGGG  30 - 33
  dataAnode[11] = 0xF0; //ob11110000    D240  GGGG  33 - 36
  dataAnode[12] = 0xF0; //ob11110000    D240  GGGG  36 - 39
  dataAnode[13] = 0xF1; //ob11110001    D241  GGGY  39 - 42
  dataAnode[14] = 0xF3; //ob11110011    D243  GGYY  42 - 45
  dataAnode[15] = 0xF7; //ob11110111    D247  GYYY  45 - 48
  dataAnode[16] = 0xFF; //ob11111111    D255  YYYY  48 - 51
  dataAnode[17] = 0xEF; //ob11101111    D239  YYYR  51 - 54
  dataAnode[18] = 0xCF; //ob11001111    D207  YYRR  54 - 57
  dataAnode[19] = 0x8F; //ob10001111    D143  YRRR  57 - 60
  dataAnode[20] = 0xF;  //ob00001111    D15   RRRR  60+

//Array of Cathode data 0x1=SegA; 0x3=SegB; 0x7=SegC 
  dataCathode[0] = 0x7; 
  dataCathode[1] = 0x1; 
  dataCathode[2] = 0x1; 
  dataCathode[3] = 0x1; 
  dataCathode[3] = 0x1; 
  dataCathode[4] = 0x1;
  dataCathode[5] = 0x1;
  dataCathode[6] = 0x1;
  dataCathode[7] = 0x1;
  dataCathode[8] = 0x1;
  dataCathode[9] = 0x3;
  dataCathode[10] = 0x3;
  dataCathode[11] = 0x7;
  dataCathode[12] = 0x7;
  dataCathode[13] = 0x7;
  dataCathode[14] = 0x7;
  dataCathode[15] = 0x7;
  dataCathode[16] = 0x7;
  dataCathode[17] = 0x7;
  dataCathode[18] = 0x7;
  dataCathode[19] = 0x7;
  dataCathode[20] = 0x7;
    
//Start the DHT sensor 
    dht.begin();
//Give sensor time to respond
    delay(3000);
    
//Check we are getting a reading
     temp_F = dht.readTemperature();
     //Serial.println(temp_F);
     if (isnan (temp_F))
     { 
//Pass reading to routine which will loop while (t_dht) is not a number (isnan)
      Serial.println("DHT sensor offline!");
      //dht_OffLine(temp_F);
      }
     Serial.println("DHT sensor online!");

//Set start of program to enable readings at specific intervals
  //int seconds = 0;
  lastSecond = millis();    //Returns the number of miliseconds program has been running
 }

//End of Void Setup-----------------------------------------------------------------------

//If sensor fails (isnan). Put fans to 100%
//Flash red/yellow led 

float dht_OffLine (float temp_F)
{
   if (isnan(temp_F))
   {
      do
      {
        analogWrite(fanPin, 255);   // Fan to full 
        analogWrite(PWM_595,0);     //Take OE low to turn on register
//Flash RYRY & RRRR        
        reg_A = dataAnode[0];       //RYRY
        reg_C = dataCathode[0];     //All 3 segments
        //Serial.print("DHT sensor offline ");
        //Serial.println(reg_A);
//Set registers C (Cathode); A (Anode) from ledCase routine
        digitalWrite(latchPin, 0);
        shiftOut(dataPin, clockPin, reg_C);
        digitalWrite(latchPin, 0);
        shiftOut(dataPin, clockPin, reg_A);
// This byte will fill up register 1
//return the latch pin high to signal chip that it no longer needs to listen for information
        digitalWrite(latchPin, 1);
//Turn led's red after 1.5 secs    
        delay(2000);
        reg_A = dataAnode[20];
        reg_C = dataCathode[20];
        //Serial.print("DHT sensor offline ");
        //Serial.println(reg_A);
        digitalWrite(latchPin, 0);
        shiftOut(dataPin, clockPin, reg_C);
        digitalWrite(latchPin, 0);
        shiftOut(dataPin, clockPin, reg_A);
// This byte will fill up register 1
//return the latch pin high to signal chip that it no longer needs to listen for information
        digitalWrite(latchPin, 1);
        delay(2000);

//Re-read DHT sensor
        temp_F = dht.readTemperature();
      }
//Check again to see if we have a valid reading 
      while (isnan(temp_F));
    }
//If we do return reading and back to main loop  
  return temp_F;
}
//------------------------------------------------------------------


//---------------------------------------------------------------------------------------
//Delay for a given amount of time (ms)
static void smartDelay(unsigned long ms)  
{
  unsigned long start = millis(); //time program been running
  do 
  {
    delay(100);
  } while (millis() - start < ms);  //Exit once time ticks over ms value
}
//--------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------   

//Routine to set fan speed. On initial startup fan will stall if rpm too low.
//So start fan at full then once running map to appropriate level

int fanControl(float temp_F)
{
  if (fan != 1)   //Not equal to 1 ie has not been running
  {
    Serial.println("1st start ");
    fanSpeed = 255; 
    return fanSpeed;    
  }
  else            //Has been running so map
  {
    fanSpeed = map(temp_F, tempMin, tempMax, 25, 255);  //25 = ~10% of max
    constrain (fanSpeed, 25, 255);
    return fanSpeed; 
  }
}
//---------------------------------------------------------------------------------------------

//shiftOut routine moves data bit by bit into the 2 shift registers

void shiftOut(int myDataPin, int myClockPin, byte myDataOut)
{
// This shifts 8 bits out MSB first on the rising edge of the clock,
//clock idles low

//internal function setup
  int i=0;
  int pinState;
  pinMode(myClockPin, OUTPUT);
  pinMode(myDataPin, OUTPUT);

//clear everything out just in case to
//prepare shift register for bit shifting
  digitalWrite(myDataPin, 0);
  digitalWrite(myClockPin, 0);

//for each bit in the byte myDataOut.
//NOTICE THAT WE ARE COUNTING DOWN in our for loop
//This means that %00000001 or "1" will go through such
//that it will be pin Q0 that lights. 
  for (i=7; i>=0; i--)
  {
    digitalWrite(myClockPin, 0);

//if the value passed to myDataOut and a bitmask result 
// true then... so if we are at i=6 and our value is
// %11010100 it would the code compares it to %01000000 
// and proceeds to set pinState to 1.
    if ( myDataOut & (1<<i) )
    {
      pinState= 1;
    }
    
    else
    {  
      pinState= 0;
    }
//Sets the pin to HIGH or LOW depending on pinState
    digitalWrite(myDataPin, pinState);
//register shifts bits on upstroke of clock pin  
    digitalWrite(myClockPin, 1);
//zero the data pin after shift to prevent bleed through
    digitalWrite(myDataPin, 0);
    }
//stop shifting
    digitalWrite(myClockPin, 0);
}
//----------------------------------------------------------------------------------------------

//-------------------------------------------------------------------
//Collation of data for display
void displayData ()
{
  Serial.println();
  Serial.println("Current sensor readings: ");
  
  //Fridge temperature & fan Speed
  Serial.print("Fridge ");
  Serial.print(temp_F, 0); 
  Serial.print("\xC2\xB0"); //Prints the degree symbol
  Serial.print("C ; ");
  Serial.print(" Fan Speed PWM = ");
  Serial.print(fanSpeed);
  Serial.print(" Fan ");
  Serial.print(fanPercent, 0);
  Serial.println(" %");
  
  //12 Segment LED Data
  Serial.print("Fridge Temp = ");
  Serial.print(temp_F, 0);
  Serial.print("\xC2\xB0"); //Prints the degree symbol
  Serial.print(" : LedDisplay = ");
  Serial.print(ledDisplay);
  Serial.print(" Anode ");
  Serial.print(reg_A);
  Serial.print(" ; Cathode ");
  Serial.println(reg_C);

  //Light level data
  Serial.print("light_lvl = ");
  Serial.print(lightSensor);
  Serial.print(" lux,"); 
  Serial.println(ledLvl); 
  
}
//--------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------

void loop()
{
//Read DHT sensor
      temp_F = dht.readTemperature();
      if (isnan (temp_F))
     { 
//Pass reading to routine which will loop while (t_dht) is not a number (isnan)
      //Serial.println("DHT sensor offline!");
      dht_OffLine(temp_F);
      }

//Fan Control
  if (temp_F<tempMin)       //Skip mapping routine no fan required
    {
      fan = 0;              //Set fan to False or off not running
      fanSpeed = 0;         //No fan needed
    }
  if (temp_F>tempMax)       //Override fancontrol fans at max
    {
      fan = 1;
      fanSpeed = 255;
    }
  else 
    {
      fanSpeed = fanControl(temp_F);      //Access mapping routine
      fan = 1;                            //Set fan to 1 as now running
    }  
  fanPercent = (fanSpeed/255)*100;    //Gives percent of full speed
  analogWrite(fanPin, fanSpeed);      // sends PWM signal to pin_D9 for fan control 

/*
//Led graph test routine uncomment to loop through data Array

for (int j = 0; j < 21; j++)
  {
//load the light sequence you want from array
    reg_C = dataCathode[j];
    reg_A = dataAnode[j];
    Serial.print ("Cathode & Anode ");
    Serial.print (reg_C);
    Serial.print (" ");
    Serial.println (reg_A);
    digitalWrite(latchPin, 0);
    shiftOut(dataPin, clockPin, reg_C);
//ground latchPin and hold low for as long as you are transmitting
    digitalWrite(latchPin, 0);
//move 'em out
    shiftOut(dataPin, clockPin, reg_A);
//return the latch pin high to signal chip that it
//no longer needs to listen for information
    digitalWrite(latchPin, 1);
//Delay to monitor
    delay(3000);
  }
*/

//Display fridge temperature on 12 segment bar graph
//Get value between 1 & 20 for temp range 0 to 60 3deg intervals
  ledDisplay = map(temp_F, 0, 60, 1, 20);  
  constrain (ledDisplay, 1,20);
//Seems to give reading +1??? ie 16C gives display array 6 not 5???? Rounding up???

//load appropriate dataArray reference for cathodes & anodes
  reg_C = dataCathode[ledDisplay];
  reg_A = dataAnode[ledDisplay];
  
//Then sets reg_A & reg_C for ledDisplay
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, reg_C);       
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, reg_A);     
//This byte will fill up register 1
//return the latch pin high to signal chip that it no longer needs to listen for information
  digitalWrite(latchPin, HIGH);

//Check light sensor return ledLvl
  
  lightSensor = analogRead(LIGHT);
//Serial.println (lightSensor);
//Will give value from 0 (dark) to 1024 (bright). Map from 0 to 255 between 0 and 1024
//npn 595 is off when OE pin is high thusly low pwm (0) = on high (255) = off

  ledLvl = map(lightSensor, 1024, 50, 0, 225); //Full on when bright dim when dark
  constrain(ledLvl, 0, 255); 

//Adjust led bar graph brightness full daytime dim nightime
 analogWrite(PWM_595, ledLvl);      // sends PWM signal to pin_3 for led lux control 

//Display/Print Data
  displayData ();

//Delay
  smartDelay(5000);   
  
}

//End of void loop*************************************************************************








//END-------------------------------------------------------------------------------------------
