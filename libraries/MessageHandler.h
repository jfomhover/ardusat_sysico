#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include <Arduino.h>

class MessageHandler
{
public:
  MessageHandler(char * charBuffer, int bufferLength, char * schemeBuffer, int schemeLength);


  void zero();                      // resets the handler, put all buffer and scheme chars to '\0'
  char * getScheme();               // returns the "scheme" of the message (format)
  char * getMessage();              // returns the message
  void compress();                  // compresses the message
//  void uncompress();
  void setMessageType(uint8_t format);
#define MESSAGE_FORMAT_TEXT    0
#define MESSAGE_FORMAT_HEX     1
#define MESSAGE_FORMAT_XHEX    2
#define MESSAGE_FORMAT_BEST    3    // BEWARE : if you choose this format, the buffer will be split in 3 to compute all formats

  int add(int16_t val);            // add a value of this type to the message
  int add(long val);               // add a value of this type to the message
  int add(float val);              // add a value of this type to the message
  int add(uint16_t val);           // add a value of this type to the message
  int add(int val, int bytes);     // add a value of this type to the message

                                   // below : return values for the "add" functions
#define MESSAGE_ADD_OK  0
#define MESSAGE_ADD_ERROR  -1
#define MESSAGE_ADD_ERROR_BUFFEROVERFLOW  -2
#define MESSAGE_ADD_ERROR_SCHEMEOVERFLOW  -3

private:
  int canAdd(int b);               // checks if b bytes can be added to the message
  void replaceSuccessive(char c, char matrix[8]);  // used for compression
  uint8_t messageType;
  char * buffer;
  int bufferLen,bufferMaxLen;
  char * scheme;
  int schemeLen,schemeMaxLen;
  char * message;
};

MessageHandler::MessageHandler(char * charBuffer, int bufferLength, char * schemeBuffer, int schemeLength) {
  buffer = charBuffer;
  bufferMaxLen = bufferLength;
  bufferLen=0;
  scheme = schemeBuffer;
  schemeMaxLen = schemeLength;  
  schemeLen=0;
};

void MessageHandler::zero() {
  for (int i=0; i<bufferLen; i++)
    buffer[i]='\0';
  for (int i=0; i<schemeLen; i++)
    scheme[i]='\0';
  bufferLen=0;
  schemeLen=0;
};

char * MessageHandler::getMessage() {
  return(buffer);
};

char * MessageHandler::getScheme() {
  return(scheme);
};

void MessageHandler::setMessageType(uint8_t type) {
  messageType = type;
};

void MessageHandler::replaceSuccessive(char c, char matrix[8]) {
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

void MessageHandler::compress() {
  char * current = buffer;
  
  replaceSuccessive('F',compressionMatrixEff);
  replaceSuccessive('0',compressionMatrixZero);
};

int MessageHandler::canAdd(int b) {
  if (bufferLen >= (bufferMaxLen-b-1))
    return(MESSAGE_ADD_ERROR_BUFFEROVERFLOW);
  if (schemeLen >= (schemeMaxLen-2))
    return(MESSAGE_ADD_ERROR_SCHEMEOVERFLOW);
  return(MESSAGE_ADD_OK);
};


int MessageHandler::add(int16_t val) {
  int ret = canAdd(4);
  if (ret != MESSAGE_ADD_OK)
    return(ret);
  int16_t t_val = val;
  t_val = t_val & 0x0000FFFF;
  scheme[schemeLen++] = 'I';
  sprintf(buffer+bufferLen, "%04X", val);  bufferLen+=4;
  return(MESSAGE_ADD_OK);
};

int MessageHandler::add(uint16_t val) {
  int ret = canAdd(4);
  if (ret != MESSAGE_ADD_OK)
    return(ret);
    
  scheme[schemeLen++] = 'U';
  sprintf(buffer+bufferLen, "%04X", val);  bufferLen+=4;
  return(MESSAGE_ADD_OK);
};

int MessageHandler::add(long val) {
  int ret = canAdd(8);
  if (ret != MESSAGE_ADD_OK)
    return(ret);

  scheme[schemeLen++] = 'L';
  sprintf(buffer+bufferLen, "%08X", val);  bufferLen+=8;
  return(MESSAGE_ADD_OK);
};

int MessageHandler::add(float val) {
  int ret = canAdd(8);
  if (ret != MESSAGE_ADD_OK)
    return(ret);

  scheme[schemeLen++] = 'F';

  float f_var = val;
  byte byte_array[4];
  memcpy(byte_array,&f_var,4);
  for (int i=0; i<4; i++) {
    sprintf(buffer+bufferLen, "%02X", byte_array[i]);  bufferLen+=2;
  }
  
  return(MESSAGE_ADD_OK);
};

int MessageHandler::add(int val, int bytes) {
  int ret = canAdd(bytes);
  int t_val = val;
  if (ret != MESSAGE_ADD_OK)
    return(ret);
    
    scheme[schemeLen++] = 'I';
    switch(bytes) {
    case 1:
      scheme[schemeLen++] = '2';
      t_val = t_val & 0x000000FF;
      sprintf(buffer+bufferLen, "%02X", t_val);  bufferLen+=2;
      break;
    case 2:
      //scheme[schemeLen++] = '4';
      t_val = t_val & 0x0000FFFF;
      sprintf(buffer+bufferLen, "%04X", t_val);  bufferLen+=4;
      break;
    case 3:
      scheme[schemeLen++] = '6';
      t_val = t_val & 0x00FFFFFF;
      sprintf(buffer+bufferLen, "%06X", t_val);  bufferLen+=6;
      break;
    case 4:
      scheme[schemeLen++] = '8';
      sprintf(buffer+bufferLen, "%08X", t_val);  bufferLen+=8;
      break;
    default:
      return(MESSAGE_ADD_ERROR);
  };
  return(MESSAGE_ADD_OK);
};
 
#endif /* MESSAGEHANDLER_H */

