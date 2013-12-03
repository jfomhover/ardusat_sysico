/*
    File :         ardusat_magfield3.ino (simplified)
    Author :       Jean-Francois Omhover (jf.omhover@gmail.com, twitter:@jfomhover)
    Last Changed : Dec. 3rd 2013, clean-up/simplified version of the ardusat_configurablepuller for pulling MAG values only
    Description :  configurable code for pulling the RAW VALUES (int) of the MAGNETOMETER regularly
                   and returning binary values back to earth

    Usage :        - compile, and launch

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


#define PULL_DELAY  22000        // data is pooled every PULL_DELAY seconds


// ***************
// *** HEADERS ***
// ***************

#include <Arduino.h>
#include <Wire.h>//for I2C
#include <nanosat_message.h>
#include <I2C_add.h>
#include <I2C_Conv.h>  // for SAT_Geiger

#include <EEPROM.h>
#include <OnboardCommLayer.h>
#include <SAT_AppStorage.h>

#include <SAT_Mag.h>
#include <util/crc16.h>

// ************************
// *** API CONSTRUCTORS ***
// ************************

SAT_AppStorage store;
SAT_Mag mag;/*
    File :         ardusat_magfield3.ino
    Author :       Jean-Francois Omhover (jf.omhover@gmail.com, twitter:@jfomhover)
    Last Changed : Dec. 3rd 2013, clean-up
    Description :  configurable code for pulling the RAW VALUES (int) of the MAGNETOMETER regularly
                   and returning binary values back to earth

    Usage :        - required : select (comment/uncomment) the necessary sensors in for CONFIG section below
                   - required : choose (define) the right PULL_DELAY for the regular pooling
                   - required : select (comment/uncomment) an output mode : OUTPUT_BINARY or OUTPUT_TEXTCSV
                   - option : uncomment DEBUG_MODE to output verbose lines on Serial (and blink a led at each pooling cycle)
                   - option : modify the CHARBUFFER_SPACE macro to fit your requirements
                   - option : use COMM_EMULATION for emulating SAT_AppStorage (output on Serial instead of sending data)
                   - option : use COMM_EMULATION for emulating SAT_AppStorage (output on SD instead of sending data)
                   ! Beware of the 10ko limit, choose the output format wisely (i.e. choose BINARY ! ^^)
                   ! Beware : considering the limit by SAT_AppStorage, you can't capture at > 8 or 9 Hz with the sketch below

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


// ***********************
// *** "PUBLIC" CONFIG ***
// ***********************

#define PULL_DELAY  1000        // data is pooled every PULL_DELAY seconds


// ***************
// *** HEADERS ***
// ***************

#include <Arduino.h>
#include <Wire.h>//for I2C
#include <nanosat_message.h>
#include <I2C_add.h>
#include <I2C_Conv.h>  // for SAT_Geiger

#include <EEPROM.h>
#include <OnboardCommLayer.h>
#include <SAT_AppStorage.h>

#include <SAT_Mag.h>
#include <util/crc16.h>

// ************************
// *** API CONSTRUCTORS ***
// ************************

SAT_AppStorage store;
SAT_Mag mag;


// ********************************
// *** SENSOR POOLING FUNCTIONS ***
// ********************************

#define AVAILABLE_SAT_LUM         0x01
#define AVAILABLE_SAT_MAG         0x02
#define AVAILABLE_SAT_TMP         0x04
#define AVAILABLE_SAT_INFRATHERM  0x08
#define AVAILABLE_SAT_ACCEL       0x10
#define AVAILABLE_SAT_GYRO        0x20
#define AVAILABLE_SAT_GEIGER      0x40
#define AVAILABLE_CRC16           0x80

struct _dataStruct {
  char header;          // don't touch that, or adapt it, whatever ^^
  byte availableValues;
  long int ms;          // i use that as an id

  int16_t mag_values[3];
  uint16_t  crc16;
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

  data.availableValues = AVAILABLE_SAT_MAG | AVAILABLE_CRC16;

  data.ms = millis();

  data.mag_values[0] = mag.readx();
  data.mag_values[1] = mag.ready();
  data.mag_values[2] = mag.readz();

  uint8_t * ptr_t = (uint8_t *)&data;
  uint8_t size_t = sizeof(struct _dataStruct) - 2;
  data.crc16 = 0;
  for (int i=0; i<size_t; i++)
    data.crc16 = _crc16_update(data.crc16,ptr_t[i]);
}


// ******************
// *** MAIN SETUP ***
// ******************

void setup()
{
  mag.configMag();          // turn the MAG3110 on
}


// *****************
// *** MAIN LOOP ***
// *****************

signed long int previousMs = 0;
signed long int nextMs = 0;
int dataSent = 0;

void loop()
{

  nextMs = PULL_DELAY-(millis()-previousMs);  // "intelligent delay" : just the ms needed to have a perfect timing, takes into account the delay of all the functions in the loop
  previousMs += PULL_DELAY;
  
  if (nextMs > 0)
    delay(nextMs); //wait for next pull
  else
    previousMs += PULL_DELAY; // we missed a beat, skip one to get back on regular track

  poolValues();   // pool the values needed

  byte * t_bytePtr = (byte *)&data;
  store.send(t_bytePtr, 0, DATA_LENGTH);   // sends data into the communication file and queue for transfer
                                           // WARNING : introduces a 100ms delay
  dataSent += DATA_LENGTH;
  
  if ((dataSent + DATA_LENGTH) > 10240) {
    // TODO : close experiments, clean mode
  }
}



// ********************************
// *** SENSOR POOLING FUNCTIONS ***
// ********************************

#define AVAILABLE_SAT_LUM         0x01
#define AVAILABLE_SAT_MAG         0x02
#define AVAILABLE_SAT_TMP         0x04
#define AVAILABLE_SAT_INFRATHERM  0x08
#define AVAILABLE_SAT_ACCEL       0x10
#define AVAILABLE_SAT_GYRO        0x20
#define AVAILABLE_SAT_GEIGER      0x40
#define AVAILABLE_CRC16           0x80

struct _dataStruct {
  char header;          // don't touch that, or adapt it, whatever ^^
  byte availableValues;
  long int ms;          // i use that as an id

  int16_t mag_values[3];
  uint16_t  crc16;
} data;

#define DATA_LENGTH  14  //sizeof(struct _dataStruct)

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

  data.availableValues = AVAILABLE_SAT_MAG | AVAILABLE_CRC16;

  data.ms = millis();

  data.mag_values[0] = mag.readx();
  data.mag_values[1] = mag.ready();
  data.mag_values[2] = mag.readz();

  uint8_t * ptr_t = (uint8_t *)&data;
  uint8_t size_t = sizeof(struct _dataStruct) - 2;
  data.crc16 = 0;
  for (int i=0; i<size_t; i++)
    data.crc16 = _crc16_update(data.crc16,ptr_t[i]);
}


// ******************
// *** MAIN SETUP ***
// ******************

void setup()
{
  mag.configMag();          // turn the MAG3110 on
}


// *****************
// *** MAIN LOOP ***
// *****************

signed long int previousMs = 0;
signed long int nextMs = 0;
int dataSent = 0;

void loop()
{

  nextMs = PULL_DELAY-(millis()-previousMs);  // "intelligent delay" : just the ms needed to have a perfect timing, takes into account the delay of all the functions in the loop
  previousMs += PULL_DELAY;
  
  if (nextMs > 0)
    delay(nextMs); //wait for next pull
  else
    previousMs += PULL_DELAY; // we missed a beat, skip one to get back on regular track

  poolValues();   // pool the values needed

  byte * t_bytePtr = (byte *)&data;
  store.send(t_bytePtr, 0, DATA_LENGTH);   // sends data into the communication file and queue for transfer
                                           // WARNING : introduces a 100ms delay
  dataSent += DATA_LENGTH;
  
  if ((dataSent + DATA_LENGTH) > 10240) {
    // TODO : close experiments, clean mode
    // TODO : for now, will continue to send data until shutdown by the controller
  }
}

