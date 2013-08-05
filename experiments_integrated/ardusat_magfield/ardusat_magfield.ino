/*
    ardusat_magfield.ino - simply gathers the magnetometer raw values and send it back to earth 
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

#define POOL_DELAY  30000  // data is pooled every 12 seconds
                     // note : this makes approx 480 data points for 1 earth rotation
                     // and 1,12 rotations before reaching the 10kb limit for data
#define MESSAGE_BUFFER_SIZE  32  // only 32 chars needed in our case
                                 // max length message : #4294967296;-32768;-32768;-32768

// ***************
// *** HEADERS ***
// ***************

#include <Wire.h>//for I2C
#include <nanosat_message.h>
#include <I2C_add.h>

#include <EEPROM.h>
#include <OnboardCommLayer.h>
#include <SAT_AppStorage.h>

#include <SAT_Mag.h>


// ************************
// *** API CONSTRUCTORS ***
// ************************

SAT_Mag mag;

SAT_AppStorage store;


// ********************************
// *** SENSOR POOLING FUNCTIONS ***
// ********************************

long int previousMillis = 0;
long int id;
int16_t values[3];

// pools the values needed by the experiment
// here : the magnetometer
void poolValues() {
  long int t_id = millis();
  values[0] = mag.readx();
  values[1] = mag.ready();
  values[2] = mag.readz();
  id = t_id - previousMillis;
  previousMillis = t_id;
}

char messageBuffer[MESSAGE_BUFFER_SIZE];  // buffer for organising the message to be sent to earth
int bufferLen = 0;

// printing the values in an optimized format (we hope !)
void prepareBuffer() {
  // RESET
  bufferLen = 0;
  for (int i=0; i<MESSAGE_BUFFER_SIZE; i++)
    messageBuffer[i] = '\0';

  messageBuffer[0]='#';
  bufferLen++;
  
  sprintf(messageBuffer+bufferLen, "%li;", id);

  bufferLen = strlen(messageBuffer);
  if (bufferLen >= (MESSAGE_BUFFER_SIZE-1))
    return;
  for (int i=0; i<3; i++) {
    sprintf(messageBuffer+bufferLen, "%i;", (int16_t)values[i]);
    bufferLen = strlen(messageBuffer);
    if (bufferLen >= (MESSAGE_BUFFER_SIZE-1))
      return;
  }
  // erase last ";" and secure the end of line
  messageBuffer[bufferLen-1] = '\0';
  bufferLen = strlen(messageBuffer);
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

  store.send(messageBuffer);   // sends data into the communication file and queue for transfer
                               // WARNING : introduces a 100ms delay

  nextMs = POOL_DELAY-(millis()-previousMs);  // "intelligent delay" : just the ms needed to have a perfect timing, takes into account the delay of all the functions in the loop

  if (nextMs > 0)
    delay(nextMs); //wait for next pool
}

