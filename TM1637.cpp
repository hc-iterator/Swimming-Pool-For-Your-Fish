#include "TM1637.h"

static int8_t TubeTab[] = {0x3f,0x06,0x5b,0x4f,
                           0x66,0x6d,0x7d,0x07,
                           0x7f,0x6f,0x77,0x7c,
                           0x39,0x5e,0x79,0x71};//0~9,A,b,C,d,E,F                        

TM1637::TM1637(uint8_t Clk, uint8_t Data)
{
  Clkpin = Clk;
  Datapin = Data;
  gpio_init(Clkpin);
  gpio_set_dir(Clkpin, GPIO_OUT);
  gpio_init(Datapin);
  gpio_set_dir(Datapin, GPIO_OUT);
  gpio_pull_up(Clkpin);
  gpio_pull_up(Datapin);
}

void TM1637::init(void)
{
  clearDisplay();
}

void TM1637::writeByte(int8_t wr_data)
{
  uint8_t i, count1 = 0;
  for(i = 0; i < 8; i++)
  {
    gpio_put(Clkpin, 0);
    sleep_us(1);
    if(wr_data & 0x01)
      gpio_put(Datapin, 1);
    else
      gpio_put(Datapin, 0);
    sleep_us(1);
    wr_data >>= 1;      
    gpio_put(Clkpin, 1);
    sleep_us(1);
  }  

  // 等待 ACK
  gpio_put(Clkpin, 0);
  sleep_us(1);
  gpio_put(Datapin, 1);
  sleep_us(1);
  gpio_put(Clkpin, 1);
  sleep_us(1);

  gpio_set_dir(Datapin, GPIO_IN);
  while(gpio_get(Datapin))    
  { 
    count1 += 1;
    if(count1 == 2000)
    {
     gpio_set_dir(Datapin, GPIO_OUT);
     gpio_put(Datapin, 0);
     count1 = 0;
    }
    gpio_set_dir(Datapin, GPIO_IN);
  }
  gpio_set_dir(Datapin, GPIO_OUT);
}

void TM1637::start(void)
{
  gpio_put(Clkpin, 1);
  gpio_put(Datapin, 1); 
  sleep_us(1);
  gpio_put(Datapin, 0); 
  sleep_us(1);
  gpio_put(Clkpin, 0); 
  sleep_us(1);
}

void TM1637::stop(void)
{
  gpio_put(Clkpin, 0);
  gpio_put(Datapin, 0);
  sleep_us(1);
  gpio_put(Clkpin, 1);
  sleep_us(1);
  gpio_put(Datapin, 1); 
  sleep_us(1);
}

void TM1637::display(int8_t DispData[])
{
  int8_t SegData[4];
  uint8_t i;
  for(i = 0; i < 4; i ++)
  {
    SegData[i] = DispData[i];
  }
  coding(SegData);
  start();
  writeByte(ADDR_AUTO);
  stop();
  start();
  writeByte(Cmd_SetAddr);
  for(i = 0; i < 4; i ++)
  {
    writeByte(SegData[i]);
  }
  stop();
  start();
  writeByte(Cmd_DispCtrl);
  stop();
}

void TM1637::display(uint8_t BitAddr, int8_t DispData)
{
  int8_t SegData;
  SegData = coding(DispData);
  start();
  writeByte(ADDR_FIXED);
  stop();
  start();
  writeByte(BitAddr | 0xc0);
  writeByte(SegData);
  stop();
  start();
  writeByte(Cmd_DispCtrl);
  stop();
}

void TM1637::displayTime(int8_t hour, int8_t minute)
{
  int8_t data[4];
  int8_t modeHour = hour % 24;
  int8_t modeMinute = minute % 60;
  data[0] = modeHour / 10;
  data[1] = modeHour % 10;
  data[2] = modeMinute / 10;
  data[3] = modeMinute % 10;
  point(true);
  display(data);
  point(false);
}

void TM1637::clearDisplay(void)
{
  display(0x00, 0x7f);
  display(0x01, 0x7f);
  display(0x02, 0x7f);
  display(0x03, 0x7f);  
}

void TM1637::set(uint8_t brightness, uint8_t SetData, uint8_t SetAddr)
{
  Cmd_SetData = SetData;
  Cmd_SetAddr = SetAddr;
  Cmd_DispCtrl = 0x88 + brightness;
}

void TM1637::point(bool PointFlag)
{
  _PointFlag = PointFlag;
}

void TM1637::coding(int8_t DispData[])
{
  uint8_t PointData;
  if(_PointFlag == POINT_ON) PointData = 0x80;
  else PointData = 0; 
  for(uint8_t i = 0; i < 4; i ++)
  {
    if(DispData[i] == 0x7f) DispData[i] = 0x00;
    else DispData[i] = TubeTab[DispData[i]] + PointData;
  }
}

int8_t TM1637::coding(int8_t DispData)
{
  uint8_t PointData;
  if(_PointFlag == POINT_ON) PointData = 0x80;
  else PointData = 0; 
  if(DispData == 0x7f) DispData = 0x00 + PointData;
  else DispData = TubeTab[DispData] + PointData;
  return DispData;
}