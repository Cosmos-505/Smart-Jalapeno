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


// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(SEMI_AUTOMATIC);

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
DS18B20  ds18b20(D2, true); 
OneWire ds(D2);
void getTemp();
void publishData();

char     szInfo[64];
float   celsius;
float   fahrenheit;
uint32_t msLastMetric;
uint32_t msLastSample; 





// setup() runs once, when the device is first turned on
void setup() {

//DS Water Temp SENSOR
  pinMode(D2,INPUT_PULLUP);

// Stepper motor and Feed timer
  stepper.setSpeed(15);
  feedTimer.startTimer(10000);

  //Serial Print
  Serial.begin(9600);
  waitFor(Serial.isConnected,10000);


//VEML LIGHT SENSOR STARTUPS
  Serial.printf("Adafruit VEML7700 Test");

  if (!veml.begin())
  {
    Serial.printf("Sensor not found");
    while (1)
      ;
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













void publishData(){
  sprintf(szInfo, "%2.2f", fahrenheit);
  Particle.publish("dsTmp", szInfo, PRIVATE);
  msLastMetric = millis();
}

void getTemp(){

  float tempCelsius = ds18b20.getTemperature();
  float tempFahrenheit = ds18b20.convertToFahrenheit(tempCelsius);
    Serial.printf("Water Temperature is %.2F\n", tempFahrenheit);
  }

