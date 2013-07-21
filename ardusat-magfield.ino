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

    CHANGES / TODOLIST
    - 07/22/2013 : added comm emulation to test the code without the ArduSat infrastructure    
    - 07/17/2013 : license edited cause unsuitable for code
    - TODO : sprintf : i'm never sure of this kind of syntax, need to check if there is a "0" before bytes (like in 0xFF or 0FF)
    - TODO : to be tested with the real sensor values before end of july
*/

#include <Wire.h>//for I2C

#include <nanosat_message.h>
#include <I2C_add.h>

#define COMM_EMULATION

#ifdef COMM_EMULATION
#include "SAT_AppStorageEMU.h"
#else
#include <EEPROM.h>
#include <OnboardCommLayer.h>
#include <SAT_AppStorage.h>
#endif /* COMM_EMULATION */

#include <SAT_Mag.h>

// *** SDK constructors needed
SAT_Mag mag;

#ifdef COMM_EMULATION
SAT_AppStorageEMU store;
#else
SAT_AppStorage store;
#endif

// *** CONFIG ***
#define POOL_DELAY  12000  // data is pooled every 12 seconds
                     // note : this makes approx 480 data points for 1 earth rotation
                     // and 1,12 rotations before reaching the 10kb limit for data

// ****************************
// *** EXPERIMENT FUNCTIONS ***
// ****************************

unsigned long int id;
int values[3];

// pools the values needed by the experiment
// here : the magnetometer
void poolValues() {
  id = millis();
  values[0] = mag.readx();
  values[1] = mag.ready();
  values[2] = mag.readz();
}

#define MESSAGE_BUFFER_SIZE  24  // only 24 chars needed in our case (18 hex chars actually)

char messageBuffer[MESSAGE_BUFFER_SIZE];  // buffer for printing the message to be sent to earth

// printing the values in an optimized format (we hope !)
void prepareBuffer() {
  // safety first (you never know...) clearing the buffer
  for (int i=0; i<MESSAGE_BUFFER_SIZE; i++) {
    messageBuffer[i] = '\0';
  }

  // data size of securely reduced to fit the sprintf
  id = id & 0x00FFFFFF;  // we keep only 6 hex digits
  for (int i=0; i<3; i++)
    values[i] = values[i] & 0xFFFF;  // we keep only the 4 hex digits (this sensors returns 16 bits only anyway).

  // data is printed in hex format
  sprintf(messageBuffer, "%06x%04x%04x%04x\0", id, values[0], values[1], values[2]);
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

void loop()
{
  poolValues();   // pool the values needed
  prepareBuffer();   // prepare the buffer for sending the message
  
  store.send(messageBuffer);   // sends data into the communication file and queue for transfer
  delay(POOL_DELAY); //wait for next pool
}
