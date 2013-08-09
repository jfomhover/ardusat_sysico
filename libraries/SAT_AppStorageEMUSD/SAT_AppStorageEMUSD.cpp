/*
    File :         SAT_AppStorageEMUSD.cpp
    Author(s) :    + Jean-Francois Omhover (@jfomhover)
                   + NanoSatisfi Inc.
    Last Changed : Aug. 8th 2013
    Description :  emulator class adapted from the SAT_AppStorage class made by NanoSatisfi for ArduSat
                   outputs the values to Serial + SD file instead of communicating them via I2C
                   
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
#include <nanosat_message.h>
#include <EEPROM.h>
#include "SAT_AppStorageEMUSD.h"
#include <SD.h>
#include <avr/pgmspace.h>

#define CHIPSELECT	4

// *** RAM OPTIMIZATION FOR DEBUG (too much strings in this class)
const prog_uchar string_0[] PROGMEM = "Initializing SD card...";
#define PGM_STRING_INITIALIZING	string_0
const prog_uchar string_1[] PROGMEM = "SD card failed, or not present";
#define PGM_STRING_SDFAILED		string_1
const prog_uchar string_2[] PROGMEM = "SD card initialized.";
#define PGM_STRING_INITIALIZED	string_2
const prog_uchar string_3[] PROGMEM = "SD writing...";
#define PGM_STRING_SDWRITING	string_3
const prog_uchar string_4[] PROGMEM = "SD not available...";
#define PGM_STRING_SDNOTAVAIL	string_4
const prog_uchar string_5[] PROGMEM = "*** END OF EXPERIMENT BY REACHING 10KB";
#define PGM_STRING_ENDOFEXP		string_5
const prog_uchar string_6[] PROGMEM = "*** SAT_AppStorageEMUSD::copyAndSend() : ";
#define PGM_STRING_COPYANDSEND	string_6
const prog_uchar string_7[] PROGMEM = " ms=";
#define PGM_STRING_MS			string_7
const prog_uchar string_8[] PROGMEM = " node_addr=";
#define PGM_STRING_NODEADDR		string_8
const prog_uchar string_9[] PROGMEM = " prefix=";
#define PGM_STRING_PREFIX		string_9
const prog_uchar string_10[] PROGMEM = " len=";
#define PGM_STRING_LEN			string_10
const prog_uchar string_11[] PROGMEM = " type=";
#define PGM_STRING_TYPE			string_11
const prog_uchar string_12[] PROGMEM = " type=";
#define PGM_STRING_TOTALLEN		string_12
const prog_uchar string_13[] PROGMEM = "Error opening ";
#define PGM_STRING_ERROROPENING	string_13


void ASEMUSD_printPROGMEMString(const prog_uchar *str) {
	char c;

	while((c = pgm_read_byte(str++)))
		Serial.write(c);
}

/******************************************************************************
 * Constructors
 ******************************************************************************/
SAT_AppStorageEMUSD::SAT_AppStorageEMUSD()
{
  nodeAddress_  = EEPROM.read(0x00);
  debugMode_ = false;
  SDavailable_ = false;
  dataCount_ = 0;
}

void SAT_AppStorageEMUSD::configEMU(boolean debug, int bauds, int csPin, char * filename) {
	debugMode_ = debug;

	if (filename == NULL)
		sprintf(filename_,"datalog.bin");
	else
		memcpy(filename_,filename,(strlen(filename) > 12) ? 12 : strlen(filename));

	if (debugMode_) {
		Serial.begin(bauds);
		ASEMUSD_printPROGMEMString(PGM_STRING_INITIALIZING);
//		Serial.print("Initializing SD card...");
	}

  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(CHIPSELECT)) {
	  SDavailable_ = false;
	  if (debugMode_)
			ASEMUSD_printPROGMEMString(PGM_STRING_SDFAILED);
//		  Serial.println("SD card failed, or not present");
  }
  SDavailable_ = true;
  if (debugMode_)
		ASEMUSD_printPROGMEMString(PGM_STRING_INITIALIZED);
//	  Serial.println("SD card initialized.");
}

/******************************************************************************
 * User API
 ******************************************************************************/
/*
   Passes the experiment data to the comm layer; the comm layer enqueues if for
   sending upon request from master;

   @note There is a 100ms delay to allow time for FS work.
*/
void SAT_AppStorageEMUSD::send(char data[])
{
  unsigned int dataLen  = (unsigned)strlen(data);
  unsigned int messages = dataLen / NODE_COMM_MAX_BUFFER_SIZE;

  if (SDavailable_) {
	  if(debugMode_)
			ASEMUSD_printPROGMEMString(PGM_STRING_SDWRITING);
//		  Serial.println("SD writing...");
	  write2SD((byte*)data,0, dataLen);
  } else
	  if(debugMode_)
			ASEMUSD_printPROGMEMString(PGM_STRING_SDNOTAVAIL);
//		  Serial.println("SD not available...");

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

void SAT_AppStorageEMUSD::send(byte *data, unsigned int offset, unsigned int length)
{
  unsigned int dataLen  = length;
  unsigned int messages = dataLen / NODE_COMM_MAX_BUFFER_SIZE;

  if (SDavailable_) {
	  if(debugMode_)
			ASEMUSD_printPROGMEMString(PGM_STRING_SDWRITING);
//		  Serial.println("SD writing...");
	  write2SD((byte*)data,0, dataLen);
  } else
	  if(debugMode_)
			ASEMUSD_printPROGMEMString(PGM_STRING_SDNOTAVAIL);
//		  Serial.println("SD not available...");

  for(unsigned int i = 0; i < messages; i++)
  {
    unsigned int start_offset   = i * NODE_COMM_MAX_BUFFER_SIZE;
    copyAndSend((byte*) data, offset+start_offset, NODE_COMM_MAX_BUFFER_SIZE);
  }
  // process remainder or if data was less then NODE_COMM_MAX_BUFFER_SIZE;
  uint8_t remainderLen = dataLen % NODE_COMM_MAX_BUFFER_SIZE;
  uint8_t finalOffset  = (dataLen > NODE_COMM_MAX_BUFFER_SIZE) ?
    (messages * NODE_COMM_MAX_BUFFER_SIZE) : 0;
  copyAndSend((byte*) data, offset+finalOffset, remainderLen);

/*  for(unsigned int i = start; i < end; i++)
  {
    unsigned int remaning_bytes = end - i;
    if (remaning_bytes > NODE_COMM_MAX_BUFFER_SIZE)
      remaning_bytes = NODE_COMM_MAX_BUFFER_SIZE;
    copyAndSend(data, i, remaning_bytes);
  }*/
}

void SAT_AppStorageEMUSD::copyAndSend(
  byte data[], unsigned int offset, unsigned int length)
{
  nanosat_message_t msg;
  msg.node_addr = nodeAddress_;
  msg.prefix    = NODE_COMM_MESSAGE_PREFIX;
  msg.len       = length;
  msg.type      = APPEND;
  memcpy(msg.buf, (uint8_t*)&(data[offset]), length * sizeof(char));
  // commLayer_.sendMessage(msg);
  dataCount_+=msg.len;


  if (dataCount_ > 10240) {
	  if (debugMode_)
		ASEMUSD_printPROGMEMString(PGM_STRING_ENDOFEXP);
//    Serial.print("*** END OF EXPERIMENT BY REACHING 10KB");
    while(1);
  }

  if (debugMode_) {
		ASEMUSD_printPROGMEMString(PGM_STRING_COPYANDSEND);
//    Serial.print("*** SAT_AppStorageEMUSD::copyAndSend() : ");
		ASEMUSD_printPROGMEMString(PGM_STRING_MS);
//    Serial.print(" ms=");
    Serial.print(millis());
	ASEMUSD_printPROGMEMString(PGM_STRING_NODEADDR);
//    Serial.print(" node_addr=");
    Serial.print(msg.node_addr,HEX);
	ASEMUSD_printPROGMEMString(PGM_STRING_PREFIX);
//   Serial.print(" prefix=");
    Serial.print(msg.prefix,HEX);
	ASEMUSD_printPROGMEMString(PGM_STRING_LEN);
//  Serial.print(" len=");
    Serial.print(msg.len);
	ASEMUSD_printPROGMEMString(PGM_STRING_TYPE);
//    Serial.print(" type=");
    Serial.print(msg.type,HEX);
	ASEMUSD_printPROGMEMString(PGM_STRING_TOTALLEN);
//    Serial.print(" totalLen=");
    Serial.println(dataCount_);
  }

  delay(100);
}

void SAT_AppStorageEMUSD::write2SD(byte data[], unsigned int offset, unsigned int length) {
	  // open the file. note that only one file can be open at a time,
	  // so you have to close this one before opening another.
	  File dataFile = SD.open("datalog.bin", FILE_WRITE);

	  // if the file is available, write to it:
	  if (dataFile) {
		  dataFile.write(data+offset,length);
		  dataFile.flush();
		  dataFile.close();
		  if (debugMode_)
			  for(int i=0; i<3; i++) {
				  digitalWrite(9,HIGH);
				  delay(50);
				  digitalWrite(9,LOW);
		  		  delay(50);
			  }
	  }
	  // if the file isn't open, pop up an error:
	  else {
		  if (debugMode_) {
				ASEMUSD_printPROGMEMString(PGM_STRING_ERROROPENING);
//			  Serial.print("error opening ");
		  	  Serial.println(filename_);
		  }
	  }
}
