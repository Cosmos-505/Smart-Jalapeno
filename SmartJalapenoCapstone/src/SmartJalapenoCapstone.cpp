/* 
 * Project Smart Jalapeno Capstone
 * Author: Chris Cade
 * Date: April 6th, 2024
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include "Adafruit_VEML7700.h"
#include "Stepper.h"
#include "IoTTimer.h"
#include "DS18B20.h"
#include "OneWire.h"
#include "math.h"
#include "Adafruit_MQTT.h"
#include "neopixel.h"
#include "Colors.h"


// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(SEMI_AUTOMATIC);

//PH SENSOR
#define SensorPin A0            //pH meter Analog output to Arduino Analog Input 0
#define Offset 0.00            //deviation compensate
#define LED 13
#define samplingInterval 20
#define printInterval 800
#define ArrayLenth  40    //times of collection
int pHArray[ArrayLenth];   //Store the average value of the sensor feedback
int pHArrayIndex=0;
static float pHValue, voltage;

//WATER LEVEL SENSOR
unsigned char low_data[8] = {0};
unsigned char high_data[12] = {0};
void getHigh12SectionValue();
void getlow8SectionValue();
void checkWaterLevel();

#define NO_TOUCH       0xFE
#define THRESHOLD      100
#define ATTINY1_HIGH_ADDR   0x78
#define ATTINY2_LOW_ADDR   0x77

//NEOPIXEL SPECTRUM LIGHT
const int pixelPin = D2;
Adafruit_NeoPixel pixels(37, pixelPin, WS2812B); 
const int NUM_PIXELS = 37;
int first;
int last;
int color;
int segment;

//VEML LIGHT SENSOR
Adafruit_VEML7700 veml;
const int VEMLAddr = 0x10; 

//STEPPER MOTOR
Stepper stepper (2048, D10, D11, D12, D13);
IoTTimer feedTimer;

// DS WATER TEMP SENSOR
const int      MAXRETRY          = 4;
const uint32_t msSAMPLE_INTERVAL = 2500;
const uint32_t msMETRIC_PUBLISH  = 10000;



// Sets Pin D3 as data pin and the only sensor on bus
DS18B20  ds18b20(D3, true); 
int DSPin = D3;
OneWire ds(DSPin);


//FUNCTIONS FOR USE LATER
void readWaterLevel();
void getTemp();
void pixelFill(int first, int last, int color);
void publishData();
double averageArray(int*arr, int number);

char     szInfo[64];
float   celsius;
float   fahrenheit;
uint32_t msLastMetric;
uint32_t msLastSample; 











// setup() runs once, when the device is first turned on
void setup() {


//PH SENSOR INIT
pinMode(A0, OUTPUT);
// MAIN LED CONTROL
 // RGB.control(true);

 // Initialize I2C communication
  Wire.begin();

//NEO PIXEL STRIP set all pixels to white
  pixels.begin();
  pixels.setBrightness(20);
  pixelFill(0,39,white);
  pixels.show();

  //Serial Print 
  Serial.begin(9600);
  waitFor(Serial.isConnected,10000);

// Stepper motor and Feed timer
  stepper.setSpeed(10);
  feedTimer.startTimer(30000);


//VEML LIGHT SENSOR STARTUPS
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



















void loop(){


//Stepper Motor and Feed Timer
if (feedTimer.isTimerReady()) {
  Serial.printf("MOTOR MOVING\nMOTOR MOVING\n");
  stepper.step(-800);
  delay(100);
  stepper.step(1500);
  feedTimer.startTimer(30000);
}

  //DS TEMP DATA
  if (millis() - msLastSample >= msSAMPLE_INTERVAL){
    getTemp();
    checkWaterLevel();

  
    //LIGHT MEASURE IS CONSTANT
    

  //LIGHT VALUES
  Serial.printf("Lux: ");
  Serial.println(veml.readLux());
  Serial.print("White: ");
  Serial.println(veml.readWhite());
  Serial.print("Raw ALS: ");
  Serial.println(veml.readALS());

  }

  if (millis() - msLastMetric >= msMETRIC_PUBLISH){
    Serial.printf("GONNA PUBLISH EVERY 10 SECONDS.\n");
    publishData();
  }




  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static float pHValue,voltage;
  if(millis()-samplingTime > samplingInterval)
  {
      pHArray[pHArrayIndex++]=analogRead(SensorPin);
      if(pHArrayIndex==ArrayLenth)pHArrayIndex=0;
      voltage = averageArray(pHArray, ArrayLenth)*5.0/4096;
      pHValue = 3.5*voltage+Offset;
      samplingTime=millis();
  }
  if(millis() - printTime > printInterval)   //Every 800 milliseconds, print a numerical, convert the state of the LED indicator
  {
    Serial.print("Voltage:");
        Serial.print(voltage,2);
        Serial.print("    pH value: ");
    Serial.println(pHValue,2);
        digitalWrite(LED,digitalRead(LED)^1);
        printTime=millis();
  }
}
















double averageArray(int* arr, int number){
  int i;
  int max,min;
  double avg;
  long amount=0;
  if(number<=0){
    Serial.println("Error number for the array to averaging!/n");
    return 0;
  }
  if(number<5){   //less than 5, calculated directly statistics
    for(i=0;i<number;i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }else{
    if(arr[0]<arr[1]){
      min = arr[0];max=arr[1];
    }
    else{
      min=arr[1];max=arr[0];
    }
    for(i=2;i<number;i++){
      if(arr[i]<min){
        amount+=min;        //arr<min
        min=arr[i];
      }else {
        if(arr[i]>max){
          amount+=max;    //arr>max
          max=arr[i];
        }else{
          amount+=arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount/(number-2);
  }//if
  return avg;
}
















// PUBLISH WATER TEMP DATA GO HERE
void publishData(){
  sprintf(szInfo, "%2.2f", fahrenheit);
  Particle.publish("dsTmp", szInfo, PRIVATE);
  msLastMetric = millis();
}

//GET TEMP FROM DS WATER TEMP
void getTemp(){
  float _temp;
  int   i = 0;

  do {
    _temp = ds18b20.getTemperature();
  } while (!ds18b20.crcCheck() && MAXRETRY > i++);

  if (i < MAXRETRY) {
    celsius = _temp;
    fahrenheit = ds18b20.convertToFahrenheit(_temp);
    Serial.printf("Fahrenheit reading is: %f\n", fahrenheit);
  }
  else {
    celsius = fahrenheit = NAN;
    Serial.println("Invalid reading");
  }
  msLastSample = millis();
}


//PIXELFILL
void  pixelFill (int first, int last, int color) {
   int i;
   for (i = first; i<= last; i++) {
      pixels.setPixelColor(i,white);
      pixels.show();
   }
}

//GROVE WATER LEVEL SENSOR
void readWaterLevel() {
    // Request data from the water level sensor
    Wire.beginTransmission(0x77);
    Wire.write(0x00); // Send command to read water level
    Wire.endTransmission();

    // Wait a short delay for the sensor to respond
    delay(100);

    // Read data from the sensor
    Wire.requestFrom(0x77, 1); // Request 1 byte of data
    if (Wire.available()) {
        uint8_t waterLevel = Wire.read(); // Read water level data
        // Convert water level data to percentage (0-100)
        float waterLevelPercent = (waterLevel * 100.0) / 255.0;

        // Print the water level percentage to serial monitor
        Serial.printf("Water Level: %f%%\n", waterLevelPercent);

//         // Update the NeoPixel strip based on water level
        if (waterLevelPercent < 50) {
            // Low water level, set pixels to blue
            RGB.color(255,0,0);
        } else {
            // High water level, set pixels to green
            RGB.color(0,255,0);
        }
     }
 }

 void getHigh12SectionValue(void)
{
  memset(high_data, 0, sizeof(high_data));
  Wire.requestFrom(ATTINY1_HIGH_ADDR, 12);
  while (12 != Wire.available());

  for (int i = 0; i < 12; i++) {
    high_data[i] = Wire.read();
  }
  delay(10);
}

void getLow8SectionValue(void)
{
  memset(low_data, 0, sizeof(low_data));
  Wire.requestFrom(ATTINY2_LOW_ADDR, 8);
  while (8 != Wire.available());

  for (int i = 0; i < 8 ; i++) {
    low_data[i] = Wire.read(); // receive a byte as character
  }
  delay(10);
}

void checkWaterLevel()
{
  int sensorvalue_min = 250;
  int sensorvalue_max = 255;
  int low_count = 0;
  int high_count = 0;
 
    uint32_t touch_val = 0;
    uint8_t trig_section = 0;
    low_count = 0;
    high_count = 0;
    getLow8SectionValue();
    getHigh12SectionValue();

    Serial.println("low 8 sections value = ");
    for (int i = 0; i < 8; i++)
    {
      Serial.print(low_data[i]);
      Serial.print(".");
      if (low_data[i] >= sensorvalue_min && low_data[i] <= sensorvalue_max)
      {
        low_count++;
      }
      if (low_count == 8)
      {
        Serial.print("      ");
        Serial.print("PASS");
      }
    }
    Serial.println("  ");
    Serial.println("  ");
    Serial.println("high 12 sections value = ");
    for (int i = 0; i < 12; i++)
    {
      Serial.print(high_data[i]);
      Serial.print(".");

      if (high_data[i] >= sensorvalue_min && high_data[i] <= sensorvalue_max)
      {
        high_count++;
      }
      if (high_count == 12)
      {
        Serial.print("      ");
        Serial.print("PASS");
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

    while (touch_val & 0x01)
    {
      trig_section++;
      touch_val >>= 1;
    }
    Serial.print("water level = ");
    Serial.print(trig_section * 5);
    Serial.println("% ");
    Serial.println(" ");
    Serial.println("*********************************************************");
    delay(1000);
  
}