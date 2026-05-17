#include <pico/stdlib.h>
#include "TM1637.h"

#define TM1637_CLK 2
#define TM1637_DIO 3

int main() {
    TM1637 tm(TM1637_CLK, TM1637_DIO);
    tm.set(BRIGHTEST);

    int minute = 0;
    int second = 0;
    bool point_on = true;

    while (true) {
        int8_t data[4];
        data[0] = minute / 10;
        data[1] = minute % 10;
        data[2] = second / 10;
        data[3] = second % 10;

        // 小数点反相：每半秒切换，亮灭交替
        tm.point(point_on);
        tm.display(data);
        point_on = !point_on;

        // 首次显示后等半秒再更新数字
        sleep_ms(500);

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

        sleep_ms(500);  // 半秒后进入下一轮
    }

    return 0;
}