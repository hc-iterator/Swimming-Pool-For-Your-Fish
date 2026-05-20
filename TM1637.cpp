#include "TM1637.h"
#include <cstdio>   // 提供 snprintf
#include <cstring>  // 提供 strlen
#include <stdexcept>   // 提供 std::invalid_argument

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

  setDigitCount(4);   // 默认4位
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

// ===================== 切换位数 =====================
void TM1637::setDigitCount(int count)
{
if (count != 4 && count != 6)
        throw std::invalid_argument("TM1637::setDigitCount: count must be 4 or 6");
  digitCount = (count == 6) ? 6 : 4;
  if (digitCount == 6) {
    displayArrayPtr  = &TM1637::display_6;
    displaySinglePtr = &TM1637::display_6;
    displayTimePtr   = &TM1637::displayTime_6;
    clearDisplayPtr  = &TM1637::clearDisplay_6;
    codingArrayPtr   = &TM1637::coding_6;
    codingSinglePtr  = &TM1637::coding_6;
  } else {
    displayArrayPtr  = &TM1637::display_4;
    displaySinglePtr = &TM1637::display_4;
    displayTimePtr   = &TM1637::displayTime_4;
    clearDisplayPtr  = &TM1637::clearDisplay_4;
    codingArrayPtr   = &TM1637::coding_4;
    codingSinglePtr  = &TM1637::coding_4;
  }
}

// ===================== 4 位版本实现 =====================
void TM1637::display_4(int8_t DispData[])
{
  int8_t SegData[4];
  for(int i = 0; i < 4; i++) SegData[i] = DispData[i];
  coding_4(SegData);
  start();
  writeByte(ADDR_AUTO);
  stop();
  start();
  writeByte(Cmd_SetAddr);
  for(int i = 0; i < 4; i++) writeByte(SegData[i]);
  stop();
  start();
  writeByte(Cmd_DispCtrl);
  stop();
}

void TM1637::display_4(uint8_t BitAddr, int8_t DispData)
{
  int8_t SegData = coding_4(DispData);
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

void TM1637::displayTime_4(int8_t hour, int8_t minute)
{
  int8_t data[4];
  data[0] = hour / 10;
  data[1] = hour % 10;
  data[2] = minute / 10;
  data[3] = minute % 10;
  point(true);
  display_4(data);
  point(false);
}

void TM1637::clearDisplay_4(void)
{
  for (int i = 0; i < 4; i++) display_4(i, 0x7f);
}

void TM1637::coding_4(int8_t DispData[])
{
  uint8_t PointData = (_PointFlag == POINT_ON) ? 0x80 : 0;
  for(int i = 0; i < 4; i++) {
    if (DispData[i] == 0x7f) DispData[i] = 0x00;
    else DispData[i] = TubeTab[DispData[i]] + PointData;
  }
}

int8_t TM1637::coding_4(int8_t DispData)
{
  uint8_t PointData = (_PointFlag == POINT_ON) ? 0x80 : 0;
  if (DispData == 0x7f) return 0x00 + PointData;
  return TubeTab[DispData] + PointData;
}

// ===================== 6 位版本实现 =====================
void TM1637::display_6(int8_t DispData[])
{
  int8_t SegData[6];
  for(int i = 0; i < 6; i++) SegData[i] = DispData[i];
  coding_6(SegData);                     // 按逻辑顺序编码（小数点等已处理）

  // 按物理顺序发送：3,2,1,6,5,4 → 索引 2,1,0,5,4,3
  const int8_t phys_order[6] = {2, 1, 0, 5, 4, 3};
  int8_t SegForSend[6];
  for (int i = 0; i < 6; i++) SegForSend[i] = SegData[phys_order[i]];

  start();
  writeByte(ADDR_AUTO);
  stop();
  start();
  writeByte(Cmd_SetAddr);
  for(int i = 0; i < 6; i++) writeByte(SegForSend[i]);
  stop();
  start();
  writeByte(Cmd_DispCtrl);
  stop();
}

void TM1637::setDot(uint8_t bit, bool on) {
    if (bit < 6) dots[bit] = on;
}
void TM1637::clearDots() {
    for (auto &d : dots) d = false;
}

void TM1637::display_6(uint8_t BitAddr, int8_t DispData)
{
  // 编码基础段码（不含小数点）
  int8_t SegData = coding_6(DispData);   // 调用单个编码，返回不带点的段码
  // 如果该位需要点亮小数点，则在段码上叠加 0x80
  if (dots[BitAddr]) SegData |= 0x80;

  start();
  writeByte(ADDR_FIXED);
  stop();
  start();
  writeByte(BitAddr | 0xc0);   // 注意：BitAddr 是物理地址（与硬件顺序一致）
  writeByte(SegData);
  stop();
  start();
  writeByte(Cmd_DispCtrl);
  stop();
}

void TM1637::displayTime_6(int8_t hour, int8_t minute)
{
  int8_t data[6] = {0};
  data[0] = hour / 10;
  data[1] = hour % 10;
  data[2] = minute / 10;
  data[3] = minute % 10;
  // 秒位默认显示00，或可扩展，这里保持占位
  point(true);
  display_6(data);
  point(false);
}

void TM1637::clearDisplay_6(void)
{
  for (int i = 0; i < 6; i++) display_6(i, 0x7f);
}

void TM1637::coding_6(int8_t DispData[]) {
    for (int i = 0; i < 6; i++) {
        if (DispData[i] == 0x7f) DispData[i] = 0x00;
        else {
            uint8_t seg = TubeTab[DispData[i]];
            if (dots[i]) seg |= 0x80;      // 点亮该位小数点
            DispData[i] = seg;
        }
    }
}

int8_t TM1637::coding_6(int8_t DispData)
{
  uint8_t PointData = (_PointFlag == POINT_ON) ? 0x80 : 0;
  if (DispData == 0x7f) return 0x00 + PointData;
  return TubeTab[DispData] + PointData;
}

// ===================== 原有公共接口 =====================
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

// ===================== 6位浮点数显示 =====================
void TM1637::displayFloat(double num)
{
    // 4位模式下报错退出
    if (digitCount != 6) {
        return;  // 静默退出，不干扰4位模式正常工作
    }

    // 1. 格式化浮点数为字符串（最多保留9位小数，避免精度丢失）
    char buf[32];
    snprintf(buf, sizeof(buf), "%.9f", num);

    // 2. 去掉末尾多余的零和小数点
    // 比如 "25.100000000" -> "25.1"，"25.000000000" -> "25"
    char* p = buf + strlen(buf) - 1;
    while (*p == '0' && p > buf) p--;
    if (*p == '.') p--;  // 如果小数点后全是零，连小数点也去掉
    *(p + 1) = '\0';

    int len = strlen(buf);
    int8_t data[6] = {0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f};  // 全部默认熄灭

    if (len <= 6) {
        // 3. 总位数 ≤6：右对齐，左边留空
        int offset = 6 - len;
        for (int i = 0; i < len; i++) {
            char c = buf[i];
            if (c == '.') {
                // 小数点：点亮前一位的小数点
                if (offset + i - 1 >= 0) {
                    data[offset + i - 1] |= 0x80;
                }
            } else if (c == '-') {
                // 负号：显示为G段（中间横杠），即段码 0x40
                data[offset + i] = 0x40;
            } else if (c >= '0' && c <= '9') {
                data[offset + i] = TubeTab[c - '0'];
            }
        }
    } else {
        // 4. 总位数 >6：从左开始截取6位显示
        int shown = 0;  // 已经放入data的位数
        for (int i = 0; i < len && shown < 6; i++) {
            char c = buf[i];
            if (c == '.') {
                // 小数点：点亮前一位的小数点
                if (shown - 1 >= 0) {
                    data[shown - 1] |= 0x80;
                }
                // 小数点本身不占用显示位
            } else if (c == '-') {
                data[shown++] = 0x40;  // 负号占用一位
            } else if (c >= '0' && c <= '9') {
                data[shown++] = TubeTab[c - '0'];
            }
        }
    }

    // 5. 发送到数码管
    display_6(data);
}
