/*
    Author :       Jean-Francois Omhover (jf.omhover@gmail.com, twitter:@jfomhover)
    Last Changed : January 16, 2014
    Description :  regularly pulls the four digital temperature sensors (TMP102) : 2 in the payload, 2 on the bottomplate
                   and returns the binary values back to earth via a data structure
                   will collect data covering approx 1.1 rotations around earth
                   duration expected : 103 minutes

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

// *************************
// *** EXPERIMENT CONFIG ***
// *************************

//#define DEBUG_MODE              // prints the values via Serial
#define ARDUSAT_PERIOD 5552     // approx from supposed TLEs
#define PULL_DELAY 9000         // data is pooled every PULL_DELAY milliseconds
                                // considering there is 15 bytes of data per pulling
                                // that should bring approx 682 packets of sensor values (in the 10ko limit)
                                // that should cover approx 1.1 period

#include <Wire.h>
#include <I2C_add.h>
#include "SAT_Temp.h"
#include "nanosat_message.h"
#include <EEPROM.h>
#include <OnboardCommLayer.h>

#ifndef DEBUG_MODE
#include <SAT_AppStorage.h>
#else
#include <SAT_AppStorageEMU.h>  // experiment data channel (via Serial, see library at https://github.com/jfomhover/ArduSat-utils)
#endif


  
// *********************
// *** SDK INSTANCES ***
// *********************

#ifndef DEBUG_MODE
SAT_AppStorage store;       // experiment data channel (via comm)
#else
SAT_AppStorageEMU store;    // experiment data channel (via Serial, see library at https://github.com/jfomhover/ArduSat-utils )
#endif

SAT_Temp tmp_sensor1(1);    // supposedly "payload"
SAT_Temp tmp_sensor2(2);    // unknown position
SAT_Temp tmp_sensor3(3);    // supposedly "bottomplate"
SAT_Temp tmp_sensor4(4);    // unknown position


// ***********************
// *** DATA STRUCTURES ***
// ***********************

#define PACKET_HEADER_CHUNK  '#'
#define DATATYPE_MS              0x0001
#define DATATYPE_SAT_TMP1        0x0010
#define DATATYPE_SAT_TMP2        0x0020
#define DATATYPE_SAT_TMP3        0x0040
#define DATATYPE_SAT_TMP4        0x0080

#define PACKET_SIZE 15 // don't trust sizeof() sometimes

// structured packet of data, easy to decode after reception (see https://github.com/jfomhover/ArduSat-utils for syntax)
struct _dataChunk {
  char header;
  uint16_t datatypes;
  uint32_t ms;
  int16_t tmp1;
  int16_t tmp2;
  int16_t tmp3;
  int16_t tmp4;
} data;

// erasing all values in the data chunk
void resetChunk() {
  byte * t_ptr = (byte*)&data;
  for (int i=0; i<PACKET_SIZE; i++)
    t_ptr[i] = 0x00;
}


// *************************
// *** PULLING FUNCTIONS ***
// *************************

// pulling values from the sensors and filling the data structure
void pullValues() {
  data.header = PACKET_HEADER_CHUNK;
  data.datatypes = DATATYPE_MS | DATATYPE_SAT_TMP1 | DATATYPE_SAT_TMP2 | DATATYPE_SAT_TMP3 | DATATYPE_SAT_TMP4;
  data.ms = millis();
  data.tmp1 = tmp_sensor1.getRawTemp();
  data.tmp2 = tmp_sensor2.getRawTemp();
  data.tmp3 = tmp_sensor3.getRawTemp();
  data.tmp4 = tmp_sensor4.getRawTemp();
}


// ******************
// *** MAIN SETUP ***
// ******************

void setup() {

#ifdef DEBUG_MODE
  Serial.begin(9600);
  store.init( true ); // debug : outputing verbose lines on Serial
#endif

  Wire.begin();       // seems necessary in the SDK examples provided
}


// *****************
// *** MAIN LOOP ***
// *****************
 
signed long int previousMs = 0;
signed long int nextMs = 0;
int dataSent = 0;

void loop() {
  resetChunk();      // zeroes the data structure

  nextMs = PULL_DELAY-(millis()-previousMs);  // "(not so) intelligent delay" : just the ms needed to have a regular timing, takes into account the delay of all the functions in the loop
  previousMs += PULL_DELAY;
  
  if (nextMs > 0)
    delay(nextMs); //wait for next pull
  else
    previousMs += PULL_DELAY; // we missed a beat, skip one to get back on regular track

  pullValues();   // pull the values needed

  byte * t_ptr = (byte *)&data;
  store.send(t_ptr, 0, PACKET_SIZE);   // sends data into the communication file and queue for transfer
                                       // WARNING : introduces a 100ms delay

  dataSent += PACKET_SIZE;
  
/*
  if ((dataSent + PACKET_SIZE) > 10240) {
    // TODO : close experiments, clean mode ?
    // my understanding is : that will be done automatically when we reach the 10kb limit
  }
*/
}

