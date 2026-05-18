// 测试引脚
#define PIN1 2
#define PIN2 3
#define PIN3 4

// 沟道类型参数
enum MOS_TYPE  : bool { N_CHANNEL, P_CHANNEL};
void identify_mosfet(MOS_TYPE type);