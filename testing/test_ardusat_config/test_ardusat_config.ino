/*
 * EEPROM Write
 *
 * Stores values read from analog input 0 into the EEPROM.
 * These values will stay in the EEPROM when the board is
 * turned off and may be retrieved later by another sketch.
 */

//#define TEST_ACCEL

#include <Arduino.h>
#include <EEPROM.h>
#include <I2C_add.h>
#include <Wire.h>
#include <I2CComm.h>
#include <nanosat_message.h>
#include <OnboardCommLayer.h>
#include <SAT_AppStorage.h>
#include <MemoryFree.h>

#include <SAT_Accel.h>
//#include <SAT_Geiger.h>
#include <SAT_Gyro.h>
#include <SAT_InfraTherm.h>
#include <SAT_Lum.h>
#include <SAT_Mag.h>
//#include <SAT_Spectro.h>
#include <SAT_Temp.h>

SAT_Accel * accel;
//SAT_Geiger * geiger;
SAT_Gyro * gyro;
SAT_InfraTherm * irtherm;
SAT_Lum * lum;
SAT_Mag * mag;
SAT_Temp * temp;

#define NODE_ADDRESS  I2C_ADD_ARD1    // begin as experiment arduino 1

// I2C Scanner, to find sensors around
void scanI2C();

void setup()
{
  int beforeRam = 0;
  int afterRam = 0;
  
  Serial.begin(115200);
  Serial.print("ArduSat-Dev config test script (freeram=");
  Serial.print(freeMemory());
  Serial.println(")");

  Serial.print("- writing node address [0x");
  Serial.print(NODE_ADDRESS,HEX);
  Serial.write("] in EEPROM.");
  EEPROM.write(0x00,NODE_ADDRESS);
  Serial.println(" done");

  Serial.print("- beginning I2C. ");
  
  beforeRam = freeMemory();  
  afterRam = freeMemory();
  Serial.print(afterRam-beforeRam);
  
  I2CComm.begin();
  Serial.println(" done");

  Serial.print("- check node address. 0x");
  byte local_addr = I2CComm.getAddress();
  Serial.println(local_addr,HEX);

  Serial.print("- scanning I2C :");
  scanI2C();
  Serial.println();
  
  // *** SYSTEM/SDK ***

  Serial.println("- testing SDK ram charge.");

  Serial.print("  I2CComm\t\t");
  beforeRam = freeMemory();
  I2C_CommManager * testI2C = new I2C_CommManager();
  testI2C->begin();
  afterRam = freeMemory();
  Serial.println(afterRam-beforeRam);

  Serial.print("  OnboardCommLayer\t\t");
  beforeRam = freeMemory();
  OnboardCommLayer * testOBCL = new OnboardCommLayer();
  afterRam = freeMemory();
  afterRam -= sizeof(nanosat_message_t);  // used in the includes itself
  Serial.println(afterRam-beforeRam);
  
  Serial.print("  SAT_AppStorage\t\t");
  char message[] = "test";  // not included in the ram calculation
  beforeRam = freeMemory();
  SAT_AppStorage * testAppStorage = new SAT_AppStorage();
  testAppStorage->send(message);
  afterRam = freeMemory();
  Serial.println(afterRam-beforeRam);
  
  
  // *** SENSORS ***
  
  Serial.println("- testing sensor ram charge.");

  Serial.print("  SAT_Accel\t\t");
  beforeRam = freeMemory();
  int16_t * accel_values = new int16_t[3];
  accel = new SAT_Accel;
  afterRam = freeMemory();
  Serial.println(afterRam-beforeRam);

  Serial.print("  SAT_Gyro\t\t");
  beforeRam = freeMemory();
  int * gyro_values = new int[3];
  gyro = new SAT_Gyro;
  gyro->init(I2C_ADD_GYR);
  afterRam = freeMemory();
  Serial.println(afterRam-beforeRam);

  Serial.print("  SAT_InfraTherm\t");
  beforeRam = freeMemory();
  int16_t * irtherm_values = new int16_t[1];
  irtherm = new SAT_InfraTherm;
  afterRam = freeMemory();
  Serial.println(afterRam-beforeRam);

  Serial.print("  SAT_Lum\t\t");
  beforeRam = freeMemory();
  int32_t * lum_values = new int32_t[1];
  lum = new SAT_Lum(1);
  afterRam = freeMemory();
  Serial.println(afterRam-beforeRam);

  Serial.print("  SAT_Mag\t\t");
  beforeRam = freeMemory();
  int16_t * mag_values = new int16_t[3];
  mag = new SAT_Mag;
  afterRam = freeMemory();
  Serial.println(afterRam-beforeRam);

  Serial.print("  SAT_Temp\t\t");
  beforeRam = freeMemory();
  int16_t * temp_values = new int16_t[1];
  temp = new SAT_Temp(1);
  afterRam = freeMemory();
  Serial.println(afterRam-beforeRam);


  unsigned long int beforeTime;
  unsigned long int afterTime;
  double freq;
  

  Serial.println("- testing sensor freq.");

  Serial.print("  SAT_Accel\t\t");
  beforeTime = millis();
  for (int i=0; i<100; i++) {
    accel->readAccel(accel_values);
  }
  afterTime = millis();
  freq = 1000/(((double)(afterTime-beforeTime))/100);  // for Hz
  Serial.println(freq);
  
  Serial.print("  SAT_Gyro\t\t");
  beforeTime = millis();
  for (int i=0; i<100; i++) {
    gyro->readGyroRaw(gyro_values);
  }
  afterTime = millis();
  freq = 1000/(((double)(afterTime-beforeTime))/100);  // for Hz
  Serial.println(freq);

  Serial.print("  SAT_InfraTherm\t");
  beforeTime = millis();
  for (int i=0; i<100; i++) {
    irtherm_values[0] = irtherm->getRawTemp();
  }
  afterTime = millis();
  freq = 1000/(((double)(afterTime-beforeTime))/100);  // for Hz
  Serial.println(freq);

  Serial.print("  SAT_Lum\t\t");
  beforeTime = millis();
  for (int i=0; i<100; i++) {
    lum_values[0] = lum->getFullLuminosity();
  }
  afterTime = millis();
  freq = 1000/(((double)(afterTime-beforeTime))/100);  // for Hz
  Serial.println(freq);

  Serial.print("  SAT_Mag\t\t");
  beforeTime = millis();
  for (int i=0; i<100; i++) {
    mag_values[0] = mag->readx();
    mag_values[1] = mag->ready();
    mag_values[2] = mag->readz();
  }
  afterTime = millis();
  freq = 1000/(((double)(afterTime-beforeTime))/100);  // for Hz
  Serial.println(freq);

  Serial.print("  SAT_Temp\t\t");
  beforeTime = millis();
  for (int i=0; i<100; i++) {
    temp_values[0] = temp->getRawTemp();
  }
  afterTime = millis();
  freq = 1000/(((double)(afterTime-beforeTime))/100);  // for Hz
  Serial.println(freq);
  
  Serial.print("end of test. ");
  Serial.println(freeMemory());
}



void loop()
{
}

// *** I2C SCANNER ***
// -------------------
// adapted frop http://playground.arduino.cc/Main/I2cScanner
// This sketch tests the standard 7-bit addresses
// Devices with higher bit address might not be seen properly.

#define SENSOR_ARRAY_MAX  14

byte i2c_addressArray[] = {
  I2C_ADD_MAG,     // magnetometer
  I2C_ADD_LUX1,    // TSL2561 #1 (bottomplate camera)
  I2C_ADD_LUX2,    // TSL2561 #2 (bottomplate slit)
  I2C_ADD_TMP1,    // temp sensor TMP102 (payload #1)
  I2C_ADD_TMP2,    // temp sensor TMP102 (payload #2)
  I2C_ADD_TMP3,    // temp sensor TMP102 (bottomplate #1)
  I2C_ADD_TMP4,    // temp sensor TMP102 (bottomplate #2)
  I2C_ADD_ACC,     // accelerometer ADXL345
  I2C_ADD_GYR,     // gyros ITG3200
  I2C_ADD_MLX,     // IR thermometer (bottomplate)
  I2C_ADD_ASSV_1,  // master arduino (ArduSat-1)
  I2C_ADD_ASSV_X,  // master arduino (ArduSat-X)
  I2C_ADD_SPECT,   // spectruino arduino
  I2C_ADD_GEIGER,  // uSD card arduino
};

String nameArray[] = {
  "MAG",
  "LUX1",
  "LUX2",
  "TMP1",
  "TMP2",
  "TMP3",
  "TMP4",
  "AXL",
  "GYRO",
  "IRTHERM",
  "MASTER-1",
  "MASTER-X",
  "SPECT",
  "GEIGER"
};

void printSensorName(byte address) {
  for (int i=0; i<SENSOR_ARRAY_MAX; i++) {
    if (address == i2c_addressArray[i]) {
      Serial.print(nameArray[i]);
      return;
    }
  }
  Serial.print("UNKNOWN");
};

void scanI2C()
{
  byte error, address;
  byte nDevices = 0;
  
  for(address = 1; address < 127; address++ ) 
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.write(' ');
      printSensorName(address);
      Serial.print("[0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.write(']');
      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknown error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.print("No I2C devices found\n");
}
