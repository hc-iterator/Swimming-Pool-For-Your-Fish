#include <pico/stdlib.h>
#include <stdio.h>

// 测试引脚
#define PIN1 2
#define PIN2 3
#define PIN3 4

// 沟道类型参数
enum MOS_TYPE { N_CHANNEL, P_CHANNEL, UNKNOWN };

void setup_gpio(uint pin, bool output, bool high) {
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
    setup_gpio(pin_a, true, true);   // A输出高电平
    setup_gpio(pin_b, true, false);  // B输出低电平
    sleep_ms(10);
    
    // 把B设为输入，检测电压
    setup_gpio(pin_b, false, false);
    sleep_ms(5);
    bool result = gpio_get(pin_b);   // 如果读到高电平，说明有二极管导通
    
    return result;
}

void identify_mosfet() {
    stdio_init_all();
    printf("\n=== MOS管引脚识别测试 ===\n");
    printf("请将MOS管插到GPIO %d, %d, %d\n", PIN1, PIN2, PIN3);
    sleep_ms(2000);
    
    // 三轮测试，每轮检测一对引脚之间的二极管方向
    // 测试1: PIN1 -> PIN2
    bool t1_forward = test_diode(PIN1, PIN2);   // PIN1正, PIN2负
    bool t1_reverse = test_diode(PIN2, PIN1);   // PIN2正, PIN1负
    
    // 测试2: PIN1 -> PIN3
    bool t2_forward = test_diode(PIN1, PIN3);
    bool t2_reverse = test_diode(PIN3, PIN1);
    
    // 测试3: PIN2 -> PIN3
    bool t3_forward = test_diode(PIN2, PIN3);
    bool t3_reverse = test_diode(PIN3, PIN2);
    
    // 分析结果
    uint gate = 0, source = 0, drain = 0;
    MOS_TYPE type = N_CHANNEL;
    
    // 栅极的特征：和任何其他引脚之间都没有二极管导通
    bool pin1_is_gate = !t1_forward && !t1_reverse && !t2_forward && !t2_reverse;
    bool pin2_is_gate = !t1_forward && !t1_reverse && !t3_forward && !t3_reverse;
    bool pin3_is_gate = !t2_forward && !t2_reverse && !t3_forward && !t3_reverse;
    
    if (pin1_is_gate) {
        gate = PIN1;
        // PIN2和PIN3之间应该有二极管
        if (t3_forward && !t3_reverse) {
            source = PIN2; drain = PIN3; type = N_CHANNEL;
        } else if (!t3_forward && t3_reverse) {
            source = PIN3; drain = PIN2; type = P_CHANNEL;
        }
    } else if (pin2_is_gate) {
        gate = PIN2;
        if (t2_forward && !t2_reverse) {
            source = PIN1; drain = PIN3; type = N_CHANNEL;
        } else if (!t2_forward && t2_reverse) {
            source = PIN3; drain = PIN1; type = P_CHANNEL;
        }
    } else if (pin3_is_gate) {
        gate = PIN3;
        if (t1_forward && !t1_reverse) {
            source = PIN1; drain = PIN2; type = N_CHANNEL;
        } else if (!t1_forward && t1_reverse) {
            source = PIN2; drain = PIN1; type = P_CHANNEL;
        }
    }
    
    // 输出结果
    printf("\n=== 识别结果 ===\n");
    if (type == N_CHANNEL) {
        printf("类型: N-MOS (AO3400)\n");
    } else if (type == P_CHANNEL) {
        printf("类型: P-MOS (AO3401)\n");
    } else {
        printf("类型: 无法识别（请检查接线）\n");
    }
    
    printf("栅极(G): GPIO %d\n", gate);
    printf("源极(S): GPIO %d\n", source);
    printf("漏极(D): GPIO %d\n", drain);
    
    if (type != UNKNOWN) {
        printf("\n体二极管方向: %d -> %d\n", source, drain);
    }
    
    while(true) tight_loop_contents();
}