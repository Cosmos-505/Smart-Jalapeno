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
#include "Neopixel.h"
#include "Colors.h"


// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(SEMI_AUTOMATIC);

//NEOPIXEL SPECTRUM LIGHT
const int pixelPin = D2;
Adafruit_NeoPixel pixels (39, SPI1, WS2812B); 
const int NUM_PIXELS = 39;
int first;
int last;
int color;
int segment;
void pixelFill(int first, int last, int color);


//VEML LIGHT SENSOR
Adafruit_VEML7700 veml;
const int VEMLAddr = 0x10;

//STEPPER MOTOR
Stepper stepper (2048, D16, D15, D17, D18);
IoTTimer feedTimer;

// DS WATER TEMP SENSOR
const int MAXRETRY =4;
const uint32_t msSAMPLE_INTERVAL = 2500;
const uint32_t msMETRIC_PUBLISH  = 10000;



// Sets Pin D3 as data pin and the only sensor on bus
DS18B20  ds18b20(D3, true); 
int DSPin = D3;
OneWire ds(DSPin);
void getTemp();
void publishData();

char     szInfo[64];
float   celsius;
float   fahrenheit;
uint32_t msLastMetric;
uint32_t msLastSample; 











// setup() runs once, when the device is first turned on
void setup() {

//NEO PIXEL STRIP set all pixels to white
  pixels.begin();
  pixels.setBrightness(20);
  pixelFill(0,39,white);
  pixels.show();

  //Serial Print
  Serial.begin(9600);
  waitFor(Serial.isConnected,10000);

//DS Water Temp SENSOR
  pinMode(D3,INPUT_PULLUP);

// Stepper motor and Feed timer
  stepper.setSpeed(15);
  feedTimer.startTimer(10000);


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
  feedTimer.startTimer(10000);
}


  //LIGHT VALUES
  Serial.printf("Lux: ");
  Serial.println(veml.readLux());
  Serial.print("White: ");
  Serial.println(veml.readWhite());
  Serial.print("Raw ALS: ");
  Serial.println(veml.readALS());

  uint16_t irq = veml.interruptStatus();
  if (irq & VEML7700_INTERRUPT_LOW)
  {
    Serial.printf("** Low threshold");
  }
  if (irq & VEML7700_INTERRUPT_HIGH)
  {
    Serial.printf("** High threshold");
  }
  delay(1000);


  //DS TEMP DATA
  if (millis() - msLastSample >= msSAMPLE_INTERVAL){
    getTemp();
  }

  if (millis() - msLastMetric >= msMETRIC_PUBLISH){
    Serial.printf("GONNA PUBLISH HERE.\n");
    publishData();
  }
}












// PUBLISH WATER TEMP DATA GO HERE
void publishData(){
  sprintf(szInfo, "%2.2f", fahrenheit);
  Particle.publish("dsTmp", szInfo, PRIVATE);
  msLastMetric = millis();
}

//GET TEMP FROM DS WATER TEMP
void getTemp(){
 
  float tempCelsius = ds18b20.getTemperature();
  Serial.printf("Celsius is %.2f\n", tempCelsius);
  float tempFahrenheit = ds18b20.convertToFahrenheit(tempCelsius);
  Serial.printf("Water Temperature is %.2F\n", tempFahrenheit);
  }

void  pixelFill (int first, int last, int color) {
   int i;
   for (i = first; i<= last; i++) {
      pixels.setPixelColor(i,white);
      pixels.show();
   }
}
