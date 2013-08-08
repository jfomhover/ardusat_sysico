/*
    File :         lumcalibrate.ino
    Author :       Jean-Francois Omhover (@jfomhover)
    Last Changed : Aug. 8th 2013
    Description :  testing the different parameters of SAT_Lum under different conditions (see datasets)

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

#define DEBUG_MODE      // prints the values in readable format via Serial
#define COMM_EMULATION  // to emulate the Comm via SAT_AppStorageEMU, prints out (via Serial) the results of the data sent
#define POOL_DELAY  1000  // data is pooled every 12 seconds
                     // note : this makes approx 480 data points for 1 earth rotation
                     // and 1,12 rotations before reaching the 10kb limit for data
#define MESSAGE_BUFFER_SIZE  256  // only 24 chars needed in our case (18 hex chars actually)
#define MESSAGE_SCHEME_MAXSIZE  32 // the number of chars used to describe the format of the message


// ***************
// *** HEADERS ***
// ***************

#include <Wire.h>//for I2C
#include <nanosat_message.h>
#include <I2C_add.h>
#include <Arduino.h>
#include <I2CComm.h>

#ifdef COMM_EMULATION
#include <SAT_AppStorageEMU.h>
#else
#include <EEPROM.h>
#include <OnboardCommLayer.h>
#include <SAT_AppStorage.h>
#endif /* COMM_EMULATION */

#include <SAT_Lum.h>

#include <ArduSatMessageWriter.h>


// ************************
// *** API CONSTRUCTORS ***
// ************************

#ifdef COMM_EMULATION
SAT_AppStorageEMU store;
#else
SAT_AppStorage store;
#endif

SAT_Lum tsl(SAT_Lum_ADDR_FLOAT);

// ********************************
// *** SENSOR POOLING FUNCTIONS ***
// ********************************

unsigned long int id;
uint16_t lum_visible;
uint16_t lum_full;
uint16_t lum_infrared;

// pools the values needed by the experiment
// here : the magnetometer
void poolValues() {
  id = millis();
}

char messageBuffer[MESSAGE_BUFFER_SIZE];  // buffer for organising the message to be sent to earth
ArduSatMessageWriter messageHdl(messageBuffer,            // argument 1 : available char[] as buffer
                          MESSAGE_BUFFER_SIZE,      // argument 2 : the size of the buffer
                          MESSAGE_FORMAT_TEXT,       // argument 3 : the format of the message (see MessageWriter.h)
                          MESSAGE_SCHEME_MAXSIZE);  // argument 4 : the size of the scheme

// printing the values in an optimized format (we hope !)
void prepareBuffer() {
  messageHdl.zero();
  id = millis();

  tsl.setGain(SAT_Lum_GAIN_0X);         // set no gain (for bright situtations)
  tsl.setTiming(SAT_Lum_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)
  lum_visible = tsl.getLuminosity(SAT_Lum_VISIBLE);
  lum_full = tsl.getLuminosity(SAT_Lum_FULLSPECTRUM);
  lum_infrared = tsl.getLuminosity(SAT_Lum_INFRARED);
  messageHdl.add(lum_visible);
  messageHdl.add(lum_full);
  messageHdl.add(lum_infrared);
  
  messageHdl.add(millis()-id,3); // we keep only 3 bytes (4.6h before looping)
  id = millis();

  tsl.setGain(SAT_Lum_GAIN_0X);         // set no gain (for bright situtations)
  tsl.setTiming(SAT_Lum_INTEGRATIONTIME_101MS);  // medium integration time (medium light)
  lum_visible = tsl.getLuminosity(SAT_Lum_VISIBLE);
  lum_full = tsl.getLuminosity(SAT_Lum_FULLSPECTRUM);
  lum_infrared = tsl.getLuminosity(SAT_Lum_INFRARED);
  messageHdl.add(lum_visible);
  messageHdl.add(lum_full);
  messageHdl.add(lum_infrared);

  messageHdl.add(millis()-id,3); // we keep only 3 bytes (4.6h before looping)
  id = millis();

  tsl.setGain(SAT_Lum_GAIN_0X);         // set no gain (for bright situtations)
  tsl.setTiming(SAT_Lum_INTEGRATIONTIME_402MS);  // longest integration time (dim light)
  lum_visible = tsl.getLuminosity(SAT_Lum_VISIBLE);
  lum_full = tsl.getLuminosity(SAT_Lum_FULLSPECTRUM);
  lum_infrared = tsl.getLuminosity(SAT_Lum_INFRARED);
  messageHdl.add(lum_visible);
  messageHdl.add(lum_full);
  messageHdl.add(lum_infrared);

  messageHdl.add(millis()-id,3); // we keep only 3 bytes (4.6h before looping)
  id = millis();

  tsl.setGain(SAT_Lum_GAIN_16X);      // set 16x gain (for dim situations)
  tsl.setTiming(SAT_Lum_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)
  lum_visible = tsl.getLuminosity(SAT_Lum_VISIBLE);
  lum_full = tsl.getLuminosity(SAT_Lum_FULLSPECTRUM);
  lum_infrared = tsl.getLuminosity(SAT_Lum_INFRARED);
  messageHdl.add(lum_visible);
  messageHdl.add(lum_full);
  messageHdl.add(lum_infrared);

  messageHdl.add(millis()-id,3); // we keep only 3 bytes (4.6h before looping)
  id = millis();

  tsl.setGain(SAT_Lum_GAIN_16X);      // set 16x gain (for dim situations)
  tsl.setTiming(SAT_Lum_INTEGRATIONTIME_101MS);  // medium integration time (medium light)
  lum_visible = tsl.getLuminosity(SAT_Lum_VISIBLE);
  lum_full = tsl.getLuminosity(SAT_Lum_FULLSPECTRUM);
  lum_infrared = tsl.getLuminosity(SAT_Lum_INFRARED);
  messageHdl.add(lum_visible);
  messageHdl.add(lum_full);
  messageHdl.add(lum_infrared);

  messageHdl.add(millis()-id,3); // we keep only 3 bytes (4.6h before looping)
  id = millis();

  tsl.setGain(SAT_Lum_GAIN_16X);      // set 16x gain (for dim situations)
  tsl.setTiming(SAT_Lum_INTEGRATIONTIME_402MS);  // longest integration time (dim light)
  lum_visible = tsl.getLuminosity(SAT_Lum_VISIBLE);
  lum_full = tsl.getLuminosity(SAT_Lum_FULLSPECTRUM);
  lum_infrared = tsl.getLuminosity(SAT_Lum_INFRARED);
  messageHdl.add(lum_visible);
  messageHdl.add(lum_full);
  messageHdl.add(lum_infrared);

  messageHdl.add(millis()-id,3); // we keep only 3 bytes (4.6h before looping)
}


// *************
// *** SETUP ***
// *************

void setup()
{
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output (fast)

  if (tsl.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No sensor?");
    while (1);
  }
  
  // Now we're ready to get readings!
}

// ************
// *** LOOP ***
// ************

int previousMs;
int nextMs;
void loop()
{
  previousMs = millis();  // marking the previous millis for "intelligent" delay

  poolValues();   // pool the values needed
  prepareBuffer();   // prepare the buffer for sending the message

#ifdef DEBUG_MODE
  Serial.print("SCHEME: ");
  Serial.println(messageHdl.getScheme());
  Serial.print("MESSAGE: ");
  Serial.println(messageHdl.getMessage());
#endif

  store.send(messageHdl.getMessage());   // sends data into the communication file and queue for transfer
                               // WARNING : introduces a 100ms delay

  nextMs = POOL_DELAY-(millis()-previousMs);  // "intelligent delay" : just the ms needed to have a perfect timing, takes into account the delay of all the functions in the loop

  if (nextMs > 0)
    delay(nextMs); //wait for next pool
}

