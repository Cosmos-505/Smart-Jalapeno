/* 
 * Project myProject
 * Author: Your Name
 * Date: 
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include "Stepper.h"
#include "IoTTimer.h"
#include "Neopixel.h"
#include "Colors.h"
#include "Adafruit_VEML7700.h"
#include "Wire.h"
#include "Button.h"
#include "DS18B20.h"
#include "math.h"



// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(MANUAL);

//DS18 TEMP SENSOR


const int16_t dsData = D3;
// Sets Pin D3 as data pin and the only sensor on bus
DS18B20  ds18b20(dsData, true); 
void getTemp();

char     szInfo[64];
double   celsius;
double   fahrenheit;
uint32_t msLastMetric;
uint32_t msLastSample;


//WATER LEVEL SENSOR
const int THRESHOLD = 100;
const int ATTINY1_HIGH_ADDR = 0x78;
const int ATTINY2_LOW_ADDR = 0x77;

unsigned char low_data[8] = {0};
unsigned char high_data[12] = {0};

void checkWaterLevel();
void getHigh12SectionValue();
void getLow8SectionValue();


//PH SENSOR
const int PHPIN = A5;
float pHValue;
float voltage;
float offset = 16.2727;
float calibrationSlope = -6.19835;
const int SAMPLINGINTERVAL = 2500;
const int PRINTINTERVAL = 10000;
unsigned int lastSample = 0;
unsigned int lastPrint = 0;
const int PH_ARRAY_LENGTH = 40;
int phArray[PH_ARRAY_LENGTH];
int phArrayIndex = 0;
double avergearray(int *arr, int number);

unsigned int samplingTime;
unsigned int printTime;

//VEML LIGHT SENSOR
Adafruit_VEML7700 veml;
const int VEMLAddr = 0x10; 

//FUNCTIONS FOR USE LATER

void pixelFill(int first, int last, int color);


//NEOPIXEL SPECTRUM LIGHT
const int pixelPin = D2;
Adafruit_NeoPixel pixels(37, pixelPin, WS2812B); 
const int NUM_PIXELS = 37;
int first;
int last;
int color;
int segment;

//STEPPER MOTOR and BUTTON
Button button(D5, INPUT_PULLDOWN);
const int buttonPin = D5;
Stepper stepper (2048, D10, D11, D12, D13);
IoTTimer feedTimer;

// DS WATER TEMP SENSOR
const int      MAXRETRY          = 4;
const uint32_t msSAMPLE_INTERVAL = 2500;
const uint32_t msMETRIC_PUBLISH  = 10000;








void setup() {

//WATER LEVEL SENSOR
  Wire.begin();

//NEO PIXEL STRIP set all pixels to white
  pixels.begin();
  pixels.setBrightness(20);
  pixelFill(0,37,white);
  pixels.show();

    //Serial Print 
  Serial.begin(9600);
  waitFor(Serial.isConnected,10000);

// Stepper motor, button and Feed timer
  stepper.setSpeed(10);
  feedTimer.startTimer(30000);
  pinMode(D5,INPUT_PULLDOWN);
  

Serial.printf("Adafruit VEML7700 Test");

  if (!veml.begin())
  {
    Serial.printf("Sensor not found");
    while (1);
  }
  Serial.printf("Sensor found");

  veml.setGain(VEML7700_GAIN_1);
  veml.setIntegrationTime(VEML7700_IT_800MS);

  Serial.print(F("Gain: "));
  switch (veml.getGain())
  {
  case VEML7700_GAIN_1:
    Serial.printf("1");
    break;
  case VEML7700_GAIN_2:
    Serial.printf("2");
    break;
  case VEML7700_GAIN_1_4:
    Serial.printf("1/4");
    break;
  case VEML7700_GAIN_1_8:
    Serial.printf("1/8");
    break;
  }

  Serial.print(F("Integration Time (ms): "));
  switch (veml.getIntegrationTime())
  {
  case VEML7700_IT_25MS:
    Serial.printf("25");
    break;
  case VEML7700_IT_50MS:
    Serial.printf("50");
    break;
  case VEML7700_IT_100MS:
    Serial.printf("100");
    break;
  case VEML7700_IT_200MS:
    Serial.printf("200");
    break;
  case VEML7700_IT_400MS:
    Serial.printf("400");
    break;
  case VEML7700_IT_800MS:
    Serial.printf("800");
    break;
  }

  veml.powerSaveEnable(true);
  veml.setPowerSaveMode(VEML7700_POWERSAVE_MODE4);

  veml.setLowThreshold(5000);
  veml.setHighThreshold(20000);
  veml.interruptEnable(true);
}






// loop() runs over and over again, as quickly as it can execute.
void loop() {
  if (feedTimer.isTimerReady()) {
  Serial.printf("MOTOR MOVING\nMOTOR MOVING\n");
  stepper.step(-40);
  delay(100);
  stepper.step(100);
  feedTimer.startTimer(30000);
  }
  
//FEED BUTTON
  int buttonstate;
  buttonstate = digitalRead(buttonPin);
    if (buttonstate ==1) {
       stepper.step(1);
      Serial.printf("Feeding fish\n");
      Serial.printf("Feeding fish %i steps\n",buttonPin);
    }



  if (millis() - samplingTime > SAMPLINGINTERVAL) {//2.5 seconds
    
      //read pH
    phArray[phArrayIndex++] = analogRead(PHPIN);
    if (phArrayIndex == PH_ARRAY_LENGTH)
      phArrayIndex = 0;
    voltage = avergearray(phArray, PH_ARRAY_LENGTH) * 3.3 / 4096;
    pHValue = calibrationSlope * voltage + offset;
    samplingTime = millis();

 
  }



  if (millis() - printTime > PRINTINTERVAL) //10 seconds
  {
    //Water Level
    checkWaterLevel();
    //print ph
                //LIGHT VALUES
  Serial.printf("Lux: ");
  Serial.println(veml.readLux());
  Serial.print("White: ");
  Serial.println(veml.readWhite());
  Serial.print("Raw ALS: ");
  Serial.println(veml.readALS());  

    //read temp
      getTemp();
    Serial.printf("Voltage: %f pH: %f\n", voltage, pHValue);
    Serial.println("\n*********************************************************\n");
    
    printTime = millis();
    //publishData();


  }
}









//PIXELFILL
void  pixelFill (int first, int last, int color) {
   int i;
   for (i = first; i<= last; i++) {
      pixels.setPixelColor(i,white);
      pixels.show();
   }
}

double avergearray(int *arr, int number)
{
  int i;
  int max, min;
  double avg;
  long amount = 0;
  if (number <= 0)
  {
    Serial.println("Error number for the array to avraging!/n");
    return 0;
  }
  if (number < 5)
  { // less than 5, calculated directly statistics
    for (i = 0; i < number; i++)
    {
      amount += arr[i];
    }
    avg = amount / number;
    return avg;
  }
  else
  {
    if (arr[0] < arr[1])
    {
      min = arr[0];
      max = arr[1];
    }
    else
    {
      min = arr[1];
      max = arr[0];
    }
    for (i = 2; i < number; i++)
    {
      if (arr[i] < min)
      {
        amount += min; // arr<min
        min = arr[i];
      }
      else
      {
        if (arr[i] > max)
        {
          amount += max; // arr>max
          max = arr[i];
        }
        else
        {
          amount += arr[i]; // min<=arr<=max
        }
      } // if
    }   // for
    avg = (double)amount / (number - 2);
  } // if
  return avg;
}

void getHigh12SectionValue() {
  memset(high_data, 0, sizeof(high_data));
  Wire.beginTransmission(ATTINY1_HIGH_ADDR);
  Wire.write(0x00); // Register address
  Wire.endTransmission();

  Wire.requestFrom(ATTINY1_HIGH_ADDR, 12);
  while (12 != Wire.available());

  for (int i = 0; i < 12; i++) {
    high_data[i] = Wire.read();
  }
  delay(10);
}

void getLow8SectionValue() {
  memset(low_data, 0, sizeof(low_data));
  Wire.beginTransmission(ATTINY2_LOW_ADDR);
  Wire.write(0x00); // Register address
  Wire.endTransmission();

  Wire.requestFrom(ATTINY2_LOW_ADDR, 8);
  while (8 != Wire.available());

  for (int i = 0; i < 8; i++) {
    low_data[i] = Wire.read();
  }
  delay(10);
}

void checkWaterLevel() {
  int sensorvalue_min = 250;
  int sensorvalue_max = 255;
  int low_count = 0;
  int high_count = 0;

 // while (1) {
    uint32_t touch_val = 0;
    uint8_t trig_section = 0;
    low_count = 0;
    high_count = 0;
    
    getLow8SectionValue();
    getHigh12SectionValue();

    Serial.println("low 8 sections value = ");
    for (int i = 0; i < 8; i++) {
      Serial.printf("%d.", low_data[i]);
      if (low_data[i] >= sensorvalue_min && low_data[i] <= sensorvalue_max) {
        low_count++;
      }
    
    }
    Serial.println("  ");
    Serial.println("  ");
    Serial.println("high 12 sections value = ");
    for (int i = 0; i < 12; i++) {
      Serial.printf("%d.", high_data[i]);
      if (high_data[i] >= sensorvalue_min && high_data[i] <= sensorvalue_max) {
        high_count++;
      }
      
    }
    Serial.println("  ");
    Serial.println("  ");

    for (int i = 0 ; i < 8; i++) {
      if (low_data[i] > THRESHOLD) {
        touch_val |= 1 << i;
      }
    }
    for (int i = 0 ; i < 12; i++) {
      if (high_data[i] > THRESHOLD) {
        touch_val |= (uint32_t)1 << (8 + i);
      }
    }

    while (touch_val & 0x01) {
      trig_section++;
      touch_val >>= 1;
    }
    Serial.printf("water level = %d%% \n", trig_section * 5);
    Serial.println("\n*********************************************************\n");
    
  }

void publishData(){
  sprintf(szInfo, "Temp is %2.2fF", fahrenheit);
  Particle.publish("dsTmp", szInfo, PRIVATE);
  msLastMetric = millis();
}

void getTemp(){
  float _temp;
  int   i = 0;

  do {
    _temp = ds18b20.getTemperature();
  } while (!ds18b20.crcCheck() && MAXRETRY > i++);

  if (i < MAXRETRY) {
    celsius = _temp;
    fahrenheit = ds18b20.convertToFahrenheit(_temp);
    Serial.printf("Temp is %.2fF\n", fahrenheit);
  }
  else {
    celsius = fahrenheit = NAN;
    Serial.println("Invalid reading");
  }
  msLastSample = millis();
}