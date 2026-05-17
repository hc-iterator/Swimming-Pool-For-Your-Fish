//  Author:Fred.Chu
//  Date:9 April,2013
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
//  Modified record:
//
/*******************************************************************************/

#ifndef TM1637_h
#define TM1637_h

#include <pico/stdlib.h>

//************definitions for TM1637*********************
#define ADDR_AUTO  0x40
#define ADDR_FIXED 0x44

#define STARTADDR  0xc0 
/**** definitions for the clock point of the digit tube *******/
#define POINT_ON   1
#define POINT_OFF  0
/**************definitions for brightness***********************/
#define  BRIGHT_DARKEST 0
#define  BRIGHT_TYPICAL 2
#define  BRIGHTEST      7

class TM1637
{
  public:
    uint8_t Cmd_SetData;
    uint8_t Cmd_SetAddr;
    uint8_t Cmd_DispCtrl;
    bool _PointFlag;
    
    TM1637(uint8_t, uint8_t);
    void init(void);
    void writeByte(int8_t wr_data);
    void start(void);
    void stop(void);
    void display(int8_t DispData[]);
    void display(uint8_t BitAddr, int8_t DispData);
    void displayTime(int8_t hour, int8_t minute);
    void clearDisplay(void);
    void set(uint8_t = BRIGHT_TYPICAL, uint8_t = 0x40, uint8_t = 0xc0);
    void point(bool PointFlag);
    void coding(int8_t DispData[]); 
    int8_t coding(int8_t DispData); 
    
  private:
    uint8_t Clkpin;
    uint8_t Datapin;
};

#endif