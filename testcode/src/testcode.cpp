#include "Particle.h" 
#include "Wire.h"

SYSTEM_MODE(MANUAL);

const int THRESHOLD = 100;
const int ATTINY1_HIGH_ADDR = 0x78;
const int ATTINY2_LOW_ADDR = 0x77;

unsigned char low_data[8] = {0};
unsigned char high_data[12] = {0};

void check();
void getHigh12SectionValue();
void getLow8SectionValue();

void setup() {
  Wire.begin();
  Serial.begin(9600);
}

void loop() {
  check();
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