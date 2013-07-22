/* 
    SAT_MagEMU.cpp - Library for Arduino returns x,y,z data on current magnetic field
    Copyright (C) 2012  Jeroen Cappaert & Lara Booth for NanoSatisfi

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

////////////////////////////////////////////////////////////////////////////////
Notes:

Designed for use with Freescale (C) MAG3110 Magnetometer

Tested with MAG3110 Breakout from Sparkfun and Arduino Uno

Based on Sparkfun's example for the MAG3110 breakout board:
http://dlnmh9ip6v2uc.cloudfront.net/datasheets/BreakoutBoards/Mag3110_v10.pde

////////////////////////////////////////////////////////////////////////////////
*/

#include "SAT_MagEMU.h"
#include <I2C_add.h>
#include <Wire.h>

//Constructor 
SAT_MagEMU::SAT_MagEMU()
{
    /* 
       The magnetometer has to be calibrated before use
       use Arduino program "Offsets.ino" to find a value for:
       min_x, max_x, min_y, max_y, min_z, max_z
    */

    mag_x_scale= 1.0/(-902 +203); //offset scale factor: 1.0/(max_x - min_x)
    mag_y_scale= 1.0/(365 + 337);    //offset scale factor: 1.0/(max_y - min_y)
    mag_z_scale= 1.0/(2872 - 2722);  //offset scale factor: 1.0/(max_z - min_z)
}

void SAT_MagEMU::init(uint8_t node_id){
    _local_address = node_id;
    this->configMag();
}

// Configure magnetometer
// call in setup()
void SAT_MagEMU::configMag() {
  delay(15);
}

// read X value
int SAT_MagEMU::readx()
{
  return 0x0408;
}

//read Y value
int SAT_MagEMU::ready()
{
  return 0x1516;
}

// read Z value
int SAT_MagEMU::readz()
{
  return 0x2342;
}

/*
   the following methods: x_value(), y_value(), and z_value() 
   require offset values to be entered before use
   see constructor
*/

//return scaled x-value
float SAT_MagEMU::x_value()
{
  return (readx()*mag_x_scale);
}

//return scaled y-value
float SAT_MagEMU::y_value()
{
  return (ready()*mag_y_scale);
}

//return scaled z-value
float SAT_MagEMU::z_value()
{
  return (readz()*mag_z_scale);
}

/*
  getHeading(...) returns heading
  heading: difference in degrees between current angle and true North
  ------------------------------------------------------------------
  use calibrated x_value(), y_value(), z_value() as parameters x,y,z
  ------------------------------------------------------------------
  method from Sparkfun Forum on MAG3110 magnetometer
*/

int SAT_MagEMU::getHeading(float x, float y, float z) {
  float heading=0;

  //defines value of heading for various x,y,z values
  if ((x == 0)&&(y < 0))
    heading= PI/2.0; 

  if ((x == 0)&&(y > 0))
    heading=3.0*PI/2.0;

  if (x < 0)  
    heading = PI - atan(y/x);

  if ((x > 0)&&(y < 0))
    heading = -atan(y/x);

  if ((x > 0)&&(y > 0))
    heading = 2.0*PI - atan(y/x);
  //convert heading from radians to degrees and return
  return  int(degrees(heading));
}
