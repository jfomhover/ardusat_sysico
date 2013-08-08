/*
    File :         ardusat_magfield2.ino
    Author :       Jean-Francois Omhover (@jfomhover)
    Last Changed : Aug. 8th 2013
    Description :  pulling SAT_Mag only, based on configurable code of ardusat_configurablepuller.ino
                   values are set to get approx. 365 points of value, on 2.10 rotations around earth
                   
  
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


// **************
// *** CONFIG ***
// **************

//#define DEBUG_MODE              // prints the values in readable format via Serial
#define DEBUG_LED   9           // the pin of the led that will blink at each pool of the sensors
//#define COMM_EMULATION        // to emulate the Comm via SAT_AppStorageEMU, prints out (via Serial) the results of the data that is sent through
#define PULL_DELAY  33000        // data is pooled every PULL_DELAY seconds
//#define LEGACY_SDK              // use sdk BEFORE the integration of I2CComm, on Aug. 7th 2013

//#define POOL_SAT_LUM            // comment to NOT POOL the luminosity values
#define POOL_SAT_MAG            // comment to NOT POOL the magnetometer values
//#define POOL_SAT_TMP            // comment to NOT POOL the temperature values
//#define POOL_INFRATHERM       // comment to NOT POOL the infratherm values

#define OUTPUT_TEXTCSV          // to output in text/csv format
//#define OUTPUT_BINARY         // to output in binary format (see struct below)
#define CHARBUFFER_SPACE  256   // space allocated for the message in ASCII (will be used only if OUTPUT_ASCII is uncommented)

/*
#define I2C_ADD_MAG             0X0E    // magnetometer
#define I2C_ADD_LUX1            0X29    // TSL2561 #1 (bottomplate camera)
#define I2C_ADD_LUX2            0x39    // TSL2561 #2 (bottomplate slit)
#define I2C_ADD_TMP1            0X48    // temp sensor TMP102 (payload #1)
#define I2C_ADD_TMP2            0X49    // temp sensor TMP102 (payload #2)
#define I2C_ADD_TMP3            0X4A    // temp sensor TMP102 (bottomplate #1)
#define I2C_ADD_TMP4            0X4B    // temp sensor TMP102 (bottomplate #2)
#define I2C_ADD_ACC             0X53    // accelerometer ADXL345
#define I2C_ADD_GYR             0X69    // gyros ITG3200
#define I2C_ADD_MLX             0X51    // IR thermometer (bottomplate)
*/

// ***************
// *** HEADERS ***
// ***************

#include <Arduino.h>
#include <Wire.h>//for I2C
#include <nanosat_message.h>
#include <I2C_add.h>
#include <I2CComm.h>

#ifdef COMM_EMULATION
#include <SAT_AppStorageEMU.h>
#else
#include <EEPROM.h>
#include <OnboardCommLayer.h>
#include <SAT_AppStorage.h>
#endif /* COMM_EMULATION */

    // BELOW, ADD THE HEADERS YOU NEED
#ifdef POOL_SAT_LUM
#include <SAT_Lum.h>
#endif
#ifdef POOL_SAT_MAG
#include <SAT_Mag.h>
#endif
#ifdef POOL_SAT_TMP
#include <SAT_Temp.h>
#endif
#ifdef POOL_INFRATHERM
#include <SAT_InfraTherm.h>
#endif

// ************************
// *** API CONSTRUCTORS ***
// ************************

#ifdef COMM_EMULATION
SAT_AppStorageEMU store;
#else
SAT_AppStorage store;
#endif

    // BELOW, ADD THE CONSTRUCTORS YOU NEED
#ifdef POOL_SAT_LUM
SAT_Lum tsl_one(1);                // one is on the front
SAT_Lum tsl_two(2);                // the other... bottomplate ?
#endif

#ifdef POOL_SAT_MAG
SAT_Mag mag;
#endif

#ifdef POOL_SAT_TMP
SAT_Temp tmp_sensor1(1);        // previously was "payload"
SAT_Temp tmp_sensor2(2);
SAT_Temp tmp_sensor3(3);    // previously was "bottomplate"
SAT_Temp tmp_sensor4(4);
#endif

#ifdef POOL_INFRATHERM
SAT_InfraTherm mlx;
#endif

    // SETUP OF THE SENSORS NEEDED (constructors mainly)
void setupSensors() {
  
  // *** SAT_MAG SETUP ***
#ifdef POOL_SAT_MAG
  mag.configMag();          // turn the MAG3110 on 
#endif

#ifdef POOL_SAT_LUM
  // *** SAT_LUM SETUP ***
#if defined(DEBUG_MODE)
#ifndef LEGACY_SDK
  if (tsl_one.begin()) {
#else // LEGACY_SDK
  if (tsl_one.begin(0x42)) {
#endif // LEGACY_SDK
    Serial.println("Found tsl_one");
  } else {
    Serial.println("No tsl_one");
//    while (1);
  }
#ifndef LEGACY_SDK
  if (tsl_two.begin()) {
#else // LEGACY_SDK
  if (tsl_two.begin(0x42)) {
#endif // LEGACY_SDK
    Serial.println("Found tsl_two");
  } else {
    Serial.println("No tsl_two");
//    while (1);
  }
#else
#ifndef LEGACY_SDK
  tsl_one.begin();
  tsl_two.begin();
#else  // LEGACY_SDK
  tsl_one.begin(1);
  tsl_two.begin(1);
#endif // LEGACY_SDK
#endif // DEBUG_MODE

  //tsl.setGain(SAT_Lum_GAIN_0X);         // set no gain (for bright situtations)
  tsl_one.setGain(SAT_Lum_GAIN_16X);      // set 16x gain (for dim situations)
  tsl_two.setGain(SAT_Lum_GAIN_16X);      // set 16x gain (for dim situations)

  //tsl.setTiming(SAT_Lum_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)
  tsl_one.setTiming(SAT_Lum_INTEGRATIONTIME_101MS);  // medium integration time (medium light)
  tsl_two.setTiming(SAT_Lum_INTEGRATIONTIME_101MS);  // medium integration time (medium light)
  //tsl.setTiming(SAT_Lum_INTEGRATIONTIME_402MS);  // longest integration time (dim light)

#endif // POOL_SAT_LUM


}

// ********************************
// *** SENSOR POOLING FUNCTIONS ***
// ********************************

#define AVAILABLE_SAT_LUM  0x01
#define AVAILABLE_SAT_MAG  0x02
#define AVAILABLE_SAT_TMP  0x04
#define AVAILABLE_SAT_INFRATHERM  0x08

struct _dataStruct {
  char header;          // don't touch that, or adapt it, whatever ^^
  byte availableValues;
  long int ms;          // i use that as an id

          // BELOW, ADD THE SENSOR VALUES YOU NEED
#ifdef POOL_SAT_LUM
  uint16_t tsl_one_values[2];
  uint16_t tsl_two_values[2];
#endif
  
#ifdef POOL_SAT_MAG
  int16_t mag_values[3]; 
#endif

#ifdef POOL_SAT_TMP
  int16_t temp_values[4];
#endif // POOL_SAT_LUM

#ifdef POOL_INFRATHERM
  int16_t infrat_value;
#endif // POOL_INFRATHERM
} data;

#define DATA_LENGTH  sizeof(struct _dataStruct)

// blanks everything (to be REALLY sure)
void resetValues() {
  byte * t_bytePtr = (byte *)&data;
  for (int i=0; i<DATA_LENGTH; i++) {
    t_bytePtr[i] = 0;
  }
}

// pools the values needed by the experiment
// here : the magnetometer
void poolValues() {
  resetValues();
  data.header = '#';            // used for parsing (even if the struct has a fixed size)
  
  data.availableValues = 0
#ifdef POOL_SAT_LUM
                         | AVAILABLE_SAT_LUM
#endif
#ifdef POOL_SAT_MAG
                         | AVAILABLE_SAT_MAG
#endif
#ifdef POOL_SAT_TMP
                         | AVAILABLE_SAT_TMP
#endif
#ifdef POOL_INFRATHERM
                         | AVAILABLE_SAT_INFRATHERM
#endif
                         ;
  
  data.ms = millis();

      // BELOW, ADD THE POOLING FUNCTIONS YOU NEED
#ifdef POOL_SAT_LUM
  data.tsl_one_values[0] = tsl_one.getLuminosity(SAT_Lum_VISIBLE);
  data.tsl_one_values[1] = tsl_one.getLuminosity(SAT_Lum_INFRARED);
  data.tsl_two_values[0] = tsl_two.getLuminosity(SAT_Lum_VISIBLE);
  data.tsl_two_values[1] = tsl_two.getLuminosity(SAT_Lum_INFRARED);
#endif

#ifdef POOL_SAT_MAG
  data.mag_values[0] = mag.readx();
  data.mag_values[1] = mag.ready();
  data.mag_values[2] = mag.readz();
#endif

#ifdef POOL_SAT_TMP
  data.temp_values[0] = tmp_sensor1.getRawTemp();
  data.temp_values[1] = tmp_sensor2.getRawTemp();
  data.temp_values[2] = tmp_sensor3.getRawTemp();
  data.temp_values[3] = tmp_sensor4.getRawTemp();
#endif // POOL_SAT_TMP

#ifdef POOL_INFRATHERM
  data.infrat_value = mlx.getRawTemp();
#endif // POOL_INFRATHERM
}

#ifdef OUTPUT_TEXTCSV
char messageBuffer[CHARBUFFER_SPACE];
char messagePreBuffer[16];
int bufferLen;

boolean commitPreMessage() {
  if ((bufferLen + strlen(messagePreBuffer)+1) < CHARBUFFER_SPACE) {
    memcpy(messageBuffer+bufferLen,messagePreBuffer,strlen(messagePreBuffer));
    bufferLen+=strlen(messagePreBuffer);
    return(true);
  } else {
    return(false);
  }
}

//#define PRINTINT(buf,val)  sprintf(buf,"%i;",val)
//#define PRINTFLOAT(buf,val)  sprintf(buf,"%f;",val)  // doesn't work ?!?
//#define PRINTINT(buf,val)  { memset(buf, 0x00, 16); itoa(val, buf, 10); buf[strlen(buf)]=';'; }          // using itoa instead of sprintf gets a 1.5ko smaller prog ^^
//#define PRINTFLOAT(buf,val)  { memset(buf, 0x00, 16); dtostrf(val, 3, 2, buf); buf[strlen(buf)]=';'; }
#define PRINTINT(buf,val)  printPreBufferInt(buf,val)
#define PRINTUINT(buf,val)  printPreBufferUInt(buf,val)

void printPreBufferInt(char * buf, int val) {
  memset(buf, 0x00, 16);
  itoa(val, buf, 10);
  buf[strlen(buf)]=';';
}

void printPreBufferUInt(char * buf, unsigned int val) {
  memset(buf, 0x00, 16);
  utoa(val, buf, 10);
  buf[strlen(buf)]=';';
}

void prepareMessage() {
  memset(messageBuffer, 0, CHARBUFFER_SPACE);
  bufferLen = 0;
  
  PRINTUINT(messagePreBuffer, data.ms);
  if (!commitPreMessage())  return;

      // BELOW, ADD THE POOLING FUNCTIONS YOU NEED
#ifdef POOL_SAT_LUM
  PRINTUINT(messagePreBuffer, data.tsl_one_values[0]);
  if (!commitPreMessage())  return;
  PRINTUINT(messagePreBuffer, data.tsl_one_values[1]);
  if (!commitPreMessage())  return;
  PRINTUINT(messagePreBuffer, data.tsl_two_values[0]);
  if (!commitPreMessage())  return;
  PRINTUINT(messagePreBuffer, data.tsl_two_values[1]);
  if (!commitPreMessage())  return;
#endif //POOL_SAT_LUM

#ifdef POOL_SAT_MAG
  PRINTINT(messagePreBuffer, data.mag_values[0]);
  if (!commitPreMessage())  return;
  PRINTINT(messagePreBuffer, data.mag_values[1]);
  if (!commitPreMessage())  return;
  PRINTINT(messagePreBuffer, data.mag_values[2]);
  if (!commitPreMessage())  return;
#endif // POOL_SAT_MAG

#ifdef POOL_SAT_TMP
  PRINTINT(messagePreBuffer, data.temp_values[0]);
  if (!commitPreMessage())  return;

  PRINTINT(messagePreBuffer, data.temp_values[1]);
  if (!commitPreMessage())  return;

  PRINTINT(messagePreBuffer, data.temp_values[2]);
  if (!commitPreMessage())  return;

  PRINTINT(messagePreBuffer, data.temp_values[3]);
  if (!commitPreMessage())  return;
#endif // POOL_SAT_TMP

#ifdef POOL_INFRATHERM
  PRINTINT(messagePreBuffer, data.infrat_value);
  if (!commitPreMessage())  return;
#endif // POOL_INFRATHERM  

  if ((bufferLen + 2) < CHARBUFFER_SPACE)
    messageBuffer[bufferLen++]='\n';
}
#endif  // OUTPUT_TEXTCSV

#ifdef DEBUG_MODE
void outputValues() {
  Serial.print("*** DEBUG :");
  Serial.print("\tms=");
  Serial.print(data.ms);

      // BELOW, IF YOU'D LIKE SOME DEBUG, ADD THE PRINTING LINES

#ifdef POOL_SAT_LUM
  Serial.print("\tvisible_1=");
  Serial.print(data.tsl_one_values[0]);
  Serial.print("\tIR_1=");
  Serial.print(data.tsl_one_values[1]);
  Serial.print("\tvisible_2=");
  Serial.print(data.tsl_two_values[0]);
  Serial.print("\tIR_2=");
  Serial.print(data.tsl_two_values[1]);
#endif

#ifdef POOL_SAT_MAG
  Serial.print("\tX=");
  Serial.print(data.mag_values[0]);
  Serial.print("\tY=");
  Serial.print(data.mag_values[1]);
  Serial.print("\tZ=");
  Serial.print(data.mag_values[2]);
#endif

#ifdef POOL_SAT_TMP
  Serial.print("\ttemp_1=");
  Serial.print(data.temp_values[0]);
  Serial.print("\ttemp_2=");
  Serial.print(data.temp_values[1]);
  Serial.print("\ttemp_3=");
  Serial.print(data.temp_values[2]);
  Serial.print("\ttemp_4=");
  Serial.print(data.temp_values[3]);
#endif

#ifdef POOL_INFRATHERM
  Serial.print("\tinfratherm=");
  Serial.print(data.infrat_value);
#endif

  Serial.println();

#ifdef OUTPUT_BINARY
  byte * t_bytePtr = (byte *)&data;
  Serial.print("MESSAGE(BIN): ");
  for (int i=0; i<DATA_LENGTH; i++) {
    if (t_bytePtr[i]<0x10) {Serial.print("0");} 
    Serial.print(t_bytePtr[i],HEX);
    Serial.print(" "); 
  }
  Serial.println("");
#endif
#ifdef OUTPUT_TEXTCSV
  Serial.print("MESSAGE(TEXT): ");
  Serial.print(messageBuffer);
  Serial.println("");
#endif
}
#endif  // DEBUG_MODE

// *************
// *** SETUP ***
// *************

void setup()
{
#if defined(DEBUG_MODE)
  Serial.begin(9600);  // start serial for output (fast)
  pinMode(DEBUG_LED,OUTPUT);
  digitalWrite(DEBUG_LED, LOW);
#endif // DEBUG_MODE
#if defined(COMM_EMULATION)
  Serial.begin(9600);  // start serial for output (fast)
  store.debugMode = true;
#endif

  Wire.begin();        // join i2c bus (address optional for master)

  setupSensors();
}

// ************
// *** LOOP ***
// ************

unsigned long int previousMs;
unsigned long int nextMs;

void loop()
{
  previousMs = millis();  // marking the previous millis for "intelligent" delay

#ifdef DEBUG_MODE
  digitalWrite(DEBUG_LED, HIGH);
#endif

  poolValues();   // pool the values needed

#ifdef OUTPUT_TEXTCSV
  prepareMessage();
#endif

#ifdef DEBUG_MODE
  outputValues();
#endif

#ifdef OUTPUT_BINARY
  byte * t_bytePtr = (byte *)&data;
  store.send(t_bytePtr, 0, DATA_LENGTH);   // sends data into the communication file and queue for transfer
                                           // WARNING : introduces a 100ms delay
#endif
#ifdef OUTPUT_TEXTCSV
  store.send(messageBuffer);               // sends data into the communication file and queue for transfer
                                           // WARNING : introduces a 100ms delay
#endif

#ifdef DEBUG_MODE
  digitalWrite(DEBUG_LED, LOW);
#endif

  nextMs = PULL_DELAY-(millis()-previousMs);  // "intelligent delay" : just the ms needed to have a perfect timing, takes into account the delay of all the functions in the loop
  
  if (nextMs > 0)
    delay(nextMs); //wait for next pull
}

