#include <Arduino.h>
#include "ArduSatMessageWriter.h"

ArduSatMessageWriter::ArduSatMessageWriter(char * charBuffer, int bufferLength, int format, int schemeLength) {
  // SCHEME BUFFER starts at the beginning of the allocated space
  scheme = charBuffer;
  schemeMaxLen = schemeLength;  
  schemeLen=0;
  // TEXT BUFFER gets all the rest
  buffer = charBuffer+schemeMaxLen;
  bufferMaxLen = bufferLength-schemeMaxLen;
  bufferLen=0;

  messageType = format;
};

void ArduSatMessageWriter::setFormat(int format) {
	messageType = format;
};

void ArduSatMessageWriter::zero() {
  for (int i=0; i<bufferLen; i++)
    buffer[i]='\0';
  for (int i=0; i<schemeLen; i++)
    scheme[i]='\0';
  bufferLen=0;
  schemeLen=0;
};

char * ArduSatMessageWriter::getMessage() {
  if (messageType == MESSAGE_FORMAT_XHEX)
    compress();
  return(buffer);
};

char * ArduSatMessageWriter::getScheme() {
  return(scheme);
};


// *** PRINTING SIGNED INT 8 BITS VALUES ***
int ArduSatMessageWriter::add(int8_t val) {
  int ret = canAdd(2);
  if (ret != MESSAGE_ADD_OK)
    return(ret);

  int8_t t_val = val;
  scheme[schemeLen++] = MESSAGE_SCHEME_INT8;
  
  switch(messageType) {
    case MESSAGE_FORMAT_TEXT:
      sprintf(buffer+bufferLen, "%i;", t_val);  bufferLen=strlen(buffer);
      break;
    case MESSAGE_FORMAT_HEX:
    case MESSAGE_FORMAT_XHEX:
    default:
      sprintf(buffer+bufferLen, "%02X", (byte)t_val);  bufferLen+=2;      
      break;
  };
  
  return(MESSAGE_ADD_OK);
};

// *** PRINTING UNSIGNED INT 8 BITS VALUES ***
int ArduSatMessageWriter::add(uint8_t val) {
  int ret = canAdd(2);
  if (ret != MESSAGE_ADD_OK)
    return(ret);
    
  scheme[schemeLen++] = MESSAGE_SCHEME_UINT8;
  
  switch(messageType) {
    case MESSAGE_FORMAT_TEXT:
      sprintf(buffer+bufferLen, "%u;", val);  bufferLen=strlen(buffer);
      break;
    case MESSAGE_FORMAT_HEX:
    case MESSAGE_FORMAT_XHEX:
    default:
      sprintf(buffer+bufferLen, "%02X", (byte)val);  bufferLen+=2;      
      break;
  };

  return(MESSAGE_ADD_OK);
};

// *** PRINTING SIGNED INT 16 BITS VALUES ***
int ArduSatMessageWriter::add(int16_t val) {
  int ret = canAdd(4);
  if (ret != MESSAGE_ADD_OK)
    return(ret);

  int16_t t_val = val;
  t_val = t_val & 0x0000FFFF;
  scheme[schemeLen++] = MESSAGE_SCHEME_INT16;
  
  switch(messageType) {
    case MESSAGE_FORMAT_TEXT:
      sprintf(buffer+bufferLen, "%i;", val);  bufferLen=strlen(buffer);
      break;
    case MESSAGE_FORMAT_HEX:
    case MESSAGE_FORMAT_XHEX:
    default:
      sprintf(buffer+bufferLen, "%04X", (uint16_t)val);  bufferLen+=4;      
      break;
  };
  
  return(MESSAGE_ADD_OK);
};

// *** PRINTING UNSIGNED INT 16 BITS VALUES ***
int ArduSatMessageWriter::add(uint16_t val) {
  int ret = canAdd(4);
  if (ret != MESSAGE_ADD_OK)
    return(ret);
    
  scheme[schemeLen++] = MESSAGE_SCHEME_UINT16;
  
  switch(messageType) {
    case MESSAGE_FORMAT_TEXT:
      sprintf(buffer+bufferLen, "%u;", val);  bufferLen=strlen(buffer);
      break;
    case MESSAGE_FORMAT_HEX:
    case MESSAGE_FORMAT_XHEX:
    default:
      sprintf(buffer+bufferLen, "%04X", (uint16_t)val);  bufferLen+=4;      
      break;
  };

  return(MESSAGE_ADD_OK);
};

// *** PRINTING LONG INT VALUES ***
int ArduSatMessageWriter::add(int32_t val) {
  int ret = canAdd(8);
  if (ret != MESSAGE_ADD_OK)
    return(ret);

  scheme[schemeLen++] = MESSAGE_SCHEME_INT32;
  
  switch(messageType) {
    case MESSAGE_FORMAT_TEXT:
      sprintf(buffer+bufferLen, "%li;", val);  bufferLen=strlen(buffer);
      break;
    case MESSAGE_FORMAT_HEX:
    case MESSAGE_FORMAT_XHEX:
    default:
      long l_var = val;
      byte byte_array[4];
      memcpy(byte_array,&l_var,4);
      for (int i=3; i>=0; i--) {
        sprintf(buffer+bufferLen, "%02X", byte_array[i]);  bufferLen+=2;
      }
      break;
  };  

  return(MESSAGE_ADD_OK);
};

int ArduSatMessageWriter::add(uint32_t val) {
  int ret = canAdd(8);
  if (ret != MESSAGE_ADD_OK)
    return(ret);

  scheme[schemeLen++] = MESSAGE_SCHEME_UINT32;
  
  switch(messageType) {
    case MESSAGE_FORMAT_TEXT:
      sprintf(buffer+bufferLen, "%lu;", val);  bufferLen=strlen(buffer);
      break;
    case MESSAGE_FORMAT_HEX:
    case MESSAGE_FORMAT_XHEX:
    default:
      long l_var = val;
      byte byte_array[4];
      memcpy(byte_array,&l_var,4);
      for (int i=3; i>=0; i--) {
        sprintf(buffer+bufferLen, "%02X", byte_array[i]);  bufferLen+=2;
      }
      break;
  };  

  return(MESSAGE_ADD_OK);
};

// *** PRINTING FLOAT VALUES ***
int ArduSatMessageWriter::add(float val, int minNumber, int precision) {
  int ret = canAdd(8);
  if (ret != MESSAGE_ADD_OK)
    return(ret);

  scheme[schemeLen++] = MESSAGE_SCHEME_FLOAT;

  switch(messageType) {
    case MESSAGE_FORMAT_TEXT:
      dtostrf(val,minNumber, precision, buffer+bufferLen); bufferLen=strlen(buffer);
      buffer[bufferLen++]=';';
      //sprintf(buffer+bufferLen, "%f;", val);  bufferLen=strlen(buffer);
      break;
    case MESSAGE_FORMAT_HEX:
    case MESSAGE_FORMAT_XHEX:
    default:
      float f_var = val;
      byte byte_array[4];
      memcpy(byte_array,&f_var,4);
      for (int i=3; i>=0; i--) {
//      for (int i=0; i<4; i++) {
        sprintf(buffer+bufferLen, "%02X", byte_array[i]);  bufferLen+=2;
      }    
      break;
  };
  
  return(MESSAGE_ADD_OK);
};

int ArduSatMessageWriter::add(long val, int bytes) {
  int ret = canAdd(bytes);
  int t_val = val;
  if (ret != MESSAGE_ADD_OK)
    return(ret);

  long l_var = val;
  byte byte_array[4];
  memcpy(byte_array,&l_var,4);
    
  if (messageType == MESSAGE_FORMAT_TEXT) {
    // TODO: reducing by mask ?
    scheme[schemeLen++] = MESSAGE_SCHEME_INT32;
    sprintf(buffer+bufferLen, "%li;", val); bufferLen=strlen(buffer);
    return(MESSAGE_ADD_OK);
  }

  
  switch(bytes) {
    case 1:
      scheme[schemeLen++] = MESSAGE_SCHEME_INT8;
      break;
    case 2:
      scheme[schemeLen++] = MESSAGE_SCHEME_INT16;
      break;
    case 3:
      scheme[schemeLen++] = MESSAGE_SCHEME_INT24;
      break;
    case 4:
      scheme[schemeLen++] = MESSAGE_SCHEME_INT32;
      break;
    default:
      return(MESSAGE_ADD_ERROR);
      break;
  };
  
  for (int i=(bytes-1); i>=0; i--) {
    sprintf(buffer+bufferLen, "%02X", byte_array[i]);  bufferLen+=2;
  }
  return(MESSAGE_ADD_OK);
};


// *** PRIVATE METHODS ***

int ArduSatMessageWriter::canAdd(int b) {
  if (messageType != MESSAGE_FORMAT_TEXT) {
    if (bufferLen >= (bufferMaxLen-b-1))
      return(MESSAGE_ADD_ERROR_BUFFEROVERFLOW);
  }
  if (schemeLen >= (schemeMaxLen-1))
    return(MESSAGE_ADD_ERROR_SCHEMEOVERFLOW);
  return(MESSAGE_ADD_OK);
};


void ArduSatMessageWriter::replaceSuccessive(char c, char matrix[8]) {
  int succ = 0;
  char * current = buffer;
  for (int i=0; i<bufferLen; i++) {
    if (buffer[i] == c) {
      succ++;
      if (succ == 8) {
        current[0] = matrix[succ-1];
        succ = 0;
        current++;
      }
    } else {
        if (succ > 0) {
          current[0] = matrix[succ-1];
          current[1]=buffer[i];
          current+=2;
        } else {
          current[0]=buffer[i];
          current++;
        }
        succ = 0;
    }
  }
  if (succ > 0) {
     current[0] = matrix[succ-1];
     current++;
  }
  current[0] = '\0';
};

char compressionMatrixZero[8] = { '0', 'G', 'H', 'I', 'J', 'K', 'L', 'M' };
char compressionMatrixEff[8] = { 'F', 'N', 'O', 'P', 'Q', 'R', 'S', 'T' };

void ArduSatMessageWriter::compress() {
  char * current = buffer;
  
  replaceSuccessive('F',compressionMatrixEff);
  replaceSuccessive('0',compressionMatrixZero);
};