#include <pico/stdlib.h>
#include <stdio.h>
#include "All.h"
// 设置GPIO模式
void setup_pin(uint pin, bool output, bool high) {
    gpio_init(pin);
    if (output) {
        gpio_set_dir(pin, GPIO_OUT);
        gpio_put(pin, high ? 1 : 0);
    } else {
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_down(pin);
    }
}

// 检测两个引脚之间是否有二极管导通（pin_a为正极，pin_b为负极）
bool test_diode(uint pin_a, uint pin_b) {
    setup_pin(pin_a, true, true);   // A输出高电平
    setup_pin(pin_b, true, false);  // B输出低电平
    sleep_ms(10);
    setup_pin(pin_b, false, false); // B设为输入，检测电压
    sleep_ms(5);
    return gpio_get(pin_b);         // 读到高电平说明有二极管导通
}

// 检测两个引脚之间是否电阻导通（用于判断MOS管是否被栅极打开）
bool test_resistance(uint pin_a, uint pin_b) {
    setup_pin(pin_a, true, true);   // A输出高电平
    setup_pin(pin_b, false, false); // B设为输入并下拉
    sleep_ms(10);
    return gpio_get(pin_b);         // 如果A和B之间有电流通路，B会被拉到高电平
}

void identify_mosfet() {
    printf("\n=== MOS管引脚识别测试 ===\n");
    printf("请将MOS管插到GPIO %d, %d, %d\n", PIN1, PIN2, PIN3);
    
    // 三轮体二极管测试
    bool d12 = test_diode(PIN1, PIN2); // PIN1正, PIN2负
    bool d21 = test_diode(PIN2, PIN1); // PIN2正, PIN1负
    bool d13 = test_diode(PIN1, PIN3);
    bool d31 = test_diode(PIN3, PIN1);
    bool d23 = test_diode(PIN2, PIN3);
    bool d32 = test_diode(PIN3, PIN2);
    
    // 第一步：找栅极（和其他引脚之间都没有体二极管）
    uint gate = 0, s1 = 0, s2 = 0;
    
    if (!d12 && !d21 && !d13 && !d31) {
        gate = PIN1; s1 = PIN2; s2 = PIN3;
        printf("栅极(G) 找到: GPIO %d\n", gate);
    } else if (!d12 && !d21 && !d23 && !d32) {
        gate = PIN2; s1 = PIN1; s2 = PIN3;
        printf("栅极(G) 找到: GPIO %d\n", gate);
    } else if (!d13 && !d31 && !d23 && !d32) {
        gate = PIN3; s1 = PIN1; s2 = PIN2;
        printf("栅极(G) 找到: GPIO %d\n", gate);
    } else {
        printf("错误：无法找到栅极，请检查MOS管和接线\n");
        while(1) tight_loop_contents();
    }
    
    // 第二步：判断N/P沟道
    // 给栅极高电平(3.3V)，测试s1和s2之间是否电阻导通
    setup_pin(gate, true, true);  // 栅极高电平
    sleep_ms(1);
    bool cond_high = test_resistance(s1, s2) || test_resistance(s2, s1);
    
    // 给栅极低电平(0V)，测试s1和s2之间是否电阻导通
    setup_pin(gate, true, false); // 栅极低电平
    sleep_ms(1);
    bool cond_low = test_resistance(s1, s2) || test_resistance(s2, s1);
    
    MOS_TYPE type;
    uint source, drain;
    
    if (!cond_low && cond_high) {
        // 栅极高电平时导通 → N沟道
        type = N_CHANNEL;
        // N沟道体二极管方向：源极(S) → 漏极(D)
        if (test_diode(s1, s2)) { source = s1; drain = s2; }
        else { source = s2; drain = s1; }
        printf("类型: N-MOS (AO3400)\n");
    } else if (cond_low && !cond_high) {
        // 栅极低电平时导通 → P沟道
        type = P_CHANNEL;
        // P沟道体二极管方向：漏极(D) → 源极(S)
        if (test_diode(s1, s2)) { drain = s1; source = s2; }
        else { drain = s2; source = s1; }
        printf("类型: P-MOS (AO3401)\n");
    } else {
        printf("错误：无法判断沟道类型\n");
        while(1) tight_loop_contents();
    }
    
    printf("源极(S): GPIO %d\n", source);
    printf("漏极(D): GPIO %d\n", drain);
    printf("体二极管方向: %d -> %d\n", source, drain);
    
    // 验证：正确识别的管子应该满足以下条件
    // N沟道：G低电平时D-S不通，G高电平时D-S导通
    // P沟道：G高电平时S-D不通，G低电平时S-D导通
    
    while(1) tight_loop_contents();
}
