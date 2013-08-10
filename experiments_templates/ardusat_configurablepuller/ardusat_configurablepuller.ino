/*
    File :         ardusat_configurablepuller.ino
    Author :       Jean-Francois Omhover (@jfomhover)
    Last Changed : Aug. 10th 2013
    Description :  configurable code for pulling the RAW VALUES (int) of the sensors regularly
                   and returning binary values back to earth

    Usage :        - required : select (comment/uncomment) the necessary sensors in for CONFIG section below
                   - required : choose (define) the right PULL_DELAY for the regular pooling
                   - required : select (comment/uncomment) an output mode : OUTPUT_BINARY or OUTPUT_TEXTCSV
                   - option : uncomment DEBUG_MODE to output verbose lines on Serial (and blink a led at each pooling cycle)
                   - option : modify the CHARBUFFER_SPACE macro to fit your requirements
                   - option : use COMM_EMULATION for emulating SAT_AppStorage (output on Serial instead of sending data)
                   - option : use COMM_EMULATION for emulating SAT_AppStorage (output on SD instead of sending data)
                   ! Beware of the 10ko limit, choose the output format wisely.

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

// *** DEBUGGING ***
#define DEBUG_MODE              // prints the values in readable format via Serial
#define DEBUG_LED   9           // the pin of the led that will blink at each pool of the sensors
#define DEBUG_FREERAM           // use MemoryFree lib to compute the amount of free ram (only for DEBUG_MODE)
#define DEBUG_BAUDRATE  9600    // the baud rate used for serial outputs
#define USE_PROGMEM             // RECOMMENDED : free 130 bytes of ram in DEBUG_MODE by putting strings in PROGMEM

// *** I2C COMMUNICATION ***
// !!! choose ONLY ONE below
//#define COMM_EMULATION          // to emulate the Comm via SAT_AppStorageEMU, prints out (via Serial) the results of the data that is sent through
#define COMM_EMULATION_SD	  // to emulate the Comm via SAT_AppStorageEMUSD, writes the result on SD/"datalog.bin"
#define SD_CHIPSELECT      4      // pin used for CS by SD lib (ArduinoUno Ethernet CS = 4)
#define SD_FILENAME "datalog.bin" // name of the file used for storing data
#define SD_RESETFILE       false  // true to erase file at reset, false to append data at the end of the file
//#define LEGACY_SDK              // NOT IMPLEMENTED YET : use sdk BEFORE the integration of I2CComm, on Aug. 7th 2013

// *** SENSOR PULLING ***
#define PULL_DELAY  2000       // data is pooled every PULL_DELAY seconds
#define PULL_SAT_LUM           // comment to NOT PULL the luminosity values
#define PULL_SAT_MAG           // comment to NOT PULL the magnetometer values
#define PULL_SAT_TMP           // comment to NOT PULL the temperature values
//#define PULL_INFRATHERM        // comment to NOT PULL the infratherm values
//#define PULL_SAT_ACCEL         // comment to NOT PULL the accelerometer

// *** RESULTS / OUTPUTS ***
//#define OUTPUT_TEXTCSV          // to output in text/csv format
#define OUTPUT_BINARY         // to output in binary format (see struct below)
#define CHARBUFFER_SPACE  96   // space allocated for the message in ASCII (will be used only if OUTPUT_ASCII is uncommented)

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

#if defined(DEBUG_FREERAM)
#include "MemoryFree.h"
#endif

#if defined (USE_PROGMEM)
#include <avr/pgmspace.h>
#endif

#ifdef COMM_EMULATION
#include <SAT_AppStorageEMU.h>
#endif
#ifdef COMM_EMULATION_SD
#include <SD.h>
#include <EEPROM.h>
#include <SAT_AppStorageEMUSD.h>
#endif
#if !defined(COMM_EMULATION) && !defined(COMM_EMULATION_SD)
#include <EEPROM.h>
#include <OnboardCommLayer.h>
#include <SAT_AppStorage.h>
#endif /* NO EMULATION */

    // BELOW, ADD THE HEADERS YOU NEED
#ifdef PULL_SAT_LUM
#include <SAT_Lum.h>
#endif
#ifdef PULL_SAT_MAG
#include <SAT_Mag.h>
#endif
#ifdef PULL_SAT_TMP
#include <SAT_Temp.h>
#endif
#ifdef PULL_INFRATHERM
#include <SAT_InfraTherm.h>
#endif
#ifdef PULL_SAT_ACCEL
#include <SAT_Accel.h>
#endif


// ******************************
// *** MEMORY OPTI BY PROGMEM ***
// *** DO NOT TOUCH !!!       ***
// ******************************

#if defined(DEBUG_MODE)

#if defined(USE_PROGMEM)

#define T_STRINGCHAR  prog_uchar
#define T_STRINGMEM  PROGMEM
void printString(const prog_uchar *str) {
	char c;

	while((c = pgm_read_byte(str++)))
		Serial.write(c);
}

#else  // not USE_PROGMEM

#define T_STRINGCHAR  char
#define T_STRINGMEM
#define printString(str)  Serial.print(str)

#endif  // USE_PROGMEM

// *** RAM OPTIMIZATION FOR DEBUG (too much strings in this sketch)
const T_STRINGCHAR string_0[] T_STRINGMEM = "Found tsl_one";
#define PGM_STRING_FOUNDTSLONE	            string_0
const T_STRINGCHAR string_1[] T_STRINGMEM = "No tsl_one";
#define PGM_STRING_NOTFOUNDTSLONE           string_1

const T_STRINGCHAR string_2[] T_STRINGMEM = "Found tsl_two";
#define PGM_STRING_FOUNDTSLTWO	            string_2
const T_STRINGCHAR string_3[] T_STRINGMEM = "No tsl_two";
#define PGM_STRING_NOTFOUNDTSLTWO           string_3

const T_STRINGCHAR string_4[] T_STRINGMEM = "MESSAGE(BIN): ";
#define PGM_STRING_MESSAGEBIN               string_4
const T_STRINGCHAR string_5[] T_STRINGMEM = "MESSAGE(TEXT): ";
#define PGM_STRING_MESSAGETEXT              string_5

const T_STRINGCHAR string_6[] T_STRINGMEM = "*** DEBUG :";
#define PGM_STRING_DEBUG                    string_6

const T_STRINGCHAR string_7[] T_STRINGMEM = "\tms=";
#define PGM_STRING_SENSOR_MS                string_7
const T_STRINGCHAR string_8[] T_STRINGMEM = "\tvisible_1=";
#define PGM_STRING_SENSOR_LUMVIS1           string_8
const T_STRINGCHAR string_9[] T_STRINGMEM = "\tIR_1=";
#define PGM_STRING_SENSOR_LUMIR1            string_9
const T_STRINGCHAR string_10[] T_STRINGMEM = "\tvisible_2=";
#define PGM_STRING_SENSOR_LUMVIS2            string_10
const T_STRINGCHAR string_11[] T_STRINGMEM = "\tIR_2=";
#define PGM_STRING_SENSOR_LUMIR2             string_11
const T_STRINGCHAR string_12[] T_STRINGMEM = "\tX=";
#define PGM_STRING_SENSOR_MAGX               string_12
const T_STRINGCHAR string_13[] T_STRINGMEM = "\tY=";
#define PGM_STRING_SENSOR_MAGY               string_13
const T_STRINGCHAR string_14[] T_STRINGMEM = "\tZ=";
#define PGM_STRING_SENSOR_MAGZ               string_14
const T_STRINGCHAR string_15[] T_STRINGMEM = "\ttemp_1=";
#define PGM_STRING_SENSOR_TEMP1              string_15
const T_STRINGCHAR string_16[] T_STRINGMEM = "\ttemp_2=";
#define PGM_STRING_SENSOR_TEMP2              string_16
const T_STRINGCHAR string_17[] T_STRINGMEM = "\ttemp_3=";
#define PGM_STRING_SENSOR_TEMP3              string_17
const T_STRINGCHAR string_18[] T_STRINGMEM = "\ttemp_4=";
#define PGM_STRING_SENSOR_TEMP4              string_18
const T_STRINGCHAR string_19[] T_STRINGMEM = "\tinfratherm=";
#define PGM_STRING_SENSOR_INTHERM            string_19
const T_STRINGCHAR string_20[] T_STRINGMEM = "\taccel_X=";
#define PGM_STRING_SENSOR_ACCELX             string_20
const T_STRINGCHAR string_21[] T_STRINGMEM = "\taccel_Y=";
#define PGM_STRING_SENSOR_ACCELY             string_21
const T_STRINGCHAR string_22[] T_STRINGMEM = "\taccel_Z=";
#define PGM_STRING_SENSOR_ACCELZ             string_22

#endif

// ************************
// *** API CONSTRUCTORS ***
// ************************

#ifdef COMM_EMULATION
SAT_AppStorageEMU store;
#endif
#ifdef COMM_EMULATION_SD
SAT_AppStorageEMUSD store;
#endif
#if !defined(COMM_EMULATION) && !defined(COMM_EMULATION_SD)
SAT_AppStorage store;
#endif

    // BELOW, ADD THE CONSTRUCTORS YOU NEED
#ifdef PULL_SAT_LUM
SAT_Lum tsl_one(1);                // one is on the front
SAT_Lum tsl_two(2);                // the other... bottomplate ?
#endif

#ifdef PULL_SAT_MAG
SAT_Mag mag;
#endif

#ifdef PULL_SAT_TMP
SAT_Temp tmp_sensor1(1);        // previously was "payload"
SAT_Temp tmp_sensor2(2);
SAT_Temp tmp_sensor3(3);    // previously was "bottomplate"
SAT_Temp tmp_sensor4(4);
#endif

#ifdef PULL_INFRATHERM
SAT_InfraTherm mlx;
#endif

#ifdef PULL_SAT_ACCEL
SAT_Accel accel;
#endif

    // SETUP OF THE SENSORS NEEDED (constructors mainly)
void setupSensors() {

  // *** SAT_MAG SETUP ***
#ifdef PULL_SAT_MAG
  mag.configMag();          // turn the MAG3110 on
#endif

#ifdef PULL_SAT_LUM
  // *** SAT_LUM SETUP ***
#if defined(DEBUG_MODE)

#ifndef LEGACY_SDK
  if (tsl_one.begin()) {
#else // LEGACY_SDK
  if (tsl_one.begin(0x42)) {
#endif // LEGACY_SDK
    printString(PGM_STRING_FOUNDTSLONE);
  } else {
    printString(PGM_STRING_NOTFOUNDTSLONE);
//    while (1);
  }
#ifndef LEGACY_SDK
  if (tsl_two.begin()) {
#else // LEGACY_SDK
  if (tsl_two.begin(0x42)) {
#endif // LEGACY_SDK
    printString(PGM_STRING_FOUNDTSLTWO);
  } else {
    printString(PGM_STRING_NOTFOUNDTSLTWO);
//    while (1);
  }

#else // for non DEBUG_MODE

#ifndef LEGACY_SDK
  tsl_one.begin();
  tsl_two.begin();
#else  // LEGACY_SDK
  tsl_one.begin(1);
  tsl_two.begin(1);
#endif // LEGACY_SDK

#endif // DEBUG_MODE

  //tsl.setGain(SAT_Lum_GAIN_0X);         // set no gain (for bright situtations)
  tsl_one.setGain(SAT_Lum_GAIN_0X);      // set 16x gain (for dim situations)
  tsl_two.setGain(SAT_Lum_GAIN_16X);      // set 16x gain (for dim situations)

  //tsl.setTiming(SAT_Lum_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)
  tsl_one.setTiming(SAT_Lum_INTEGRATIONTIME_13MS);  // medium integration time (medium light)
  tsl_two.setTiming(SAT_Lum_INTEGRATIONTIME_101MS);  // medium integration time (medium light)
  //tsl.setTiming(SAT_Lum_INTEGRATIONTIME_402MS);  // longest integration time (dim light)

#endif // PULL_SAT_LUM

#ifdef PULL_SAT_ACCEL
  accel.powerOn();
#endif
}

// ********************************
// *** SENSOR POOLING FUNCTIONS ***
// ********************************

#define AVAILABLE_SAT_LUM         0x01
#define AVAILABLE_SAT_MAG         0x02
#define AVAILABLE_SAT_TMP         0x04
#define AVAILABLE_SAT_INFRATHERM  0x08
#define AVAILABLE_SAT_ACCEL       0x10

struct _dataStruct {
  char header;          // don't touch that, or adapt it, whatever ^^
  byte availableValues;
  long int ms;          // i use that as an id

          // BELOW, ADD THE SENSOR VALUES YOU NEED
#ifdef PULL_SAT_LUM
  uint16_t tsl_one_values[2];
  uint16_t tsl_two_values[2];
#endif

#ifdef PULL_SAT_MAG
  int16_t mag_values[3];
#endif

#ifdef PULL_SAT_TMP
  int16_t temp_values[4];
#endif // PULL_SAT_LUM

#ifdef PULL_INFRATHERM
  int16_t infrat_value;
#endif // PULL_INFRATHERM

#ifdef PULL_SAT_ACCEL
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
#endif
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
#ifdef PULL_SAT_LUM
                         | AVAILABLE_SAT_LUM
#endif
#ifdef PULL_SAT_MAG
                         | AVAILABLE_SAT_MAG
#endif
#ifdef PULL_SAT_TMP
                         | AVAILABLE_SAT_TMP
#endif
#ifdef PULL_INFRATHERM
                         | AVAILABLE_SAT_INFRATHERM
#endif
#ifdef PULL_SAT_ACCEL
                         | AVAILABLE_SAT_ACCEL
#endif
                         ;

  data.ms = millis();

      // BELOW, ADD THE POOLING FUNCTIONS YOU NEED
#ifdef PULL_SAT_LUM
  data.tsl_one_values[0] = tsl_one.getLuminosity(SAT_Lum_VISIBLE);
  data.tsl_one_values[1] = tsl_one.getLuminosity(SAT_Lum_INFRARED);
  data.tsl_two_values[0] = tsl_two.getLuminosity(SAT_Lum_VISIBLE);
  data.tsl_two_values[1] = tsl_two.getLuminosity(SAT_Lum_INFRARED);
#endif

#ifdef PULL_SAT_MAG
  data.mag_values[0] = mag.readx();
  data.mag_values[1] = mag.ready();
  data.mag_values[2] = mag.readz();
#endif

#ifdef PULL_SAT_TMP
  data.temp_values[0] = tmp_sensor1.getRawTemp();
  data.temp_values[1] = tmp_sensor2.getRawTemp();
  data.temp_values[2] = tmp_sensor3.getRawTemp();
  data.temp_values[3] = tmp_sensor4.getRawTemp();
#endif // PULL_SAT_TMP

#ifdef PULL_INFRATHERM
  data.infrat_value = mlx.getRawTemp();
#endif // PULL_INFRATHERM

#ifdef PULL_SAT_ACCEL
  accel.readAccel(&(data.accel_x), &(data.accel_y), &(data.accel_z));
#endif
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

// PRINTING MACROS
#define PRINTINT(buf,val)  printPreBufferInt(buf,val)
#define PRINTUINT(buf,val)  printPreBufferUInt(buf,val)
#define PRINTLONG(buf,val)	printPreBufferLInt(buf,val)
#define PRINTULONG(buf,val)	printPreBufferULInt(buf,val)

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

void printPreBufferLInt(char * buf, long int val) {
  memset(buf, 0x00, 16);
  ltoa(val, buf, 10);
  buf[strlen(buf)]=';';
}

void printPreBufferULInt(char * buf, unsigned long int val) {
  memset(buf, 0x00, 16);
  ultoa(val, buf, 10);
  buf[strlen(buf)]=';';
}

// PRINTING PROCESS
void prepareMessage() {
  memset(messageBuffer, 0, CHARBUFFER_SPACE);
  bufferLen = 0;

  PRINTULONG(messagePreBuffer, data.ms);
  if (!commitPreMessage())  return;

      // BELOW, ADD THE POOLING FUNCTIONS YOU NEED
#ifdef PULL_SAT_LUM
  PRINTUINT(messagePreBuffer, data.tsl_one_values[0]);
  if (!commitPreMessage())  return;
  PRINTUINT(messagePreBuffer, data.tsl_one_values[1]);
  if (!commitPreMessage())  return;
  PRINTUINT(messagePreBuffer, data.tsl_two_values[0]);
  if (!commitPreMessage())  return;
  PRINTUINT(messagePreBuffer, data.tsl_two_values[1]);
  if (!commitPreMessage())  return;
#endif //PULL_SAT_LUM

#ifdef PULL_SAT_MAG
  PRINTINT(messagePreBuffer, data.mag_values[0]);
  if (!commitPreMessage())  return;
  PRINTINT(messagePreBuffer, data.mag_values[1]);
  if (!commitPreMessage())  return;
  PRINTINT(messagePreBuffer, data.mag_values[2]);
  if (!commitPreMessage())  return;
#endif // PULL_SAT_MAG

#ifdef PULL_SAT_TMP
  PRINTINT(messagePreBuffer, data.temp_values[0]);
  if (!commitPreMessage())  return;

  PRINTINT(messagePreBuffer, data.temp_values[1]);
  if (!commitPreMessage())  return;

  PRINTINT(messagePreBuffer, data.temp_values[2]);
  if (!commitPreMessage())  return;

  PRINTINT(messagePreBuffer, data.temp_values[3]);
  if (!commitPreMessage())  return;
#endif // PULL_SAT_TMP

#ifdef PULL_INFRATHERM
  PRINTINT(messagePreBuffer, data.infrat_value);
  if (!commitPreMessage())  return;
#endif // PULL_INFRATHERM

#ifdef PULL_SAT_ACCEL
  PRINTINT(messagePreBuffer, data.accel_x);
  if (!commitPreMessage())  return;
  PRINTINT(messagePreBuffer, data.accel_y);
  if (!commitPreMessage())  return;
  PRINTINT(messagePreBuffer, data.accel_z);
  if (!commitPreMessage())  return;
#endif

  if ((bufferLen + 2) < CHARBUFFER_SPACE)
    messageBuffer[bufferLen++]='\n';
}
#endif  // OUTPUT_TEXTCSV

#ifdef DEBUG_MODE
void outputValues() {
  printString(PGM_STRING_DEBUG);

  printString(PGM_STRING_SENSOR_MS);
  Serial.print(data.ms);

      // BELOW, IF YOU'D LIKE SOME DEBUG, ADD THE PRINTING LINES

#ifdef PULL_SAT_LUM
  printString(PGM_STRING_SENSOR_LUMVIS1);
  Serial.print(data.tsl_one_values[0]);
  printString(PGM_STRING_SENSOR_LUMIR1);
  Serial.print(data.tsl_one_values[1]);
  printString(PGM_STRING_SENSOR_LUMVIS2);
  Serial.print(data.tsl_two_values[0]);
  printString(PGM_STRING_SENSOR_LUMIR2);
  Serial.print(data.tsl_two_values[1]);
#endif

#ifdef PULL_SAT_MAG
  printString(PGM_STRING_SENSOR_MAGX);
  Serial.print(data.mag_values[0]);
  printString(PGM_STRING_SENSOR_MAGY);
  Serial.print(data.mag_values[1]);
  printString(PGM_STRING_SENSOR_MAGZ);
  Serial.print(data.mag_values[2]);
#endif

#ifdef PULL_SAT_TMP
  printString(PGM_STRING_SENSOR_TEMP1);
  Serial.print(data.temp_values[0]);
  printString(PGM_STRING_SENSOR_TEMP2);
  Serial.print(data.temp_values[1]);
  printString(PGM_STRING_SENSOR_TEMP3);
  Serial.print(data.temp_values[2]);
  printString(PGM_STRING_SENSOR_TEMP4);
  Serial.print(data.temp_values[3]);
#endif

#ifdef PULL_INFRATHERM
  printString(PGM_STRING_SENSOR_INTHERM);
  Serial.print(data.infrat_value);
#endif

#ifdef PULL_SAT_ACCEL
  printString(PGM_STRING_SENSOR_ACCELX);
  Serial.print(data.accel_x);
  printString(PGM_STRING_SENSOR_ACCELY);
  Serial.print(data.accel_y);
  printString(PGM_STRING_SENSOR_ACCELZ);
  Serial.print(data.accel_z);
#endif

  Serial.println();

#ifdef OUTPUT_BINARY
  byte * t_bytePtr = (byte *)&data;
  printString(PGM_STRING_MESSAGEBIN);
//  Serial.print("MESSAGE(BIN): ");
  for (int i=0; i<DATA_LENGTH; i++) {
    if (t_bytePtr[i]<0x10) {Serial.print("0");}
    Serial.print(t_bytePtr[i],HEX);
    Serial.print(" ");
  }
  Serial.println("");
#endif
#ifdef OUTPUT_TEXTCSV
  printString(PGM_STRING_MESSAGETEXT);
//  Serial.print("MESSAGE(TEXT): ");
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

  Serial.begin(DEBUG_BAUDRATE);  // start serial for output
  I2CComm.begin();
  pinMode(DEBUG_LED,OUTPUT);
  digitalWrite(DEBUG_LED, LOW);
#if defined(COMM_EMULATION)
  store.configEMU(true,DEBUG_BAUDRATE);
#endif
#if defined(COMM_EMULATION_SD)
  store.configEMU(true,DEBUG_BAUDRATE,SD_CHIPSELECT,SD_RESETFILE,SD_FILENAME);
#endif

#else // NON DEBUG_MODE

#if defined(COMM_EMULATION)
  store.configEMU(false,0);
#endif
#if defined(COMM_EMULATION_SD)
  store.configEMU(false,0,4);
#endif

#endif // DEBUG_MODE

#if defined(DEBUG_FREERAM)
#if !defined(DEBUG_MODE)
  Serial.begin(DEBUG_BAUDRATE);
#endif
  Serial.print("free ram = ");
  Serial.println(freeMemory());
#endif
  setupSensors();
}

// ************
// *** LOOP ***
// ************

signed long int previousMs;
signed long int nextMs;

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

