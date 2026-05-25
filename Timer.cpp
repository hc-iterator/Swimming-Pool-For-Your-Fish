#include <pico/stdlib.h>
#include "TM1637.h"
#include <stdio.h>
#include <pico/time.h>  // 引入绝对时间库

#define TM1637_CLK 3
#define TM1637_DIO 2

int timermain() {

    stdio_init_all();
    
    TM1637 tm(TM1637_CLK, TM1637_DIO);
    tm.setDigitCount(6);
    tm.set(BRIGHTEST);

    int hour = 0, minute = 0, second = 0;

    // 记录第一个周期的开始时间
    absolute_time_t cycle_start = get_absolute_time();

    while (true) {
        // 1. 准备当前要显示的数字（HHMMSS）
        int8_t data[6];
        data[0] = hour / 10;
        data[1] = hour % 10;
        data[2] = minute / 10;
        data[3] = minute % 10;
        data[4] = second / 10;
        data[5] = second % 10;

        // 2. 显示节奏：无点(250ms) -> 第2、4位点亮(500ms) -> 无点(250ms)
        
        // 第一阶段：无点显示，并等待250ms
        tm.display(data);
        sleep_until(delayed_by_ms(cycle_start, 250)); // 等待到从周期开始算起的第250ms

        // 第二阶段：第2、4位小数点同时点亮，并等待500ms
        tm.display(data).dot(1).dot(3);
        sleep_until(delayed_by_ms(cycle_start, 750)); // 等待到从周期开始算起的第750ms

        // 第三阶段：再次无点显示，并等待剩下的时间
        tm.display(data);
        // 精确等待到周期结束（从 cycle_start 算起的第 1000ms）
        sleep_until(delayed_by_ms(cycle_start, 1000));

        // 3. 更新时间
        second++;
        if (second >= 60) { second = 0; minute++; }
        if (minute >= 60) { minute = 0; hour++; }
        if (hour >= 24)   { hour = 0; }

        // 4. 为下一个周期设定准确的开始时间
        cycle_start = delayed_by_ms(cycle_start, 1000);
    }

    return 0;
}