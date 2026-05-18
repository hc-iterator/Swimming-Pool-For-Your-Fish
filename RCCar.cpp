#include <pico/stdlib.h>
#include "TM1637.h"
#include "All.h"

#define TM1637_CLK 2
#define TM1637_DIO 3

int main() {


    stdio_init_all();
    sleep_ms(5000);
    identify_mosfet(N_CHANNEL);
    while(true);

    /*
    TM1637 tm(TM1637_CLK, TM1637_DIO);
    tm.set(BRIGHTEST);

    int minute = 0;
    int second = 0;

    while (true) {
        int8_t data[4];
        data[0] = minute / 10;
        data[1] = minute % 10;
        data[2] = second / 10;
        data[3] = second % 10;
        
        tm.display(data);

        sleep_ms(250);

        tm.point(true);
        tm.display(data);

        sleep_ms(500);

        tm.point(false);
        tm.display(data);

        sleep_ms(249);

        // 关闭小数点
        tm.point(false);
        tm.display(data);

        // 再等半秒后数字进位
        second++;
        if (second >= 60) {
            second = 0;
            minute++;
            if (minute >= 60) {
                minute = 0;
            }
        }

    }

    return 0;

    */
}