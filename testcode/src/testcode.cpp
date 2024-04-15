#include "Particle.h" 
#include "Wire.h"
#include "Stepper.h"
#include "Button.h"
#include "DS18B20.h"
#include "math.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"
#include "Adafruit_MQTT/Adafruit_MQTT.h"
#include "credentials.h"


SYSTEM_MODE(MANUAL);

/************ Global State (you don't need to change this!) ***   ***************/ 
TCPClient TheClient; 

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details. 
Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY); 
void MQTT_connect();
bool MQTT_ping();
Adafruit_MQTT_Publish mqtttemp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/watertemp");



const int      MAXRETRY          = 4;
const uint32_t msSAMPLE_INTERVAL = 2500;
const uint32_t msMETRIC_PUBLISH  = 10000;


const int16_t dsData = D3;
// Sets Pin D3 as data pin and the only sensor on bus
DS18B20  ds18b20(dsData, true); 
void getTemp();

char     szInfo[64];
double   celsius;
double   fahrenheit;
uint32_t msLastMetric;
uint32_t msLastSample;

unsigned int last, lastTime;

const int THRESHOLD = 100;
const int ATTINY1_HIGH_ADDR = 0x78;
const int ATTINY2_LOW_ADDR = 0x77;

unsigned char low_data[8] = {0};
unsigned char high_data[12] = {0};

void check();
void getHigh12SectionValue();
void getLow8SectionValue();

Stepper stepper (2048, D10, D11, D12, D13);
Button button(D5, INPUT_PULLDOWN);
const int ButtonPin = D5;


void setup() {

  // Connect to Internet but not Particle Cloud
WiFi.on();
WiFi.connect();
  while(WiFi.connecting()) {
    Serial.printf(".");
  }
  Wire.begin();
  Serial.begin(9600);

  pinMode(D5, INPUT_PULLDOWN);
  stepper.setSpeed(10);


}

void loop() {

//PUBLISHING TO ADAFRUIT
MQTT_connect();
MQTT_ping();


 

  //check();
  getTemp();

if((millis()-lastTime > 6000)) {
   if (mqtt.Update())
mqtttemp.publish(fahrenheit);
}
//FEED BUTTON
  int buttonstate;
  buttonstate = digitalRead(ButtonPin);
    if (buttonstate ==1) {
       stepper.step(1);
      Serial.printf("Feeding fish\n");
      Serial.printf("Print %i\n",ButtonPin);
    }


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

void check() {
  int sensorvalue_min = 250;
  int sensorvalue_max = 255;
  int low_count = 0;
  int high_count = 0;

  while (1) {
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
    delay(1000);
  }
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


// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;
 
  // Return if already connected.
  if (mqtt.connected()) {
    return;
  }
 
  Serial.print("Connecting to MQTT... ");
 
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.printf("Error Code %s\n",mqtt.connectErrorString(ret));
       Serial.printf("Retrying MQTT connection in 5 seconds...\n");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds and try again
  }
  Serial.printf("MQTT Connected!\n");
}

bool MQTT_ping() {
  static unsigned int last;
  bool pingStatus;

  if ((millis()-last)>120000) {
      Serial.printf("Pinging MQTT \n");
      pingStatus = mqtt.ping();
      if(!pingStatus) {
        Serial.printf("Disconnecting \n");
        mqtt.disconnect();
      }
      last = millis();
  }
  return pingStatus;
}