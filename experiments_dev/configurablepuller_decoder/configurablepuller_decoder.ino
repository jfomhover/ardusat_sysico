/*
    File :         configurablepuller_decoder.ino
    Author :       Jean-Francois Omhover (@jfomhover)
    Last Changed : Aug. 10th 2013
    Description :  decodes a binary file written on SD that has been created by the configurable puller
    Code :         derived from the SD dumpFile sketch created by Limor Fried, Tom Igoe
    Usage :        - required : check the SD_FILENAME to match your file
                   - required : check the SD_CHIPSELECT to match your config
                   - option : uncomment VERBOSE to output verbose lines on Serial for debug

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

//#define VERBOSE  // to output debug lines along the parsing

#define SD_FILENAME  "datalog.bin"  // the name of the file on SD card

// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.
#define SD_CHIPSELECT  4

#define DEBUG_BAUDRATE  115200  // the baud rate used for serial outputs


// ****************
// *** INCLUDES ***
// ****************

#include <SD.h>
#include <util/crc16.h>

// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.
const int chipSelect = 4;


// ***********************
// *** DATA STRUCTURES ***
// ***********************

// note : we HAVE to define the size manually cause structs aren't always aligned on 4 bytes (maybe ?)

#define SAT_MILLIS_HEADER "millis;"
#define SAT_MILLIS_SIZE  4
struct _dataMILLIS {
	long int ms;          // i use that as an id
};

#define SAT_LUM_HEADER "tsl1_visible;tsl1_ir;tsl2_visible;tsl2_ir;"
#define SAT_LUM_SIZE  8
struct _dataLUM {
	uint16_t tsl_one_values[2];
	uint16_t tsl_two_values[2];
};

#define SAT_MAG_HEADER "mag_x;mag_y;mag_z;"
#define SAT_MAG_SIZE  6
struct _dataMAG {
	int16_t mag_values[3];
};

#define SAT_TMP_HEADER "tmp_1;tmp_2;tmp_3;tmp_4;"
#define SAT_TMP_SIZE  8
struct _dataTMP {
	int16_t temp_values[4];
};

#define SAT_INFRATHERM_HEADER "infratherm;"
#define SAT_INFRATHERM_SIZE  2
struct _dataINFRATHERM {
  int16_t infrat_value;
};

#define SAT_ACCEL_HEADER "accel_x;accel_y;accel_z;"
#define SAT_ACCEL_SIZE  6
struct _dataACCEL {
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
};

#define SAT_GYRO_HEADER  "gyro_x;gyro_y;gyro_z;"
#define SAT_GYRO_SIZE  6
struct _dataGYRO {
  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
};

#define SAT_GEIGER_HEADER  ""
#define SAT_GEIGER_SIZE  0
struct _dataGEIGER {
// TODO
};

#define CRC16_HEADER  "crc16;crc_ok;"
#define CRC16_SIZE  2
struct _dataCRC16 {
  uint16_t crc16;
};

File dataFile;


// ******************************
// *** MAIN DECODING FUNCTION ***
// ******************************

#define CHUNK_HEADER_CHAR	'#'
#define AVAILABLE_SAT_LUM         0x01
#define AVAILABLE_SAT_MAG         0x02
#define AVAILABLE_SAT_TMP         0x04
#define AVAILABLE_SAT_INFRATHERM  0x08
#define AVAILABLE_SAT_ACCEL       0x10
#define AVAILABLE_SAT_GYRO        0x20
#define AVAILABLE_SAT_GEIGER      0x40
#define AVAILABLE_CRC16           0x80

#define MAX_BUFFER_LEN  128
struct _chunkStruct {
  char header;
  byte availableValues;
  byte readBuffer[MAX_BUFFER_LEN];
} chunk;

//byte * readBuffer = NULL;
//int bufferLen = 0;

byte previousContent = 0;

boolean decodeChunk(byte content) {
        // clears the buffer
        for (int i=0; i<MAX_BUFFER_LEN; i++)
          chunk.readBuffer[i] = 0;
          
	int expectedLen = sizeof(_dataMILLIS);
	if (content & AVAILABLE_SAT_LUM) {
		expectedLen += SAT_LUM_SIZE;
#ifdef VERBOSE
		Serial.print("+LUM");
#endif
	}
	if (content & AVAILABLE_SAT_MAG) {
		expectedLen += SAT_MAG_SIZE;
#ifdef VERBOSE
		Serial.print("+MAG");
#endif
	}
	if (content & AVAILABLE_SAT_TMP) {
		expectedLen += SAT_TMP_SIZE;
#ifdef VERBOSE
		Serial.print("+TMP");
#endif
	}
	if (content & AVAILABLE_SAT_INFRATHERM) {
		expectedLen += SAT_INFRATHERM_SIZE;
#ifdef VERBOSE
		Serial.print("+INFRATHERM");
#endif
	}
	if (content & AVAILABLE_SAT_ACCEL) {
		expectedLen += SAT_ACCEL_SIZE;
#ifdef VERBOSE
		Serial.print("+ACCEL");
#endif
	}
	if (content & AVAILABLE_SAT_GYRO) {
		expectedLen += SAT_GYRO_SIZE;
#ifdef VERBOSE
		Serial.print("+GYRO");
#endif
	}
	if (content & AVAILABLE_SAT_GEIGER) {
		expectedLen += SAT_GEIGER_SIZE;
#ifdef VERBOSE
		Serial.print("+GEIGER");
#endif
	}
	if (content & AVAILABLE_CRC16) {
		expectedLen += CRC16_SIZE;
#ifdef VERBOSE
		Serial.print("+CRC16");
#endif
	}
#ifdef VERBOSE
	Serial.write('\n');
#endif

	for (int i=0; i<expectedLen; i++)
		if (dataFile.available())
			chunk.readBuffer[i] = dataFile.read();
		else {
			Serial.println("!!! NOT ENOUGH DATA");
			return(false);
		}

	// PRINT CSV HEADERS IF THEY CHANGE
	if (previousContent != content) {
		Serial.print(SAT_MILLIS_HEADER);

		if (content & AVAILABLE_SAT_LUM) {
			Serial.print(SAT_LUM_HEADER);
		}
		if (content & AVAILABLE_SAT_MAG) {
			Serial.print(SAT_MAG_HEADER);
		}
		if (content & AVAILABLE_SAT_TMP) {
			Serial.print(SAT_TMP_HEADER);
		}
		if (content & AVAILABLE_SAT_INFRATHERM) {
			Serial.print(SAT_INFRATHERM_HEADER);
		}
		if (content & AVAILABLE_SAT_ACCEL) {
			Serial.print(SAT_ACCEL_HEADER);
		}
		if (content & AVAILABLE_SAT_GYRO) {
			Serial.print(SAT_GYRO_HEADER);
		}
		if (content & AVAILABLE_SAT_GEIGER) {
			Serial.print(SAT_GEIGER_HEADER);
		}
		if (content & AVAILABLE_CRC16) {
			Serial.print(CRC16_HEADER);
		}
		Serial.write('\n');

		previousContent = content;
	}

	int decodeIndex = 0;

	struct _dataMILLIS * ptr =  (struct _dataMILLIS *)(chunk.readBuffer+decodeIndex);
	decodeIndex += sizeof(struct _dataMILLIS);
	Serial.print((uint32_t)ptr->ms);
	Serial.write(';');

	if (content & AVAILABLE_SAT_LUM) {
		struct _dataLUM * ptr =  (struct _dataLUM *)(chunk.readBuffer+decodeIndex);
		decodeIndex += sizeof(struct _dataLUM);
		Serial.print((int16_t)ptr->tsl_one_values[0]);
		Serial.write(';');
		Serial.print((int16_t)ptr->tsl_one_values[1]);
		Serial.write(';');
		Serial.print((int16_t)ptr->tsl_two_values[0]);
		Serial.write(';');
		Serial.print((int16_t)ptr->tsl_two_values[1]);
		Serial.write(';');
	}

	if (content & AVAILABLE_SAT_MAG) {
		struct _dataMAG * ptr =  (struct _dataMAG *)(chunk.readBuffer+decodeIndex);
		decodeIndex += sizeof(struct _dataMAG);
		Serial.print((int16_t)ptr->mag_values[0]);
		Serial.write(';');
		Serial.print((int16_t)ptr->mag_values[1]);
		Serial.write(';');
		Serial.print((int16_t)ptr->mag_values[2]);
		Serial.write(';');
	}

	if (content & AVAILABLE_SAT_TMP) {
		struct _dataTMP * ptr =  (struct _dataTMP *)(chunk.readBuffer+decodeIndex);
		decodeIndex += sizeof(struct _dataTMP);
		for (int i=0; i<4; i++) {
			Serial.print((int16_t)ptr->temp_values[i]);
			Serial.write(';');
		}
	}

	if (content & AVAILABLE_SAT_INFRATHERM) {
		struct _dataINFRATHERM * ptr =  (struct _dataINFRATHERM *)(chunk.readBuffer+decodeIndex);
		decodeIndex += SAT_INFRATHERM_SIZE; // sizeof(struct _dataINFRATHERM);
		Serial.print((int16_t)ptr->infrat_value);
		Serial.write(';');
	}

	if (content & AVAILABLE_SAT_ACCEL) {
		struct _dataACCEL * ptr =  (struct _dataACCEL *)(chunk.readBuffer+decodeIndex);
		decodeIndex += SAT_ACCEL_SIZE; // sizeof(struct _dataACCEL);
		Serial.print((int16_t)ptr->accel_x);
		Serial.write(';');
		Serial.print((int16_t)ptr->accel_y);
		Serial.write(';');
		Serial.print((int16_t)ptr->accel_z);
		Serial.write(';');
	}

	if (content & AVAILABLE_SAT_GYRO) {
		struct _dataGYRO * ptr =  (struct _dataGYRO *)(chunk.readBuffer+decodeIndex);
		decodeIndex += SAT_GYRO_SIZE;
		Serial.print((int16_t)ptr->gyro_x);
		Serial.write(';');
		Serial.print((int16_t)ptr->gyro_y);
		Serial.write(';');
		Serial.print((int16_t)ptr->gyro_z);
		Serial.write(';');
	}

	if (content & AVAILABLE_SAT_GEIGER) {
		struct _dataGEIGER * ptr =  (struct _dataGEIGER *)(chunk.readBuffer+decodeIndex);
		decodeIndex += SAT_GEIGER_SIZE;
// TODO
	}

	if (content & AVAILABLE_CRC16) {
		struct _dataCRC16 * ptr =  (struct _dataCRC16 *)(chunk.readBuffer+decodeIndex);
		decodeIndex += CRC16_SIZE; // sizeof(struct _dataACCEL);

                uint8_t * ptr_t = (uint8_t *)&chunk;
                uint8_t size_t = 2 + expectedLen - 2; // +2 for headers, -2 for crc at the end (kept for readibility)
                uint16_t crc16_t = 0;
                for (int i=0; i<size_t; i++)
                  crc16_t = _crc16_update(crc16_t,ptr_t[i]);

                if (crc16_t == ptr->crc16) {
                  Serial.print(ptr->crc16,HEX);
                  Serial.print(";OK;");
                } else {
                  Serial.print(ptr->crc16,HEX);
                  Serial.print(";ERR;");
                }
	}
	Serial.write('\n');
	return(true);
}


// *************
// *** SETUP ***
// *************

void setup()
{
	// Open serial communications and wait for port to open:
	Serial.begin(DEBUG_BAUDRATE);
	while (!Serial) {
		; // wait for serial port to connect. Needed for Leonardo only
	}

	Serial.print("Initializing SD card...");

	// make sure that the default chip select pin is set to
	// output, even if you don't use it:
	pinMode(10, OUTPUT);

	// see if the card is present and can be initialized:
	if (!SD.begin(chipSelect)) {
		Serial.println("Card failed, or not present");
		// don't do anything more:
		return;
	}
	Serial.println("card initialized.");

	// open the file. note that only one file can be open at a time,
	// so you have to close this one before opening another.
	dataFile = SD.open(SD_FILENAME);

	if (!dataFile) {
		Serial.print("error opening ");
                Serial.println(SD_FILENAME);
		return;
	}
}

// ************
// *** LOOP ***
// ************

void loop()
{
	// if we're there, it means the file is open
	while (dataFile.available() > 2) {
		byte b = dataFile.read();
		if (b == CHUNK_HEADER_CHAR) {
			byte content = dataFile.read();
                        chunk.header = b;
                        chunk.availableValues = content;
			decodeChunk(content);
		} else {
			Serial.println("!!! HEADER MISMATCH");
			continue;
		}
	}
	dataFile.close();
	while(1);
}


