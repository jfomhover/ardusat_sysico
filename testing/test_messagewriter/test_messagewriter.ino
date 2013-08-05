/*
    test_messagewriter.ino - testing the ArduSatMessageWriter class with several example values
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

    TODOLIST

*/


// **************
// *** CONFIG ***
// **************

#define MESSAGE_BUFFER_SIZE  256  // only 24 chars needed in our case (18 hex chars actually)
#define MESSAGE_SCHEME_MAXSIZE  32 // the number of chars used to describe the format of the message


// ***************
// *** HEADERS ***
// ***************

#include <ArduSatMessageWriter.h>

char messageBuffer[MESSAGE_BUFFER_SIZE];  // buffer for organising the message to be sent to earth
ArduSatMessageWriter messageHdl(messageBuffer,            // argument 1 : available char[] as buffer
                          MESSAGE_BUFFER_SIZE,      // argument 2 : the size of the buffer
                          MESSAGE_FORMAT_TEXT,       // argument 3 : the format of the message (see MessageWriter.h)
                          MESSAGE_SCHEME_MAXSIZE);  // argument 4 : the size of the scheme

struct testingValues {
  uint32_t test1;
  int32_t test2;
  uint16_t test3;
  int16_t test4;
  uint8_t test5;
  int8_t test6;
  float test7;
};

struct testingValues values1 = { 0xFED00A09, 0xF00BCA98, 0xF765, 0xF432, 0xF0, 0xF0, 3.1415 };
char referenceText1[] = "4275046921;-267662696;63333;-3022;240;-16;3.14;";
char referenceHex1[] = "FED00A09F00BCA98F765F432F0F040490E56";
char referenceXhex1[] = "FEDGA09FGBCA98F765F432F0F040490E56";
char referenceScheme1[] = "LlUIBbF";

struct testingValues values2 = { 0x48151623, 0x78543210, 0x1084, 0x3208, 26, -27, 483.18 };
char referenceText2[] = "1209341475;2018783760;4228;12808;26;-27;483.18;";
char referenceHex2[] = "4815162378543210108432081AE543F1970A";
//                        4815162378543210108432081AE543F1970A
char referenceXhex2[] = "4815162378543210108432081AE543F1970A";
char referenceScheme2[] = "LlUIBbF";

void prepareWriter(ArduSatMessageWriter * writer, struct testingValues * values) {
  writer->zero();
  
  writer->add(values->test1);
  writer->add(values->test2);
  writer->add(values->test3);
  writer->add(values->test4);
  writer->add(values->test5);
  writer->add(values->test6);
  writer->add(values->test7,3,2);
}

void testMessageWriter(ArduSatMessageWriter * writer, int format, struct testingValues * values, char * referenceMessage, char * referenceScheme) {
  writer->zero();
  writer->setFormat(format);
  prepareWriter(writer, values);
  
  Serial.print("SCHEME: ");
  Serial.print(writer->getScheme());    // should output : LlUIBbF

  if (strcmp(writer->getScheme(), referenceScheme) == 0)
    Serial.println(" OK");
  else
    Serial.println(" ERROR !!!");
  
  Serial.print("MESSAGE: ");
  Serial.print(writer->getMessage());    // should output : 4275816969;-19150184;63333;-3022;240;-16;3.14;

  if (strcmp(writer->getMessage(), referenceMessage) == 0)
    Serial.println(" OK");
  else
    Serial.println(" ERROR !!!");
};

// *************
// *** SETUP ***
// *************

void setup()
{
  Serial.begin(115200);  // start serial for output (fast)
}

// ************
// *** LOOP ***
// ************


void loop()
{
  testMessageWriter(&messageHdl, MESSAGE_FORMAT_TEXT, &values1, referenceText1, referenceScheme1);
  testMessageWriter(&messageHdl, MESSAGE_FORMAT_HEX, &values1, referenceHex1, referenceScheme1);
  testMessageWriter(&messageHdl, MESSAGE_FORMAT_XHEX, &values1, referenceXhex1, referenceScheme1);
  
  testMessageWriter(&messageHdl, MESSAGE_FORMAT_TEXT, &values2, referenceText2, referenceScheme2);
  testMessageWriter(&messageHdl, MESSAGE_FORMAT_HEX, &values2, referenceHex2, referenceScheme2);
  testMessageWriter(&messageHdl, MESSAGE_FORMAT_XHEX, &values2, referenceXhex2, referenceScheme2);

  while(1) {
    delay(1000);
  };
}

