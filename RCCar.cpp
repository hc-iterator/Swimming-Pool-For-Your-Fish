#include <pico/stdlib.h>
#include "TM1637.h"
#include <pico/time.h> 
#include "All.h"

#define TM1637_CLK 2
#define TM1637_DIO 3

int main() {

      /*
    stdio_init_all();
    sleep_ms(5000);
    identify_mosfet(N_CHANNEL);
    while(true);

    */
    TM1637 tm(TM1637_CLK, TM1637_DIO);
    tm.setDigitCount(6);
    tm.set(BRIGHTEST);

    int hour = 0, minute = 0, second = 0;
    int dot_pos = 0;                     // 当前闪烁的小数点位（0‑5）

    absolute_time_t cycle_start = get_absolute_time();   // 记录当前时间作为周期起点

    while (true) {
        // 1. 准备本次要显示的数字（HHMMSS）
        int8_t data[6];
        data[0] = hour / 10;
        data[1] = hour % 10;
        data[2] = minute / 10;
        data[3] = minute % 10;
        data[4] = second / 10;
        data[5] = second % 10;

        // 2. 显示数字，所有小数点先灭
        tm.clearDots();
        tm.display(data);

        // 3. 等待 250 ms（从周期起点算起）
        absolute_time_t t_dot_on = delayed_by_ms(cycle_start, 250);
        best_effort_wfe_or_timeout(t_dot_on);

        // 4. 点亮当前闪烁位的小数点
        tm.setDot(dot_pos, true);
        tm.display(data);

        // 5. 再等 500 ms（从点亮时刻算起）
        absolute_time_t t_dot_off = delayed_by_ms(t_dot_on, 500);
        best_effort_wfe_or_timeout(t_dot_off);

        // 6. 熄灭该位小数点
        tm.setDot(dot_pos, false);
        tm.display(data);

        // 7. 等待到下一个周期起点（周期起点 + 1000 ms）
        absolute_time_t next_cycle = delayed_by_ms(cycle_start, 1000);
        best_effort_wfe_or_timeout(next_cycle);

        // 8. 更新时间
        second++;
        if (second >= 60) { second = 0; minute++; }
        if (minute >= 60) { minute = 0; hour++; }
        if (hour >= 24)   { hour = 0; }

        // 9. 切换小数点闪烁位置（1→2→…→6→1）
        dot_pos = (dot_pos + 1) % 6;

        // 10. 当前周期结束，cycle_start 切换到下一周期起点
        cycle_start = next_cycle;
    }

    return 0;
}