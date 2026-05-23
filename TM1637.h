#ifndef TM1637_h
#define TM1637_h

#include <pico/stdlib.h>
#include <cstdint>

#define ADDR_AUTO  0x40
#define ADDR_FIXED 0x44
#define STARTADDR  0xc0 

#define BRIGHT_DARKEST 0
#define BRIGHT_TYPICAL 2
#define BRIGHTEST      7

// ===================== 前置声明 =====================
class TM1637;

// ===================== 显示配置器（临时对象，RAII） =====================
class DisplayConfigurator {
public:
    // 禁止拷贝
    DisplayConfigurator(const DisplayConfigurator&) = delete;
    DisplayConfigurator& operator=(const DisplayConfigurator&) = delete;

    // 允许移动（如果需要的话）
    DisplayConfigurator(DisplayConfigurator&&) = default;
    DisplayConfigurator& operator=(DisplayConfigurator&&) = default;

    // 析构函数：真正执行显示
    ~DisplayConfigurator();

    // 小数点设置（链式调用）
    DisplayConfigurator& dot(uint8_t bit, bool on = true);

private:
    friend class TM1637;
    
    // 构造函数由 TM1637::display 调用
    DisplayConfigurator(TM1637* parent, int8_t* data, uint8_t len);

    TM1637* owner;       // 指向 TM1637 对象
    int8_t  segData[6];  // 段码副本
    uint8_t dotMask;     // 小数点位掩码，bit0=第0位小数点
    uint8_t dataLen;     // 数据长度（4 或 6）
};

// ===================== TM1637 主类 =====================
class TM1637
{
public:
    uint8_t Cmd_SetData;
    uint8_t Cmd_SetAddr;
    uint8_t Cmd_DispCtrl;

    TM1637(uint8_t, uint8_t);
    void init(void);
    void writeByte(int8_t wr_data);
    void start(void);
    void stop(void);

    // ---- 4位/6位切换 ----
    void setDigitCount(int count);

    // ---- 显示接口（返回临时配置器） ----
    DisplayConfigurator display(int8_t DispData[]);
    DisplayConfigurator display(uint8_t BitAddr, int8_t DispData);

    void clearDisplay(void);
    void set(uint8_t = BRIGHT_TYPICAL, uint8_t = 0x40, uint8_t = 0xc0);
    void coding(int8_t DispData[], uint8_t dotMask = 0);
    int8_t coding(int8_t DispData, bool dot = false);

private:
    uint8_t Clkpin;
    uint8_t Datapin;

    int digitCount;
    
};

#endif