/*
    ardusat_tmp.ino - gathers the values of the temperature sensor(s) 
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
*/

// **************
// *** CONFIG ***
// **************

#define DEBUG_MODE      // prints the values in readable format via Serial
#define COMM_EMULATION  // to emulate the Comm via SAT_AppStorageEMU, prints out (via Serial) the results of the data sent
#define POOL_DELAY  30000  // data is pooled every 12 seconds
                     // note : this makes approx 480 data points for 1 earth rotation
                     // and 1,12 rotations before reaching the 10kb limit for data
#define MESSAGE_BUFFER_SIZE  32  // only 24 chars needed in our case (18 hex chars actually)
#define MESSAGE_SCHEME_MAXSIZE  6 // the number of chars used to describe the format of the message

// ***************
// *** HEADERS ***
// ***************

#include <Wire.h>//for I2C
#include <nanosat_message.h>
#include <I2C_add.h>

#ifdef COMM_EMULATION
#include <SAT_AppStorageEMU.h>
#else
#include <EEPROM.h>
#include <OnboardCommLayer.h>
#include <SAT_AppStorage.h>
#endif /* COMM_EMULATION */

#include <ArduSatMessageWriter.h>

#include <SAT_Temp.h>

// ************************
// *** API CONSTRUCTORS ***
// ************************

#ifdef COMM_EMULATION
SAT_AppStorageEMU store;
#else
SAT_AppStorage store;
#endif

#define TEMP_PAYLOAD 0
#define TEMP_BOTTOMPLATE 2

SAT_Temp tmp_payload(TEMP_PAYLOAD);
SAT_Temp tmp_bottomplate(TEMP_BOTTOMPLATE);

// ********************************
// *** SENSOR POOLING FUNCTIONS ***
// ********************************

unsigned long int id;
float temp_values[2];

// pools the values needed by the experiment
// here : the magnetometer
void poolValues() {
  id = millis();
  temp_values[0] = tmp_payload.getTemp();
  temp_values[1] = tmp_bottomplate.getTemp();
}

char messageBuffer[MESSAGE_BUFFER_SIZE];  // buffer for organising the message to be sent to earth
ArduSatMessageWriter messageHdl(messageBuffer,            // argument 1 : available char[] as buffer
                          MESSAGE_BUFFER_SIZE,      // argument 2 : the size of the buffer
                          MESSAGE_FORMAT_TEXT,       // argument 3 : the format of the message (see MessageWriter.h)
                          MESSAGE_SCHEME_MAXSIZE);  // argument 4 : the size of the scheme

// printing the values in an optimized format (we hope !)
void prepareBuffer() {
  messageHdl.zero();
  messageHdl.add(id,3); // we keep only 3 bytes (4.6h before looping)
  messageHdl.add(temp_values[0],3,2);
  messageHdl.add(temp_values[1],3,2);
}


// *************
// *** SETUP ***
// *************

void setup(){
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output (fast)
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
  Serial.print("\ttemp_payload=");
  Serial.print(temp_values[0]);
  Serial.print("\ttemp_bottomplate=");
  Serial.println(temp_values[1]);
  
  Serial.print("MESSAGE: ");
  Serial.println(messageHdl.getMessage());
  Serial.print("SCHEME: ");
  Serial.println(messageHdl.getScheme());
#endif

  store.send(messageHdl.getMessage());   // sends data into the communication file and queue for transfer
                               // WARNING : introduces a 100ms delay

  nextMs = POOL_DELAY-(millis()-previousMs);  // "intelligent delay" : just the ms needed to have a perfect timing, takes into account the delay of all the functions in the loop

  if (nextMs > 0)
    delay(nextMs); //wait for next pool
}

