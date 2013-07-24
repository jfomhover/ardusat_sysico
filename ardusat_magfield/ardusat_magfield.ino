/*
    AMPT_MagExp.ino - gathers the magnetometer raw values and send it back to earth 
    @author : Jean-François Omhover for Arts et Metiers ParisTech

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

    CHANGES
    - 07/24/2013 : added a DEBUG_MODE macro for printing out the readable results, also did a little bit of ordering and commenting
    - 07/24/2013 : tested on MAG3110, modified the type of "value[]" for int16_t, in order to keep the signed 16bits data coming out of the sensor
    - 07/22/2013 : introduce an "intelligent" delay, exact delay between two values (in order to take into account the delay of the support functions)
    - 07/22/2013 : first test without real mag/comm, then corrections on sprintf bugs (multiple ["%04x",int] in the same line seems to have a wrong behavior)
    - 07/22/2013 : added mag emulation to test the code without the sensor
    - 07/22/2013 : added comm emulation to test the code without the ArduSat infrastructure    
    - 07/17/2013 : license edited cause unsuitable for code

    TODOLIST
    - TODO : we really need to implement a decoder now ^^
    - TODO : try to decode the data and see if it matches the real values
*/


// **************
// *** CONFIG ***
// **************

#define DEBUG_MODE      // prints the values in readable format via Serial
#define COMM_EMULATION  // to emulate the Comm via SAT_AppStorageEMU, prints out (via Serial) the results of the data sent
//#define MAG_EMULATION // to emulate the MAG3110 via SAT_MagEMU, outputs constant values
#define POOL_DELAY  12000  // data is pooled every 12 seconds
                     // note : this makes approx 480 data points for 1 earth rotation
                     // and 1,12 rotations before reaching the 10kb limit for data
#define MESSAGE_BUFFER_SIZE  24  // only 24 chars needed in our case (18 hex chars actually)


// ***************
// *** HEADERS ***
// ***************

#include <Wire.h>//for I2C
#include <nanosat_message.h>
#include <I2C_add.h>

#ifdef COMM_EMULATION
#include "SAT_AppStorageEMU.h"
#else
#include <EEPROM.h>
#include <OnboardCommLayer.h>
#include <SAT_AppStorage.h>
#endif /* COMM_EMULATION */

#ifdef MAG_EMULATION
#include "SAT_MagEMU.h"
#else
#include <SAT_Mag.h>
#endif /* MAG_EMULATION */

// ************************
// *** API CONSTRUCTORS ***
// ************************

#ifdef MAG_EMULATION
SAT_MagEMU mag;
#else
SAT_Mag mag;
#endif

#ifdef COMM_EMULATION
SAT_AppStorageEMU store;
#else
SAT_AppStorage store;
#endif


// ********************************
// *** SENSOR POOLING FUNCTIONS ***
// ********************************

unsigned long int id;
int16_t values[3];

// pools the values needed by the experiment
// here : the magnetometer
void poolValues() {
  id = millis();
  values[0] = mag.readx();
  values[1] = mag.ready();
  values[2] = mag.readz();
}

char messageBuffer[MESSAGE_BUFFER_SIZE];  // buffer for printing the message to be sent to earth

// printing the values in an optimized format (we hope !)
void prepareBuffer() {
  // safety first (you never know...) clearing the buffer
  for (int i=0; i<MESSAGE_BUFFER_SIZE; i++) {
    messageBuffer[i] = '\0';
  }

  // data size of securely reduced to fit the sprintf
  id = id & 0x00FFFFFF;  // we keep only 6 hex digits (4.6h before looping)
  sprintf(messageBuffer, "%06X", id);

  for (int i=0; i<3; i++) {
    values[i] = values[i] & 0xFFFF;  // we keep only the 4 hex digits (this sensors returns 16 bits only anyway).
    sprintf(messageBuffer+6+i*4, "%04X", values[i]); // after testing, it seems using multiple "%04x" on the same sprintf has a bugged behavior
  }
}


// *************
// *** SETUP ***
// *************

void setup()
{
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output (fast)
  mag.configMag();          // turn the MAG3110 on
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
  Serial.print("*** DEBUG :");
  Serial.print("\tid=");
  Serial.print(id);
  Serial.print("\tX=");
  Serial.print(values[0]);
  Serial.print("\tY=");
  Serial.print(values[1]);
  Serial.print("\tZ=");
  Serial.println(values[2]);
#endif

  store.send(messageBuffer);   // sends data into the communication file and queue for transfer
                               // WARNING : introduces a 100ms delay

  nextMs = POOL_DELAY-(millis()-previousMs);  // "intelligent delay" : just the ms needed to have a perfect timing, takes into account the delay of all the functions in the loop
  delay(nextMs); //wait for next pool
}

