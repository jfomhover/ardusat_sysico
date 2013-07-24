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
    - 07/24/2013 : modified code for introduction of BUFFER_SIZE macro, set to 128 cause the arduino Uno can't take more.
    - 07/24/2013 : tested on MAG3110, modified the type of "value[]" for int16_t, in order to keep the signed 16bits data coming out of the sensor    
    - 07/22/2013 : introduce a sensorhandler for handling data "continuously" between two iterations, (min,max,avg,var) is sent with raw values
    - 07/22/2013 : (obsolete) introduce an "intelligent" delay, exact delay between two values (in order to take into account the delay of the support functions)
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
#define POOL_PERIOD 50  // sensors are pooled every POOL_PERIOD (ms)
#define POOL_INTERVAL  1000  // data is pooled for POOL_INTERVAL (ms) before sending data
#define BUFFER_SPACE  128

// ***************
// *** HEADERS ***
// ***************

#include <Wire.h>//for I2C
#include <nanosat_message.h>
#include <I2C_add.h>

#include "SensorHandler.h"

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
int16_t valuesBuffer_x[BUFFER_SPACE];            // yeah, i know, but I just wanted to keep all the havy memory allocations on the "front page"
int16_t valuesBuffer_y[BUFFER_SPACE];
int16_t valuesBuffer_z[BUFFER_SPACE];
SensorHandler<int16_t> sensor_x(valuesBuffer_x, BUFFER_SPACE);
SensorHandler<int16_t> sensor_y(valuesBuffer_y, BUFFER_SPACE);
SensorHandler<int16_t> sensor_z(valuesBuffer_z, BUFFER_SPACE);

#ifdef DEBUG_MODE
int freeRam() {
  int byteCounter = 0; // initialize a counter
  byte *byteArray; // create a pointer to a byte array
  
  while ( (byteArray = (byte*) malloc (byteCounter * sizeof(byte))) != NULL ) {
    byteCounter++; // if allocation was successful, then up the count for the next try
    free(byteArray); // free memory after allocating it
  }
  
  free(byteArray); // also free memory after the function finishes 
  return byteCounter; // send back the highest number of bytes successfully allocated
};
#endif

void resetValues() {
  sensor_x.reset();
  sensor_y.reset();
  sensor_y.reset();
}

// pools the values needed by the experiment
void poolValues() {
  id = millis();
  sensor_x.pushValue((int16_t)mag.readx());
  sensor_y.pushValue((int16_t)mag.ready());
  sensor_z.pushValue((int16_t)mag.readz());
}


// *************************
// *** MESSAGE STRUCTURE ***
// *************************

struct t_message {
  unsigned long int id;
  int count;
  
  int16_t value_x;
  int16_t value_x_min;
  int16_t value_x_max;
  float value_x_avg;
  float value_x_var;
  
  int16_t value_y;
  int16_t value_y_min;
  int16_t value_y_max;
  float value_y_avg;
  float value_y_var;
  
  int16_t value_z;
  int16_t value_z_min;
  int16_t value_z_max;
  float value_z_avg;
  float value_z_var;
} msg;


void prepareMessage() {
  msg.id = id;
  
  msg.count = sensor_x.getCount();
  
  msg.value_x = sensor_x.getValue();
  msg.value_x_min = sensor_x.getMinimum();
  msg.value_x_max = sensor_x.getMaximum();
  msg.value_x_avg = sensor_x.getAverage();
  msg.value_x_var = sensor_x.getVariance();

  msg.value_y = sensor_y.getValue();
  msg.value_y_min = sensor_y.getMinimum();
  msg.value_y_max = sensor_y.getMaximum();
  msg.value_y_avg = sensor_y.getAverage();
  msg.value_y_var = sensor_y.getVariance();

  msg.value_z = sensor_z.getValue();
  msg.value_z_min = sensor_z.getMinimum();
  msg.value_z_max = sensor_z.getMaximum();
  msg.value_z_avg = sensor_z.getAverage();
  msg.value_z_var = sensor_z.getVariance();
};


// **********************
// *** MESSAGE BUFFER ***
// **********************

#define MESSAGE_BUFFER_SIZE  96  // 94 chars needed in our case
char messageBuffer[MESSAGE_BUFFER_SIZE];  // buffer for printing the message to be sent to earth

// printing the values in an optimized format (we hope !)
void prepareBuffer() {
  // safety first (you never know...) clearing the buffer
  for (int i=0; i<MESSAGE_BUFFER_SIZE; i++) {
    messageBuffer[i] = '\0';
  }

  char * messageIndex = messageBuffer;

  // data size of securely reduced to fit the sprintf
  id = id & 0x00FFFFFF;  // we keep only 6 hex digits (4.6h before looping)
  
  sprintf(messageIndex, "%06X", id);              messageIndex+=6;

  sprintf(messageIndex, "%04X", msg.count);       messageIndex+=4;
  
  sprintf(messageIndex, "%04X", msg.value_x);     messageIndex+=4;
  sprintf(messageIndex, "%04X", msg.value_x_min); messageIndex+=4;
  sprintf(messageIndex, "%04X", msg.value_x_max); messageIndex+=4;
  sprintf(messageIndex, "%08X", msg.value_x_avg); messageIndex+=8;
  sprintf(messageIndex, "%08X", msg.value_x_var); messageIndex+=8;
  
  sprintf(messageIndex, "%04X", msg.value_y);     messageIndex+=4;
  sprintf(messageIndex, "%04X", msg.value_y_min); messageIndex+=4;
  sprintf(messageIndex, "%04X", msg.value_y_max); messageIndex+=4;
  sprintf(messageIndex, "%08X", msg.value_y_avg); messageIndex+=8;
  sprintf(messageIndex, "%08X", msg.value_y_var); messageIndex+=8;

  sprintf(messageIndex, "%04X", msg.value_z);     messageIndex+=4;
  sprintf(messageIndex, "%04X", msg.value_z_min); messageIndex+=4;
  sprintf(messageIndex, "%04X", msg.value_z_max); messageIndex+=4;
  sprintf(messageIndex, "%08X", msg.value_z_avg); messageIndex+=8;
  sprintf(messageIndex, "%08X", msg.value_z_var); messageIndex+=8;
};


// ******************
// *** MAIN SETUP ***
// ******************

void setup()
{
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output (fast)
  mag.configMag();          // turn the MAG3110 on
#ifdef DEBUG_MODE
  Serial.println("*** DEBUG : setup complete");
#endif
}

// *****************
// *** MAIN LOOP ***
// *****************

unsigned long int previousMs;

void loop()
{
#ifdef DEBUG_MODE
  Serial.print("*** DEBUG : free ram = ");
  Serial.println(freeRam());
  Serial.println("*** DEBUG : reset values");
#endif

  resetValues();          // blanks all the values of the sensor handlers
  
  previousMs = millis();  // marking the first millis before looping
  
                          // pool the values needed during the interval specified, each period
  while ((millis()-previousMs)<POOL_INTERVAL) {
    poolValues();
    delay(POOL_PERIOD);
  };
  
  prepareMessage();            // prepare a structure for the message to be sent
  
  prepareBuffer();             // prepare the buffer for sending the message
  
#ifdef DEBUG_MODE
  Serial.println("*** DEBUG : ");
  Serial.print("count=");
  Serial.println(msg.count);

  Serial.println("*** DEBUG : ");
  Serial.print("X=");
  Serial.print(msg.value_x);
  Serial.print("\tMIN=");
  Serial.print(msg.value_x_min);
  Serial.print("\tMAX=");
  Serial.print(msg.value_x_max);
  Serial.print("\tAVG=");
  Serial.print(msg.value_x_avg);
  Serial.print("\tVAR=");
  Serial.print(msg.value_x_var);
  Serial.println(" ;");
  
  Serial.println("*** DEBUG : ");
  Serial.print("Y=");
  Serial.print(msg.value_y);
  Serial.print("\tMIN=");
  Serial.print(msg.value_y_min);
  Serial.print("\tMAX=");
  Serial.print(msg.value_y_max);
  Serial.print("\tAVG=");
  Serial.print(msg.value_y_avg);
  Serial.print("\tVAR=");
  Serial.print(msg.value_y_var);
  Serial.println(" ;");

  Serial.println("*** DEBUG : ");
  Serial.print("Z=");
  Serial.print(msg.value_z);
  Serial.print("\tMIN=");
  Serial.print(msg.value_z_min);
  Serial.print("\tMAX=");
  Serial.print(msg.value_z_max);
  Serial.print("\tAVG=");
  Serial.print(msg.value_z_avg);
  Serial.print("\tVAR=");
  Serial.print(msg.value_z_var);
  Serial.println(" ;");
#endif

  store.send(messageBuffer);   // sends data into the communication file and queue for transfer
                               // WARNING : introduces a 100ms delay
}


