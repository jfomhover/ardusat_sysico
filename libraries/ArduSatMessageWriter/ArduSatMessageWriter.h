#ifndef ArduSatMessageWriter_H
#define ArduSatMessageWriter_H

#include <Arduino.h>

class ArduSatMessageWriter
{
public:
  ArduSatMessageWriter(char * charBuffer, int bufferLength, int format, int schemeLength);

#define MESSAGE_FORMAT_TEXT    0    // plain ascii (csv compatible)
#define MESSAGE_FORMAT_HEX     1    // full HEX
#define MESSAGE_FORMAT_XHEX    2    // compressed HEX

  void setFormat(int format);
  void zero();                      // resets the handler, put all buffer and scheme chars to '\0'
  char * getScheme();               // returns the "scheme" of the message (format)
  char * getMessage();              // returns the message

  int add(int8_t val);            // add a value of this type to the message
  int add(uint8_t val);            // add a value of this type to the message
  int add(int16_t val);            // add a value of this type to the message
  int add(uint16_t val);           // add a value of this type to the message
  int add(int32_t val);               // add a value of this type to the message
  int add(uint32_t val);               // add a value of this type to the message
  int add(float val, int minNumber, int precision);              // add a value of this type to the message
  int add(int32_t val, int bytes);     // add a value of this type to the message

                                   // below : return values for the "add" functions
#define MESSAGE_ADD_OK  0
#define MESSAGE_ADD_ERROR  -1
#define MESSAGE_ADD_ERROR_BUFFEROVERFLOW  -2
#define MESSAGE_ADD_ERROR_SCHEMEOVERFLOW  -3

private:
  int canAdd(int b);               // checks if b bytes can be added to the message
  void compress();                  // compresses the message
  void replaceSuccessive(char c, char matrix[8]);  // used for compression
  uint8_t messageType;
  char * buffer;
  int bufferLen,bufferMaxLen;
  char * scheme;
  int schemeLen,schemeMaxLen;
  char * message;
};

#define MESSAGE_SCHEME_INT8	'b'
#define MESSAGE_SCHEME_UINT8	'B'
#define MESSAGE_SCHEME_INT16	'I'
#define MESSAGE_SCHEME_UINT16	'U'
#define MESSAGE_SCHEME_FLOAT	'F'
#define MESSAGE_SCHEME_INT24	'E'
#define MESSAGE_SCHEME_INT32	'l'
#define MESSAGE_SCHEME_UINT32	'L'

#endif /* ArduSatMessageWriter_H */
