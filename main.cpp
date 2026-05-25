#include <pico/stdlib.h>
#include <stdio.h>
#include <map>
#include <cstring>
#include <cctype>
#include <string>
#include <cstdlib>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <regex>
#include <cctype>
#include <map>

// ===================== 引脚映射表 =====================
// 存储“设备名”到“GPIO引脚号”的映射，修改这里即可完成所有单引脚设备的定义
std::pair<const char*, uint> device_map[] = {
    {"pump1",   16},   // 水泵1：水循环
    {"pump2",   17},   // 水泵2：泵空气
    {"pump3",   18},   // 水泵3：补水（代替电磁阀）
    {"buzzer",  19},   // 蜂鸣器
    {"stby",    20},   // DRV8833 待机引脚
};

#define PIN_TEC_IN1    14   // DRV8871 控制TEC
#define PIN_TEC_IN2    15
#define BUFFER_SIZE    64

// ===================== 函数声明 =====================
void init_hardware();
bool set_tec(bool on, bool cool);
void process_command(const char* cmd, char* response);
void str_to_lower(char* str);

// ===================== 硬件初始化 =====================
void init_hardware() {
    // 1. 初始化映射表中的所有单引脚设备
    for (auto d : device_map) {
        gpio_init(d.second);
        gpio_set_dir(d.second, GPIO_OUT);
        gpio_put(d.second, 0);       // 默认关闭
    }
    // STBY 默认拉高
    gpio_put(20, 1);

    // 2. 初始化 TEC 控制引脚 (双引脚 H 桥)
    gpio_init(PIN_TEC_IN1);
    gpio_set_dir(PIN_TEC_IN1, GPIO_OUT);
    gpio_put(PIN_TEC_IN1, 0);
    gpio_init(PIN_TEC_IN2);
    gpio_set_dir(PIN_TEC_IN2, GPIO_OUT);
    gpio_put(PIN_TEC_IN2, 0);
}

// ===================== TEC 控制 (DRV8871) =====================
bool set_tec(bool on, bool cool) {
    if (on) {
        if (cool) {         // 制冷：IN1高, IN2低
            gpio_put(PIN_TEC_IN1, 1);
            gpio_put(PIN_TEC_IN2, 0);
        } else {            // 制热：IN1低, IN2高
            gpio_put(PIN_TEC_IN1, 0);
            gpio_put(PIN_TEC_IN2, 1);
        }
    } else {                // 停止
        gpio_put(PIN_TEC_IN1, 0);
        gpio_put(PIN_TEC_IN2, 0);
    }
    return true;
}

// ===================== 字符串转小写 =====================
void str_to_lower(char* str) {
    for (int i = 0; str[i]; i++)
        str[i] = tolower(str[i]);
}

// ===================== 命令处理 =====================
void process_command(const char* cmd, char* response) {

    // 跳过命令开头所有不可见字符（比如 \0、BOM、控制字符等）
    while (*cmd && (*cmd < ' ' || *cmd > '~')) {
        cmd++;
    }
    std::string input(cmd);
    
    // 正则表达式：匹配 "设备名 动作"，忽略大小写，忽略前后空格
    // 设备名可以是任何不含空格的字符（数字、字母、下划线等）
    // 动作只能是 1, 0, on, off, true, false
    std::regex pattern(R"(^\s*([^\s]+)\s+(1|0|on|off|true|false)\s*$)", std::regex::icase);
    std::smatch match;
    
    if (!std::regex_match(input, match, pattern)) {
        sprintf(response, "Failed: Invalid format. Use: <device> <on/off/0/1/true/false>");
        return;
    }
    
    std::string device = match[1].str();
    std::string action = match[2].str();
    
    // 解析动作
    bool state;
    if (action == "1" || action == "on" || action == "true") {
        state = true;
    } else { // 正则已保证只能是 0, off, false
        state = false;
    }
    
    // ========== TEC 特殊处理 ==========
    if (device == "tec") {
        set_tec(state, true);   // 默认制冷
        sprintf(response, "Success: TEC = %s (cooling)", state ? "ON" : "OFF");
        return;
    }
    if (device == "tec_heat" || device == "tech") {
        set_tec(state, false);  // 制热
        sprintf(response, "Success: TEC = %s (heating)", state ? "ON" : "OFF");
        return;
    }
    
    // ========== 查找映射表 ==========
    for (auto& entry : device_map) {
        if (device == entry.first) {
            gpio_put(entry.second, state ? 1 : 0);
            sprintf(response, "Success: %s = %s", entry.first, state ? "ON" : "OFF");
            return;
        }
    }
    
    // ========== 纯数字 GPIO 命令 ==========
    // 检查 device 是否全为数字
    bool is_number = true;
    for (char c : device) {
        if (!isdigit(c)) {
            is_number = false;
            break;
        }
    }
    
    if (is_number) {
        int gpio_num = std::stoi(device);
        if (gpio_num >= 0 && gpio_num <= 29 && gpio_num != 23 && gpio_num != 24 && gpio_num != 29) {
            gpio_init(gpio_num);
            gpio_set_dir(gpio_num, GPIO_OUT);
            gpio_put(gpio_num, state);
            sprintf(response, "Success: GPIO%d = %d", gpio_num, state);
            return;
        } else {
            sprintf(response, "Failed: GPIO%d is invalid or reserved", gpio_num);
            return;
        }
    }
    
    // ========== 未知设备 ==========
    sprintf(response, "Failed: Unknown device '%s'. Supported: pump1/2/3, tec, tec_heat, buzzer, stby, or GPIO number", device.c_str());
}


// ===================== 主函数 =====================
int main() {
    stdio_init_all();
    init_hardware();

    // 等待 USB 串口真正连接（不是固定延时）
    printf("\nWaiting for USB serial connection...\n");
    while (!stdio_usb_connected()) { sleep_ms(10); }
    sleep_ms(10); 
    while (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT){};
    printf("Connected!\n");

    // 欢迎信息
    printf("========================================\n");
    printf("  FishTank Serial Command Console\n");
    printf("========================================\n");
    printf("Commands:\n");
    printf("  pump1/pump2/pump3 ON/OFF  - Control pumps\n");
    printf("  tec ON/OFF                - TEC cooling\n");
    printf("  tec_heat ON/OFF           - TEC heating\n");
    printf("  buzzer ON/OFF             - Buzzer\n");
    printf("  stby ON/OFF               - DRV8833 standby\n");
    printf("  <GPIO> ON/OFF             - Direct GPIO control (e.g. 25 on)\n");
    printf("========================================\n");
    printf("> ");

    char input[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int pos = 0;

    while (true) {
        int c = getchar_timeout_us(10000);

        if (c != PICO_ERROR_TIMEOUT) {
            if (c == '\r' || c == '\n') {
                if (pos > 0) {
                    input[pos] = '\0';
                    printf("\n");
                    process_command(input, response);
                    printf("%s\n> ", response);
                    pos = 0;
                }
            } else if (pos < BUFFER_SIZE - 1) {
                input[pos++] = c;
                printf("%c", c);
            }
        }

        sleep_ms(10);
    }

    return 0;
}