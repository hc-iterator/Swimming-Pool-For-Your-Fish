#include "TM1637.h"
#include <stdexcept>

// ===================== 段码表（带小数点扩展） =====================
static uint8_t TubeTab[] = {
    0x3f,0x06,0x5b,0x4f,          // 0,1,2,3
    0x66,0x6d,0x7d,0x07,          // 4,5,6,7
    0x7f,0x6f,0x77,0x7c,          // 8,9,A,b
    0x39,0x5e,0x79,0x71,          // C,d,E,F
    
    0xbf,0x86,0xdb,0xcf,          // 0.,1.,2.,3.
    0xe6,0xed,0xfd,0x87,          // 4.,5.,6.,7.
    0xff,0xef,0xf7,0xfc,          // 8.,9.,A.,b.
    0xb9,0xde,0xf9,0xf1           // C.,d.,E.,F.
};

// ===================== TM1637 主类实现 =====================

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
    setDigitCount(4);
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
}

// ===================== 统一的清屏函数 =====================
void TM1637::clearDisplay(void)
{
    // 构建全熄灭数据（0x7f 表示该位熄灭）
    int8_t data[6] = {0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f};
    
    // 编码（无小数点）
    coding(data, 0);

    // 直接发送到硬件，不走 DisplayConfigurator
    if (digitCount == 6) {
        const int8_t phys_order[6] = {2, 1, 0, 5, 4, 3};
        int8_t send[6];
        for (int i = 0; i < 6; i++) send[i] = data[phys_order[i]];

        start();
        writeByte(ADDR_AUTO);
        stop();
        start();
        writeByte(Cmd_SetAddr);
        for (int i = 0; i < 6; i++) writeByte(send[i]);
        stop();
        start();
        writeByte(Cmd_DispCtrl);
        stop();
    } else {
        start();
        writeByte(ADDR_AUTO);
        stop();
        start();
        writeByte(Cmd_SetAddr);
        for (int i = 0; i < 4; i++) writeByte(data[i]);
        stop();
        start();
        writeByte(Cmd_DispCtrl);
        stop();
    }
}

// ===================== 原有公共接口（保留兼容） =====================
void TM1637::set(uint8_t brightness, uint8_t SetData, uint8_t SetAddr)
{
    Cmd_SetData = SetData;
    Cmd_SetAddr = SetAddr;
    Cmd_DispCtrl = 0x88 + brightness;
}

void TM1637::coding(int8_t DispData[], uint8_t dotMask) {
    uint8_t len = digitCount;
    for (uint8_t i = 0; i < len; i++) {
        if (DispData[i] == 0x7f) {
            DispData[i] = 0x00;
        } else {
            uint8_t idx = (uint8_t)DispData[i];
            if (dotMask & (1 << i))
                DispData[i] = (int8_t)TubeTab[idx + 16];
            else
                DispData[i] = (int8_t)TubeTab[idx];
        }
    }
}

int8_t TM1637::coding(int8_t DispData, bool dot) {
    if (DispData == 0x7f) return 0x00;
    uint8_t idx = (uint8_t)DispData;
    if (dot)
        return (int8_t)TubeTab[idx + 16];
    else
        return (int8_t)TubeTab[idx];
}

// ===================== DisplayConfigurator 实现 =====================

DisplayConfigurator::DisplayConfigurator(TM1637* parent, int8_t* data, uint8_t len)
    : owner(parent), dotMask(0), dataLen(len)
{
    for (uint8_t i = 0; i < len; i++) segData[i] = data[i];
}

DisplayConfigurator::~DisplayConfigurator()
{
    if (!owner) return;

    // 拷贝段码数据
    int8_t coded[6];
    for (uint8_t i = 0; i < dataLen; i++) coded[i] = segData[i];

    // 编码（dotMask 在这里传入，coding 内部处理小数点）
    owner->coding(coded, dotMask);

    // 发送数据
    if (dataLen == 6) {
        const int8_t phys_order[6] = {2, 1, 0, 5, 4, 3};
        int8_t send[6];
        for (uint8_t i = 0; i < 6; i++) send[i] = coded[phys_order[i]];
        owner->start();
        owner->writeByte(ADDR_AUTO);
        owner->stop();
        owner->start();
        owner->writeByte(owner->Cmd_SetAddr);
        for (uint8_t i = 0; i < 6; i++) owner->writeByte(send[i]);
        owner->stop();
        owner->start();
        owner->writeByte(owner->Cmd_DispCtrl);
        owner->stop();
    } else {
        owner->start();
        owner->writeByte(ADDR_AUTO);
        owner->stop();
        owner->start();
        owner->writeByte(owner->Cmd_SetAddr);
        for (uint8_t i = 0; i < 4; i++) owner->writeByte(coded[i]);
        owner->stop();
        owner->start();
        owner->writeByte(owner->Cmd_DispCtrl);
        owner->stop();
    }
}

DisplayConfigurator& DisplayConfigurator::dot(uint8_t bit, bool on)
{
    if (bit < dataLen) {
        if (on) dotMask |= (1 << bit);
        else    dotMask &= ~(1 << bit);
    }
    return *this;
}

// ===================== TM1637 的 display 接口（返回配置器） =====================

DisplayConfigurator TM1637::display(int8_t DispData[])
{
    return DisplayConfigurator(this, DispData, digitCount);
}

DisplayConfigurator TM1637::display(uint8_t BitAddr, int8_t DispData)
{
    int8_t data[6] = {0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f};
    if (BitAddr < digitCount) data[BitAddr] = DispData;
    return DisplayConfigurator(this, data, digitCount);
}