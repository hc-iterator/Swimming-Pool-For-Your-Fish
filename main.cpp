#include <pico/stdlib.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <cctype>
#include <map>

// ===================== 引脚定义 =====================
#define PIN_TEC_IN1    14   // DRV8871 控制TEC
#define PIN_TEC_IN2    15
#define PIN_PUMP1      16   // 水泵1：水循环
#define PIN_PUMP2      17   // 水泵2：泵空气
#define PIN_PUMP3      18   // 水泵3：补水（代替电磁阀）
#define PIN_BUZZER     19   // 蜂鸣器
#define PIN_STBY       20   // DRV8833 待机引脚

#define BUFFER_SIZE    64

// ===================== 设备映射表 =====================
// 存储单引脚设备的名称与GPIO引脚号
static const std::pair<const char*, uint8_t> device_map[] = {
    {"pump1",   PIN_PUMP1},
    {"pump2",   PIN_PUMP2},
    {"pump3",   PIN_PUMP3},
    {"buzzer",  PIN_BUZZER},
    {"蜂鸣器",   PIN_BUZZER},
    {"stby",    PIN_STBY},
    {"standby", PIN_STBY},
};

// ===================== 函数声明 =====================
void init_hardware();
bool set_gpio(uint8_t gpio, bool state);
bool set_tec(bool on, bool cool);  // cool: true=制冷, false=制热
void process_command(const char* cmd, char* response);
void str_to_lower(char* str);

// ===================== 硬件初始化 =====================
void init_hardware() {
    // 初始化 TEC 控制引脚
    gpio_init(PIN_TEC_IN1);
    gpio_set_dir(PIN_TEC_IN1, GPIO_OUT);
    gpio_put(PIN_TEC_IN1, 0);
    gpio_init(PIN_TEC_IN2);
    gpio_set_dir(PIN_TEC_IN2, GPIO_OUT);
    gpio_put(PIN_TEC_IN2, 0);

    // 初始化三个水泵引脚（单桥，一个GPIO控制开关）
    gpio_init(PIN_PUMP1);
    gpio_set_dir(PIN_PUMP1, GPIO_OUT);
    gpio_put(PIN_PUMP1, 0);
    gpio_init(PIN_PUMP2);
    gpio_set_dir(PIN_PUMP2, GPIO_OUT);
    gpio_put(PIN_PUMP2, 0);
    gpio_init(PIN_PUMP3);
    gpio_set_dir(PIN_PUMP3, GPIO_OUT);
    gpio_put(PIN_PUMP3, 0);

    // 蜂鸣器
    gpio_init(PIN_BUZZER);
    gpio_set_dir(PIN_BUZZER, GPIO_OUT);
    gpio_put(PIN_BUZZER, 0);

    // STBY 默认拉高，使 DRV8833 正常工作
    gpio_init(PIN_STBY);
    gpio_set_dir(PIN_STBY, GPIO_OUT);
    gpio_put(PIN_STBY, 1);
}

// ===================== 通用GPIO设置 =====================
bool set_gpio(uint8_t gpio, bool state) {
    // 安全检查：排除特殊引脚
    if (gpio == 23 || gpio == 24 || gpio == 25 || gpio == 29)
        return false;
    if (gpio > 28) return false;

    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_OUT);
    gpio_put(gpio, state);
    return true;
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
    char buffer[BUFFER_SIZE];
    strncpy(buffer, cmd, BUFFER_SIZE - 1);
    buffer[BUFFER_SIZE - 1] = '\0';
    str_to_lower(buffer);

    char device[BUFFER_SIZE] = {0};
    char action[BUFFER_SIZE] = {0};

    // 解析命令：device action
    int parsed = sscanf(buffer, "%s %s", device, action);
    if (parsed < 2) {
        sprintf(response, "Failed: Invalid format. Use: <device> <on/off/0/1/true/false>");
        return;
    }

    // 解析动作
    bool state;
    if (strcmp(action, "1") == 0 || strcmp(action, "on") == 0 || strcmp(action, "true") == 0) {
        state = true;
    } else if (strcmp(action, "0") == 0 || strcmp(action, "off") == 0 || strcmp(action, "false") == 0) {
        state = false;
    } else {
        sprintf(response, "Failed: Invalid action '%s'. Use on/off/0/1/true/false", action);
        return;
    }

    // ========== TEC 特殊处理 ==========
    if (strcmp(device, "tec") == 0) {
        set_tec(state, true);   // 默认制冷
        sprintf(response, "Success: TEC = %s (cooling)", state ? "ON" : "OFF");
        return;
    }
    if (strcmp(device, "tec_heat") == 0 || strcmp(device, "tech") == 0) {
        set_tec(state, false);  // 制热
        sprintf(response, "Success: TEC = %s (heating)", state ? "ON" : "OFF");
        return;
    }

    // ========== 查找映射表 ==========
    for (const auto& entry : device_map) {
        if (strcmp(device, entry.first) == 0) {
            gpio_put(entry.second, state ? 1 : 0);
            sprintf(response, "Success: %s = %s", entry.first, state ? "ON" : "OFF");
            return;
        }
    }

    // ========== GPIO 命令 ==========
    if (strncmp(device, "gpio", 4) == 0) {
        int gpio_num = atoi(device + 4);
        if (set_gpio(gpio_num, state))
            sprintf(response, "Success: GPIO%d = %d", gpio_num, state);
        else
            sprintf(response, "Failed: Invalid GPIO%d (must be 0-22, 26-28)", gpio_num);
        return;
    }

    // ========== 未知设备 ==========
    sprintf(response, "Failed: Unknown device '%s'. Supported: pump1/2/3, buzzer, stby, tec, tec_heat, gpioXX", device);
}

// ===================== 主函数 =====================
int main() {
    stdio_init_all();
    init_hardware();

    // 等待 USB 串口真正连接（不是固定延时）
    printf("\nWaiting for USB serial connection...\n");
    while (!stdio_usb_connected()) {
        sleep_ms(10);
    }
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
    printf("  gpioXX ON/OFF             - Control GPIO\n");
    printf("========================================\n");
    printf("> ");

    char input[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int pos = 0;

    while (true) {
        int c = getchar_timeout_us(10000);  // 10ms超时，非阻塞

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
                printf("%c", c);  // 回显
            }
        }

        sleep_ms(10);  // 主循环延时，防止CPU满载
    }

    return 0;
}