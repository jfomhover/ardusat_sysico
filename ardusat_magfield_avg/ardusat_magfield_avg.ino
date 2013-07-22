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
    - 07/22/2013 : introduce an "intelligent" delay, exact delay between two values (in order to take into account the delay of the support functions)
    - 07/22/2013 : first test without real mag/comm, then corrections on sprintf bugs (multiple ["%04x",int] in the same line seems to have a wrong behavior)
    - 07/22/2013 : added mag emulation to test the code without the sensor
    - 07/22/2013 : added comm emulation to test the code without the ArduSat infrastructure    
    - 07/17/2013 : license edited cause unsuitable for code
    - TODO : sprintf : i'm never sure of this kind of syntax, need to check if there is a "0" before bytes (like in 0xFF or 0FF)
    - TODO : to be tested with the real sensor values before end of july
*/

#include <Wire.h>//for I2C

#include <nanosat_message.h>
#include <I2C_add.h>
#include "SensorHandler.h"

#define COMM_EMULATION

#ifdef COMM_EMULATION
#include "SAT_AppStorageEMU.h"
#else
#include <EEPROM.h>
#include <OnboardCommLayer.h>
#include <SAT_AppStorage.h>
#endif /* COMM_EMULATION */

#define MAG_EMULATION

#ifdef MAG_EMULATION
#include "SAT_MagEMU.h"
#else
#include <SAT_Mag.h>
#endif

// *** SDK constructors needed
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


// *** CONFIG ***
#define POOL_PERIOD 50  // sensors are pooled every POOL_PERIOD (ms)
#define POOL_INTERVAL  53000  // data is pooled for POOL_INTERVAL (ms) before sending data


// ******************************
// *** SENSOR VALUES HANDLING ***
// ******************************

unsigned long int id;
int valuesBuffer_x[1024];            // yeah, i know, but I just wanted to keep all the havy memory allocations on the "front page"
int valuesBuffer_y[1024];
int valuesBuffer_z[1024];
SensorHandler<int> sensor_x(valuesBuffer_x, 1024);
SensorHandler<int> sensor_y(valuesBuffer_y, 1024);
SensorHandler<int> sensor_z(valuesBuffer_z, 1024);

void resetValues() {
  sensor_x.reset();
  sensor_y.reset();
  sensor_y.reset();
}

// pools the values needed by the experiment
void poolValues() {
  id = millis();
  sensor_x.pushValue((int)mag.readx());
  sensor_y.pushValue((int)mag.ready());
  sensor_z.pushValue((int)mag.readz());
}


// *************************
// *** MESSAGE STRUCTURE ***
// *************************

struct t_message {
  unsigned long int id;
  int count;
  
  int value_x;
  int value_x_min;
  int value_x_max;
  float value_x_avg;
  float value_x_var;
  
  int value_y;
  int value_y_min;
  int value_y_max;
  float value_y_avg;
  float value_y_var;
  
  int value_z;
  int value_z_min;
  int value_z_max;
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
  sprintf(messageIndex, "%06X", id);              messageIndex += 6;

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
}

// *****************
// *** MAIN LOOP ***
// *****************

unsigned long int previousMs;

void loop()
{
  resetValues();          // blanks all the values of the sensor handlers
  
  previousMs = millis();  // marking the first millis before looping
  
                          // pool the values needed during the interval specified, each period
  while ((millis()-previousMs)<POOL_INTERVAL) {
    poolValues();
    delay(POOL_PERIOD);
  }
  
  prepareMessage();            // prepare a structure for the message to be sent
  
  prepareBuffer();             // prepare the buffer for sending the message
  
  store.send(messageBuffer);   // sends data into the communication file and queue for transfer
                               // WARNING : introduces a 100ms delay
}

