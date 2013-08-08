/*
    File :         ardusat_genericpuller.ino
    Author :       Jean-Francois Omhover (@jfomhover)
    Last Changed : Aug. 8th 2013
    Description :  template code for pulling sensors regularly and returning binary values back to earth 
                   !!! this generic code is rather obsolete since there is a simpler "ardusat_configurablepuller.ino"
                   
    Usage :        - required : fill/modify all the functions where a "TODO" comment is placed
                   - option : you can choose to use COMM_EMULATION to simulate the OnboardCommLayer (requires the SAT_AppStorageEMU library)
                   - option : uncomment DEBUG_MODE to output verbose lines on Serial
                   
    ardusat_genericpooler.ino -  
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
#define COMM_EMULATION  // to emulate the Comm via SAT_AppStorageEMU, prints out (via Serial) the results of the data that is sent through
#define PULL_DELAY  15000  // data is pooled every PULL_DELAY seconds

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

    // BELOW, ADD THE HEADERS YOU NEED
#include <SAT_Lum.h>


// ************************
// *** API CONSTRUCTORS ***
// ************************

#ifdef COMM_EMULATION
SAT_AppStorageEMU store;
#else
SAT_AppStorage store;
#endif

    // TODO : BELOW, ADD THE CONSTRUCTORS YOU NEED
SAT_Lum tsl(SAT_Lum_ADDR_FLOAT);


    // TODO : BELOW, SETUP OF THE SENSORS NEEDED (constructors mainly)
void setupSensors() {
    if (tsl.begin(1)) {
#if defined(DEBUG_MODE)
    Serial.println("Found sensor");
#endif
  } else {
#if defined(DEBUG_MODE)
    Serial.println("No sensor?");
#endif
    while (1);
  }
  
  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  //tsl.setGain(SAT_Lum_GAIN_0X);         // set no gain (for bright situtations)
  tsl.setGain(SAT_Lum_GAIN_16X);      // set 16x gain (for dim situations)

  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  tsl.setTiming(SAT_Lum_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)
  //tsl.setTiming(SAT_Lum_INTEGRATIONTIME_101MS);  // medium integration time (medium light)
  //tsl.setTiming(SAT_Lum_INTEGRATIONTIME_402MS);  // longest integration time (dim light)  
}

// ********************************
// *** SENSOR POOLING FUNCTIONS ***
// ********************************

struct _dataStruct {
  char header;          // don't touch that, or adapt it, whatever ^^
  long int ms;          // i use that as an id

          // TODO : BELOW, ADD THE SENSOR VALUES YOU NEED
  uint16_t lum_visible;
  uint16_t lum_infrared;
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
  data.ms = millis();

      // TODO : BELOW, ADD THE POOLING FUNCTIONS YOU NEED
  data.lum_visible = tsl.getLuminosity(SAT_Lum_VISIBLE);
  data.lum_infrared = tsl.getLuminosity(SAT_Lum_INFRARED);
}

#ifdef DEBUG_MODE
void outputValues() {
  Serial.print("*** DEBUG :");
  Serial.print("\tms=");
  Serial.print(data.ms);

      // TODO : BELOW, IF YOU'D LIKE SOME DEBUG, ADD THE PRINTING LINES

  Serial.print("\tvisible=");
  Serial.print(data.lum_visible);
  Serial.print("\tIR=");
  Serial.print(data.lum_infrared);

      // AFTER THAT, DON'T TOUCH ANYTHING
  Serial.println();

  byte * t_bytePtr = (byte *)&data;
  Serial.print("MESSAGE: ");
  for (int i=0; i<DATA_LENGTH; i++) {
    if (t_bytePtr[i]<0x10) {Serial.print("0");} 
    Serial.print(t_bytePtr[i],HEX);
    Serial.print(" "); 
  }
  Serial.println("");
}
#endif  // DEBUG_MODE

// *************
// *** SETUP ***
// *************

void setup()
{
#if defined(DEBUG_MODE) || defined(COMM_EMULATION)
  Serial.begin(9600);  // start serial for output (fast)
  store.debugMode = true;
#endif

  Wire.begin();        // join i2c bus (address optional for master)

  setupSensors();
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

#ifdef DEBUG_MODE
  outputValues();
#endif

  byte * t_bytePtr = (byte *)&data;
  store.send(t_bytePtr, 0, DATA_LENGTH);   // sends data into the communication file and queue for transfer
                               // WARNING : introduces a 100ms delay

  nextMs = PULL_DELAY-(millis()-previousMs);  // "intelligent delay" : just the ms needed to have a perfect timing, takes into account the delay of all the functions in the loop

  if (nextMs > 0)
    delay(nextMs); //wait for next pool
}

