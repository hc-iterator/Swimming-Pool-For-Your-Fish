#ifndef TM1637_h
#define TM1637_h

#include <pico/stdlib.h>

#define ADDR_AUTO  0x40
#define ADDR_FIXED 0x44
#define STARTADDR  0xc0 

#define POINT_ON   1
#define POINT_OFF  0

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

    // 6位浮点数显示，4位模式报错
    void displayFloat(double num);
    void setDot(uint8_t bit, bool on);
    void clearDots();

    // 切换位数
    void setDigitCount(int count);

    // 包装函数（对外接口不变）
    void display(int8_t DispData[])           { (this->*displayArrayPtr)(DispData); }
    void display(uint8_t BitAddr, int8_t DispData) { (this->*displaySinglePtr)(BitAddr, DispData); }
    void displayTime(int8_t hour, int8_t minute)   { (this->*displayTimePtr)(hour, minute); }
    void clearDisplay(void)                   { (this->*clearDisplayPtr)(); }
    void coding(int8_t DispData[])            { (this->*codingArrayPtr)(DispData); }
    int8_t coding(int8_t DispData)            { return (this->*codingSinglePtr)(DispData); }

    void set(uint8_t = BRIGHT_TYPICAL, uint8_t = 0x40, uint8_t = 0xc0);
    void point(bool PointFlag);

    // ---- 4位版本（后缀 _4） ----
    void display_4(int8_t DispData[]);
    void display_4(uint8_t BitAddr, int8_t DispData);
    void displayTime_4(int8_t hour, int8_t minute);
    void clearDisplay_4(void);
    void coding_4(int8_t DispData[]);
    int8_t coding_4(int8_t DispData);

    // ---- 6位版本（后缀 _6） ----
    void display_6(int8_t DispData[]);
    void display_6(uint8_t BitAddr, int8_t DispData);
    void displayTime_6(int8_t hour, int8_t minute);
    void clearDisplay_6(void);
    void coding_6(int8_t DispData[]);
    int8_t coding_6(int8_t DispData);

private:
    uint8_t Clkpin;
    uint8_t Datapin;
    bool dots[6] = {false};               // 记录每位小数点状态

    // ---- 4位/6位共存的函数指针 ----
    typedef void (TM1637::*DisplayArrayFunc)(int8_t[]);
    typedef void (TM1637::*DisplaySingleFunc)(uint8_t, int8_t);
    typedef void (TM1637::*DisplayTimeFunc)(int8_t, int8_t);
    typedef void (TM1637::*VoidFunc)();
    typedef void (TM1637::*CodingArrayFunc)(int8_t[]);
    typedef int8_t (TM1637::*CodingSingleFunc)(int8_t);

    int digitCount;
    DisplayArrayFunc  displayArrayPtr;
    DisplaySingleFunc displaySinglePtr;
    DisplayTimeFunc   displayTimePtr;
    VoidFunc          clearDisplayPtr;
    CodingArrayFunc   codingArrayPtr;
    CodingSingleFunc  codingSinglePtr;
};

#endif