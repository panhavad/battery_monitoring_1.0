/////////////////////////
// AUTHOR : AHIRU SAN  //
// DATE : 01 NOVE 2019 //
/////////////////////////


//hardware related
#include <SPI.h> //library for ethernet shield
#include <Ethernet.h> //library for ethernet shield
#include <BlynkSimpleEthernet.h> //BLYNK communication using ethernet
#include <EEPROM.h> //memory that store data even restart

//BLYNK related
#define WARNING_VOL_LEVEL_PIN V5
#define WARNING_INTERVAL_PIN V6

//asyncronization related
#define samplingInterval 20
#define printInterval 1000
#define sampleLoad 50

//arduino ethernet shield related
#define W5100_CS  10
#define SDCARD_CS 4


float R1 = 220000.0; //first resistor value on voltage devider
float R2 = 20000.0; //second resistor value on voltage devider

float voltage_in_res[4]; //array of calculated voltage massure from battery (final result)
int analog_value_origin[4]; //array of raw analog value from arduino pin 0 - 1024 (series massurement)
int analog_value_calculated[4]; //array of raw analog value calculated isolate the messurement from series
int analog_pin[] = {16, 17, 18, 19}; //array of analog pin that are using (A2 - A5)
int arr_pin_len = sizeof(analog_pin)/sizeof(int); //len of the array analog pin
int virtual_battery_pin[] = {V0, V1, V2, V3}; //array of battery pin in BLYNK

int warning_interval_counter = 0; //counter when condition of warning matched
int warning_interval_value = 10; //warning counter set, will be change by user in BLYNK
int warning_voltage_level = 0; //voltage of each battery that set for warning (minimum)
int start_indicator = 1; //indicator that said the board is just start power on

//BLYNK server configuration
// char auth[] = "dNLW3Q96qY_M4Em16CHrQiU6Sz0S4lAA"; //BLYNK project token
IPAddress server_ip (192, 168, 7, 100); //BLYNK server IP

//arduino ethernet confirguration (network)
byte arduino_mac[] = { 0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED }; // MAC address should be different for each device in your LAN
IPAddress arduino_ip ( 10,   0,   16,  11); //static IP for this arduino
IPAddress dns_ip     (  8,   8,   8,   8);
IPAddress gateway_ip ( 10,   0,   0,   1);
IPAddress subnet_mask(255, 0, 0, 0);


void setup() {

  for (int i = 0; i < arr_pin_len; i += 1) {
    //enable reading mode on analog pin
    pinMode(analog_pin[i], INPUT);
  }

  Serial.begin(9600); //enable serial mode on bitrate at 9600

  //some conflict in BLYNK configuration so have to place raw token string here
  Blynk.begin("lWoWHfQ-jUH0byk8DzUCwbeDhay-OCp2", server_ip, 8080, arduino_ip, dns_ip, gateway_ip, subnet_mask, arduino_mac); //establish connection to BLYNK server

}

BLYNK_WRITE(V5) {
  //gets the value stored in V5 as an integer
  warning_voltage_level = param.asInt();
}

BLYNK_WRITE(V6) {
  //gets the value stored in V2 as an integer 
  warning_interval_value = param.asInt(); 
}

void loop() {

  //timing variable for asyncronus execution
  static unsigned long samplingTime = millis(); //sample processing time
  static unsigned long printTime = millis(); //print or output time
  
  Blynk.run(); //start BLYNK communication

  if (start_indicator == 1) { //is this board just power up?
    warning_voltage_level  = EEPROM.read(0); //read value from EEPROM on address 0
    warning_interval_value = EEPROM.read(1); //read value from EEPROM on address 1
    
    //write the value that get from EEPROM to BLYNK
    Blynk.virtualWrite(WARNING_VOL_LEVEL_PIN, warning_voltage_level);
    Blynk.virtualWrite(WARNING_INTERVAL_PIN, warning_interval_value);
    
    start_indicator = 0; //reset board state
  }
  
  //read data from analog pin from A2 - A5
  for (int i = 0; i < arr_pin_len; i += 1) {
    analog_value_origin[i] = analogRead(analog_pin[i]); //orginal value raw from alanalog pin
  }

  //find the arverage from 20 collected samples value
  for (int i = 0; i < arr_pin_len; i++ ) {

    for (int i_pin = 0; i_pin < arr_pin_len; i_pin++) {
      int buffer[sampleLoad], temp; //buffer, temp ready for ordering value in order
      unsigned long int avgValue; //final result of the loop (arverage value of 20 samples)

        //read data raw data from analog pin 0 - 1024
        for (int i = 0; i < sampleLoad; i++) {
          buffer[i]=analogRead(analog_pin[i_pin]); //read 20 times of value on only one pin (getting sample)
        }
        
        //ordering the collected sameple in ASC order
        for (int i = 0; i < sampleLoad - 1; i++) {
          for (int j=i+1;j<sampleLoad;j++) {
            if (buffer[i]>buffer[j]) {
              temp=buffer[i];
              buffer[i]=buffer[j];
              buffer[j]=temp;
            }
          }
        } //ordering done     
      
      avgValue = 0; //make sure that the initail average value is 0

      //ignore the first 5 small value then sum the rest together
      for (int i = 5; i < sampleLoad - 5; i++) {
        avgValue+=buffer[i]; //sum each buffer value together
      }
      analog_value_origin[i_pin]=(float)avgValue/(sampleLoad - 10); //find the average result
    }
  
    //isolate the the massurement value from series connection & and refine the value
    if (i == 0) {//if one first battery value
      analog_value_calculated[i] = analog_value_origin[i]; //value dont change
    }else if (i == 3) {//if the one last battery value
      analog_value_calculated[i] = analog_value_origin[i] - analog_value_origin[i - 1] - 20; //throught out the whole massurement there are at most 20 raw value extra
    }else {
      analog_value_calculated[i] = analog_value_origin[i] - analog_value_origin[i - 1]; //isolate the massurement value for only one spacific battery
    }
    //ensure the final value is not exit what we expect
    analog_value_calculated[i] >= 239 ? analog_value_calculated[i] = 239 : analog_value_calculated[i] = analog_value_calculated[i]; //if the value exit 240 analog value
    analog_value_calculated[i] <= 0 ? analog_value_calculated[i] = 0 : analog_value_calculated[i] = analog_value_calculated[i]; //if the value lower them 0 analog value
  }
  //sample processing timer
  if (millis() - samplingTime > samplingInterval) {
    
    for (int i = 0; i < arr_pin_len; i += 1) {
      voltage_in_res[i] = voltageCalculation(analog_value_calculated[i]); //convert from raw analog input to voltage
    }

    samplingTime = millis(); //reset timer
  }

  //output timer
  if (millis() - printTime > printInterval) {//Every 800 milliseconds, print a numerical, convert the state of the LED indicator
    
    //DEBUG
    // for (int i = 0; i < arr_pin_len; i += 1) {
    //   Serial.print("ANALOG -- ");
    //   Serial.print(i + 16);
    //   Serial.print(" Voltage: ");
    //   Serial.println(voltage_in_res[i],2);
    // }

    //update the current value to EEPROM
    EEPROM.update(0, warning_voltage_level);
    EEPROM.update(1, warning_interval_value);

    //write value of the massured voltage of each battery of each battery vitual pin in BLYNK
    for (int i = 0; i < arr_pin_len; i += 1) {
      Blynk.virtualWrite(virtual_battery_pin[i], voltage_in_res[i]); //write to voltage value to virtual pin
    }

    Blynk.virtualWrite(V4, warning_interval_counter); //write warning counter to BLYNK virtual pin V4

    float minVol = getMin(voltage_in_res, arr_pin_len); //find the minimum voltage value among all batteries
    
    //validate the current voltage and minimum 
    if (minVol <= warning_voltage_level) {
      warning_interval_counter += 1;
        if (warning_interval_counter == warning_interval_value) { // when the counter reach the limit
          Blynk.notify("Battery is warning, Please check!"); //send warning to app
          warning_interval_counter = 0; //reset the counter
        }
      }else{
        warning_interval_counter = 0; //reset the counter
      }
      
      printTime = millis(); //reset timer
    }
}

//convert raw analog value to voltage
float voltageCalculation(float value) {
  float calculation_result = 0.0, voltage_output = 0.0;
  
  voltage_output = (value * 5.0) / 1024.0; //devider base on the voltage of the arduino
  calculation_result = voltage_output / (R2/(R1+R2)); //voltage devider
  
  return calculation_result;
}

//find the minimum voltage value
float getMin(float* array, int size) {
  int minimum = array[0];

  for (int i = 0; i < size; i++) {
    if (array[i] < minimum) {
      minimum = array[i];
    }
  }
  return minimum;
}
