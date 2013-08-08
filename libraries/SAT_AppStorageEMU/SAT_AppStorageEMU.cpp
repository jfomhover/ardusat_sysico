/*
    File :         SAT_AppStorageEMU.cpp
    Author(s) :    + Jean-Francois Omhover (@jfomhover)
                   + NanoSatisfi Inc.
    Last Changed : Aug. 8th 2013
    Description :  emulator class adapted from the SAT_AppStorage class made by NanoSatisfi for ArduSat
                   outputs the values to Serial instead of communicating them via I2C
                   
    Usage :        - same as SAT_AppStorage
                   - option : modify debugMode -> true to output the message to Serial

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

/******************************************************************************
 * Includes
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <I2C_add.h>
#include "SAT_AppStorageEMU.h"
#include "nanosat_message.h"
#include "EEPROM.h"

/******************************************************************************
 * Constructors
 ******************************************************************************/
SAT_AppStorageEMU::SAT_AppStorageEMU()
{
  Serial.begin(9600);
  nodeAddress_  = EEPROM.read(0x00);
  debugMode = false;
}

/******************************************************************************
 * User API
 ******************************************************************************/
/*
   Passes the experiment data to the comm layer; the comm layer enqueues if for
   sending upon request from master;

   @note There is a 100ms delay to allow time for FS work.
*/
void SAT_AppStorageEMU::send(char data[])
{
  unsigned int dataLen  = (unsigned)strlen(data);
  unsigned int messages = dataLen / NODE_COMM_MAX_BUFFER_SIZE;

  for(unsigned int i = 0; i < messages; i++)
  {
    unsigned int start_offset   = i * NODE_COMM_MAX_BUFFER_SIZE;
    copyAndSend((byte*) data, start_offset, NODE_COMM_MAX_BUFFER_SIZE);
  }
  // process remainder or if data was less then NODE_COMM_MAX_BUFFER_SIZE;
  uint8_t remainderLen = dataLen % NODE_COMM_MAX_BUFFER_SIZE;
  uint8_t finalOffset  = (dataLen > NODE_COMM_MAX_BUFFER_SIZE) ?
    (messages * NODE_COMM_MAX_BUFFER_SIZE) : 0;
  copyAndSend((byte*) data, finalOffset, remainderLen);
}

void SAT_AppStorageEMU::send(byte *data, unsigned int start, unsigned int end)
{
  unsigned int dataLen  = (unsigned)(end-start);
  unsigned int messages = dataLen / NODE_COMM_MAX_BUFFER_SIZE;

  for(unsigned int i = 0; i < messages; i++)
  {
    unsigned int start_offset   = i * NODE_COMM_MAX_BUFFER_SIZE;
    copyAndSend((byte*) data, start+start_offset, NODE_COMM_MAX_BUFFER_SIZE);
  }
  // process remainder or if data was less then NODE_COMM_MAX_BUFFER_SIZE;
  uint8_t remainderLen = dataLen % NODE_COMM_MAX_BUFFER_SIZE;
  uint8_t finalOffset  = (dataLen > NODE_COMM_MAX_BUFFER_SIZE) ?
    (messages * NODE_COMM_MAX_BUFFER_SIZE) : 0;
  copyAndSend((byte*) data, start+finalOffset, remainderLen);

/*  for(unsigned int i = start; i < end; i++)
  {
    unsigned int remaning_bytes = end - i;
    if (remaning_bytes > NODE_COMM_MAX_BUFFER_SIZE)
      remaning_bytes = NODE_COMM_MAX_BUFFER_SIZE;
    copyAndSend(data, i, remaning_bytes);
  }*/
}

void SAT_AppStorageEMU::copyAndSend(
  byte data[], unsigned int offset, unsigned int length)
{
  nanosat_message_t msg;
  msg.node_addr = nodeAddress_;
  msg.prefix    = NODE_COMM_MESSAGE_PREFIX;
  msg.len       = length;
  msg.type      = APPEND;
  memcpy(msg.buf, (uint8_t*)&(data[offset]), length * sizeof(char));
  // commLayer_.sendMessage(msg);
  dataCount+=msg.len;

  if (dataCount > 10240) {
    Serial.print("*** END OF EXPERIMENT BY REACHING 10KB");
    while(1);
  }

  if (debugMode) {
    Serial.print("*** SAT_AppStorageEMU::copyAndSend() : (ms=");
    Serial.print(millis());
    Serial.print(")");
    Serial.print(" node_addr=");
    Serial.print(msg.node_addr,HEX);
    Serial.print(" prefix=");
    Serial.print(msg.prefix,HEX);
    Serial.print(" len=");
    Serial.print(msg.len);
    Serial.print(" type=");
    Serial.print(msg.type,HEX);
    Serial.print(" totalLen=");
    Serial.println(dataCount);
  }

  delay(100);
}
