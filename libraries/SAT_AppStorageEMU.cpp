/******************************************************************************
 * Includes
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <I2C_add.h>
#include "SAT_AppStorageEMU.h"
#include "nanosat_message.h"
#include "EEPROM.h"
#include <Arduino.h>

/******************************************************************************
 * Constructors
 ******************************************************************************/
SAT_AppStorageEMU::SAT_AppStorageEMU() {
  Serial.begin(9600);
  nodeAddress_  = 0xFE;
}

/******************************************************************************
 * User API
 ******************************************************************************/
/*
   Passes the experiment data to the comm layer; the comm layer enqueues if for
   sending upon request from master;

   @note There is a 100ms delay to allow time for FS work.
*/
void SAT_AppStorageEMU::send(char data[]){
  unsigned int dataLen  = (unsigned)strlen(data);
  unsigned int messages = dataLen / NODE_COMM_MAX_BUFFER_SIZE;
  
  Serial.print("*** SAT_AppStorageEMU::send() : data=");
  Serial.println(data);

  for(unsigned int i = 0; i < messages; i++) {
    unsigned int start_offset   = i * NODE_COMM_MAX_BUFFER_SIZE;
    copyAndSend(data, start_offset, NODE_COMM_MAX_BUFFER_SIZE);
  }
  // process remainder or if data was less then NODE_COMM_MAX_BUFFER_SIZE;
  uint8_t remainderLen = dataLen % NODE_COMM_MAX_BUFFER_SIZE;
  uint8_t finalOffset  = (dataLen > NODE_COMM_MAX_BUFFER_SIZE) ?
    (messages * NODE_COMM_MAX_BUFFER_SIZE) : 0;
  copyAndSend(data, finalOffset, remainderLen);
}

void SAT_AppStorageEMU::copyAndSend(
  char data[], unsigned int offset, unsigned int length)
{
  nanosat_message_t msg;
  msg.node_addr = nodeAddress_;
  msg.prefix    = NODE_COMM_MESSAGE_PREFIX;
  msg.len       = length;
  msg.type      = APPEND;
  memcpy(msg.buf, (uint8_t*)&(data[offset]), length * sizeof(char));
  // commLayer_.sendMessage(msg);
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
  dataCount+=msg.len;
  Serial.print(" totalLen=");
  Serial.println(dataCount);
  delay(100);
}
