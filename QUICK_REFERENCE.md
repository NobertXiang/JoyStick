# FreeRTOS V2 架构 - 快速参考

## 任务优先级和栈大小速查表

| 任务名称 | 优先级 (CMSIS-OS2) | 数值 | 栈大小 | 周期 | 功能 |
|---------|------------------|------|--------|------|------|
| HID Task | osPriorityRealtime | 48 | 512×4 B | 10ms | HID通讯，数据聚合 |
| ADC Task | osPriorityHigh | 40 | 256×4 B | 10ms | ADC轮询，DMA读取 |
| Encoder Task | osPriorityAboveNormal | 32 | 256×4 B | 10ms | 编码器计数读取 |
| Button Task | osPriorityNormal | 24 | 256×4 B | 20ms | 按钮SPI读取 |
| LED Task | osPriorityBelowNormal | 16 | 256×4 B | 事件驱动 | LED SPI输出 |
| Default Task | osPriorityNormal | 24 | 128×4 B | 500ms | USB初始化，LED闪烁 |

## 队列大小配置

| 队列名称 | 元素数量 | 元素大小 | 总大小 | 内容 |
|---------|---------|---------|--------|------|
| adcQueue | 4 | 24 B | 96 B | ADC 6通道 |
| encoderQueue | 4 | 16 B | 64 B | 4个编码器计数 |
| buttonQueue | 4 | 3 B | 12 B | 24按钮状态 |
| ledQueue | 4 | 3 B | 12 B | 24LED控制 |
| hidDataQueue | 2 | 34 B | 68 B | 完整HID数据 |

## GPIO 引脚映射

| 功能 | 端口 | 引脚 | 描述 |
|------|------|------|------|
| System LED | PC | 13 | 500ms闪烁 |
| SPI1 RCLK | PB | 2 | 74HC595输出锁存 |
| SPI2 RCLK | PB | 12 | 74HC165输入锁存 |
| ADC 通道0-5 | PA | 0-5 | 6路模拟输入 |
| SPI1 | PA | 5,6,7 | SCK,MISO,MOSI |
| SPI2 | PB | 13,14,15 | SCK,MISO,MOSI |
| 编码器1 | PA | 8,9 | TIM1 |
| 编码器2 | PA | 0,1 | TIM2 |
| 编码器3 | PA | 6,7 | TIM3 |
| 编码器4 | PD | 12,13 | TIM4 |
| USB | PA | 11,12 | D-,D+ |

## HID 报告格式

### 输入报告 (IN) - 12字节
```
字节  0-1:  ADC CH0 (uint16_t, 大端)
字节  2-3:  ADC CH1
字节  4-5:  ADC CH2
字节  6-7:  ADC CH3
字节  8-9:  ADC CH4
字节 10-11: ADC CH5
```

### 输出报告 (OUT) - 3字节（待实现）
```
字节 0-2: LED 控制状态 (24位)
         位0: LED0, 位1: LED1, ... 位23: LED23
```

## 内存分配速查

| 项目 | 大小 | 百分比 |
|------|------|--------|
| 总SRAM | 20 KB | 100% |
| FreeRTOS堆 | 3 KB | 15% |
| 任务栈 | ~7.7 KB | 38% |
| 其他/缓冲区 | ~9.3 KB | 47% |

## 数据结构定义

### AdcData_t
```c
typedef struct {
    uint16_t channels[6];  // 6个ADC通道
} AdcData_t;  // 大小: 12 字节
```

### EncoderData_t
```c
typedef struct {
    uint32_t encoders[4];  // 4个编码器
} EncoderData_t;  // 大小: 16 字节
```

### ButtonData_t
```c
typedef struct {
    uint8_t states[3];  // 24个按钮 = 3字节
} ButtonData_t;  // 大小: 3 字节
```

### LedData_t
```c
typedef struct {
    uint8_t states[3];  // 24个LED = 3字节
} LedData_t;  // 大小: 3 字节
```

## 常见编译错误及解决方案

| 错误 | 原因 | 解决方案 |
|------|------|---------|
| `undefined reference to AdcTask` | 函数未实现 | 检查freertos.c中的函数定义 |
| `joystick_rtos.h not found` | 路径错误 | 确保文件在Core/Inc/目录 |
| `stack overflow` | 栈大小不足 | 增加对应任务的STACK_SIZE |
| `heap overflow` | 堆大小不足 | 检查FreeRTOSConfig.h中的configTOTAL_HEAP_SIZE |
| `undefined reference to __HAL_TIM_GET_COUNTER` | HAL宏未定义 | 确保包含了stm32f1xx_hal_tim.h |

## 调试输出宏

在freertos.c中添加调试输出：

```c
#define DEBUG_ENABLE 1

#if DEBUG_ENABLE
#define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

// 使用示例
DEBUG_PRINT("ADC Task: CH0=%d\r\n", adcData.channels[0]);
```

## 任务状态监控

```c
// 获取任务状态
TaskStatus_t xTaskDetails;
vTaskGetInfo(adcTaskHandle, &xTaskDetails, pdTRUE, eInvalid);

// 获取栈水位线
UBaseType_t watermark = uxTaskGetStackHighWaterMark(adcTaskHandle);

// 打印任务列表
char taskList[1024];
vTaskList(taskList);
printf("%s", taskList);
```

## 关键文件清单

### 新创建的文件
- `Core/Inc/joystick_rtos.h` - 数据结构和队列定义
- `Core/Src/ARCHITECTURE.c` - 架构文档
- `HID_LED_INTEGRATION.md` - HID集成指南
- `BUILD_AND_TEST_GUIDE.md` - 构建测试指南

### 修改的文件
- `Core/Src/freertos.c` - 完整的任务实现
- `Core/Src/main.c` - 编码器启动代码

### 未修改但相关的文件
- `Core/Src/adc.c` - ADC初始化
- `Core/Src/spi.c` - SPI初始化
- `Core/Src/tim.c` - 定时器初始化
- `Core/Inc/FreeRTOSConfig.h` - FreeRTOS配置

## 线程安全考虑

### 无需互斥锁的原因
1. **ADC数据** - DMA自动更新，任务只读
2. **编码器计数** - 硬件计数器，原子读取
3. **HID报告** - 一次性发送，无并发问题
4. **队列操作** - FreeRTOS队列本身是线程安全的

### 需要考虑的保护点（如有扩展）
- LED控制数据 - 如果多个任务写入需要互斥锁
- 共享配置 - 运行时参数修改需要信号量

## 性能目标

| 指标 | 目标 | 实现状态 |
|------|------|---------|
| ADC采样率 | 100 Hz (10ms) | ✓ |
| 编码器更新率 | 100 Hz (10ms) | ✓ |
| 按钮扫描率 | 50 Hz (20ms) | ✓ |
| HID报告率 | 100 Hz (10ms) | ✓ |
| 系统响应性 | < 1ms | ✓ |
| 堆栈使用率 | < 50% | ✓ |
| CPU使用率 | < 30% | 待测量 |

## 快速测试命令

### 编译
```bash
cd d:\Projects\Coding\STM32\JoyStick
cmake --preset Debug && cmake --build build/Debug
```

### 烧录
```bash
STM32_Programmer_CLI --connect port=swd --download build/Debug/JoyStick.elf -hardRst -run
```

### 清理
```bash
rm -r build
```

## 常见问题FAQ

**Q: 如何增加堆大小？**
A: 修改 `Core/Inc/FreeRTOSConfig.h` 中的 `configTOTAL_HEAP_SIZE`

**Q: 如何改变任务优先级？**
A: 修改 `Core/Inc/joystick_rtos.h` 中的 `*_TASK_PRIORITY` 常量

**Q: 如何添加新任务？**
A: 
1. 在 `joystick_rtos.h` 中定义数据结构
2. 在 `freertos.c` 中实现任务函数
3. 在 `MX_FREERTOS_Init()` 中创建任务

**Q: LED不工作怎么办？**
A: 检查 SPI1 初始化、74HC595 接线、RCLK 脉冲

**Q: 如何监控内存使用？**
A: 启用 `configUSE_TRACE_FACILITY` 和 `configGENERATE_RUN_TIME_STATS`
