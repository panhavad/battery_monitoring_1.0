#define BLYNK_PRINT Serial
#define BAT1_PIN V0
#define WARNING_VOL_LEVEL_PIN V5
#define WARNING_INTERVAL_PIN V6

#define samplingInterval 20
#define printInterval 1000

#include <SPI.h>
#include <Ethernet.h>
#include <BlynkSimpleEthernet.h>
#include <EEPROM.h>

char auth[] = "dNLW3Q96qY_M4Em16CHrQiU6Sz0S4lAA";
float vout = 0.0;
float R1 = 220000.0;
float R2 = 20000.0;
int analog_value_origin[4];
int analog_value_caled[4];
float vin_res[4];
int analog_pin[] = {16, 17, 18, 19};
int virtual_bat_pin[] = {V0, V1, V2, V3};
int arr_pin_len = sizeof(analog_pin)/sizeof(int);

int interval = 0;
int warning_interval_value = 10;
int warning_vol_level = 0;
int start_indicator = 1; 

IPAddress server_ip (10, 254, 253, 248);

// Mac address should be different for each device in your LAN
byte arduino_mac[] = { 0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress arduino_ip ( 10,   0,   16,  11);
IPAddress dns_ip     (  8,   8,   8,   8);
IPAddress gateway_ip ( 10,   0,   0,   1);
IPAddress subnet_mask(255, 0, 0, 0);

#define W5100_CS  10
#define SDCARD_CS 4

void setup(){
  for (int i = 0; i < arr_pin_len; i += 1) {
    pinMode(analog_pin[i], INPUT);
  }
  pinMode(SDCARD_CS, OUTPUT);
  digitalWrite(SDCARD_CS, HIGH); // Deselect the SD card

  Serial.begin(9600);
  
   Blynk.begin("dNLW3Q96qY_M4Em16CHrQiU6Sz0S4lAA", server_ip, 8080, arduino_ip, dns_ip, gateway_ip, subnet_mask, arduino_mac);
}

 BLYNK_WRITE(V5){   
     warning_vol_level = param.asInt(); // Gets the value stored in V2 as an integer
 }
 BLYNK_WRITE(V6){   
     warning_interval_value = param.asInt(); // Gets the value stored in V2 as an integer
 }

void loop(){
  
   Blynk.run();

  if (start_indicator == 1){
    warning_vol_level  = EEPROM.read(0);
    warning_interval_value = EEPROM.read(1);
    
     Blynk.virtualWrite(WARNING_VOL_LEVEL_PIN, warning_vol_level);
     Blynk.virtualWrite(WARNING_INTERVAL_PIN, warning_interval_value);
    
    start_indicator = 0;
    Serial.print("EEPROM WARNING_VOL_LEVEL_PIN ---- ");
    Serial.println(warning_vol_level);
    Serial.print("EEPROM WARNING_INTERVAL_PIN ---- ");
    Serial.println(warning_interval_value);
  }
  
  // read the value at analog input
//  analog_2_value = analogRead(analog_2);
//  analog_3_value = analogRead(analog_3);
//  Serial.println("++++++++++");
//  Serial.println(arr_pin_len);
  for (int i = 0; i < arr_pin_len; i += 1) {
    analog_value_origin[i] = analogRead(analog_pin[i]);
//    Serial.print(analog_pin[i]);
//    Serial.print("------------------");
//    Serial.println(analog_value_caled[i]);
  }
  for (int i = 0; i < arr_pin_len; i += 1) {
    if (i == 0){
      analog_value_caled[i] = analog_value_origin[i];
    }else{
      analog_value_caled[i] = analog_value_origin[i] - analog_value_origin[i - 1];
    }
    analog_value_caled[i] >= 239 ? analog_value_caled[i] = 239 : analog_value_caled[i] = analog_value_caled[i]; //if the value exist 240 analog value
    analog_value_caled[i] <= 0 ? analog_value_caled[i] = 0 : analog_value_caled[i] = analog_value_caled[i]; //if the value exist 240 analog value
  }
  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  
  if (millis() - samplingTime > samplingInterval)
  {
    for (int i = 0; i < arr_pin_len; i += 1) {
      vin_res[i] = vcal(analog_value_caled[i]);
  }
    
//    vin_3 = vcal(analog_3_value);
    
    samplingTime = millis();
  }
  if (millis() - printTime > printInterval)  //Every 800 milliseconds, print a numerical, convert the state of the LED indicator
  {
    for (int i = 0; i < arr_pin_len; i += 1) {
      Serial.print("ANALOG -- ");
      Serial.print(i + 16);
      Serial.print(" Voltage: ");
      Serial.println(vin_res[i],2);
    }

    EEPROM.update(0, warning_vol_level);
    EEPROM.update(1, warning_interval_value);

    for (int i = 0; i < arr_pin_len; i += 1) {
       Blynk.virtualWrite(virtual_bat_pin[i], vin_res[i]);
    }

     Blynk.virtualWrite(V4, interval);

    Serial.println(warning_vol_level);

    float minVol = getMin(vin_res, arr_pin_len);

    Serial.print("Min Value");
    Serial.println(minVol);
    
    if (minVol <= warning_vol_level){
      interval += 1;
        if (interval == warning_interval_value){
           Blynk.notify("Battery Warning");
          interval = 0;
        }
      }else{
        interval = 0;
      }
      
      printTime = millis();
    }
}

float vcal(float value){
  float cal_res = 0.0;
  
  vout = (value * 5.0) / 1024.0; // see text
  cal_res = vout / (R2/(R1+R2));
  return cal_res;
}

float getMin(float* array, int size)
{
  int minimum = array[0];
  for (int i = 0; i < size; i++)
  {
    if (array[i] < minimum) minimum = array[i];
  }
  return minimum;
}
