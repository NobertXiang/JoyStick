# STM32F103 JoyStick FreeRTOS V2 - 构建与测试指南

## 项目结构

```
Core/
├── Inc/
│   ├── joystick_rtos.h       ← 新建：RTOS数据结构和队列定义
│   ├── main.h                ← 已修改：GPIO定义
│   ├── adc.h                 ← 已有
│   ├── spi.h                 ← 已有
│   ├── tim.h                 ← 已有
│   └── FreeRTOSConfig.h      ← 已有
├── Src/
│   ├── freertos.c            ← 大幅修改：5个任务实现
│   ├── main.c                ← 修改：启动编码器计时器
│   ├── adc.c                 ← 已有（未修改）
│   ├── spi.c                 ← 已有（未修改）
│   ├── tim.c                 ← 已有（未修改）
│   └── ARCHITECTURE.c        ← 新建：架构文档
└── HID_LED_INTEGRATION.md    ← 新建：HID集成文档
```

## 编译步骤

### 使用VS Code CMake工具

1. **打开项目**
   ```
   在VS Code中打开此工作区
   ```

2. **选择构建预设**
   - 按 `Ctrl+Shift+P` 打开命令面板
   - 选择 "CMake: Select Configure Preset"
   - 选择 "Debug" 或 "Release"

3. **配置CMake**
   - 命令面板 > "CMake: Configure"
   - 或直接点击左下角的"配置"按钮

4. **编译项目**
   - 按 `Ctrl+Shift+B` 构建
   - 或命令面板 > "CMake: Build"
   - 或左下角点击"构建"按钮

5. **烧录到STM32**
   - 使用任务 "STM32: 烧录 (ST-Link SWD)"
   - 或手动命令：
   ```bash
   STM32_Programmer_CLI --connect port=swd --download build/Debug/JoyStick.elf -hardRst -run
   ```

### 使用命令行

```bash
# 进入项目目录
cd d:\Projects\Coding\STM32\JoyStick

# 配置Debug版本
cmake --preset Debug

# 编译
cmake --build build/Debug

# 烧录
STM32_Programmer_CLI --connect port=swd --download build/Debug/JoyStick.elf -hardRst -run
```

## 编译可能的问题及解决方案

### 问题1：找不到cmake
**解决方案：**
- 确保安装了CMake（版本3.16+）
- 验证cmake在系统PATH中
- 或使用VS Code的CMake扩展

### 问题2：头文件包含错误
**可能原因和解决方案：**

```c
// 错误：找不到joystick_rtos.h
// 解决：确保文件在 Core/Inc/ 目录中
// 验证：freertos.c 中的 #include "joystick_rtos.h"

// 错误：找不到CMSIS_device_header
// 解决：这在 FreeRTOSConfig.h 中定义，应该自动包含
```

### 问题3：链接错误
**可能原因：**
- CMakeLists.txt中的源文件列表缺少新文件
- FreeRTOS库链接缺失
- HAL库版本不匹配

**解决方案：**
```bash
# 清理构建文件
rm -r build/Debug
# 重新配置
cmake --preset Debug
# 重新编译
cmake --build build/Debug --verbose
```

### 问题4：烧录失败
**检查清单：**
- [ ] ST-Link V2调试器已连接
- [ ] USB线缆已连接
- [ ] STM32F103 VCC和GND已连接
- [ ] SWDIO/SWCLK连接正确
- [ ] 微控制器没有被锁定

**测试连接：**
```bash
STM32_Programmer_CLI --connect port=swd
```

## 测试步骤

### 1. 系统启动测试
**预期行为：**
- PC13 LED（SysLED）每500ms闪烁一次
- USB设备在计算机上枚举为"Custom HID Device"

**测试方法：**
```bash
# 烧录后，观察LED闪烁
# 在Windows：设备管理器中应显示USB设备
# 在Linux：dmesg | grep -i usb
```

### 2. ADC转换测试
**预期行为：**
- ADC数据每10ms采集一次
- HID报告每10ms发送一次

**测试方法：**
```c
// 在调试器中设置断点在 HidTask 中
// 检查 hidReport 缓冲区中的数据是否变化
// 使用USB HID监视工具读取IN报告
```

### 3. 编码器读取测试
**预期行为：**
- 旋转编码器时，encoderQueue 中的值变化
- 最大计数值为 65535（16位）

**测试方法：**
```c
// 在调试器中添加监视表达式
// encoderData.encoders[0] 应随编码器旋转而变化
// 检查是否能检测两个方向的旋转
```

### 4. 按钮读取测试
**预期行为：**
- 按下按钮时，SPI2 读取 74HC165 的数据
- buttonQueue 中的状态位变化

**测试方法：**
```c
// 在 ButtonTask 中设置断点
// 检查 rxBuffer 中的数据
// 每个字节对应8个按钮
```

### 5. LED控制测试（需要后续实现）
**预期行为：**
- 接收到LED控制命令后，SPI1 输出变化
- 74HC595 芯片的输出反映命令

**测试方法：**
```c
// 需要先实现 HID OUT 报告处理
// 将测试数据放入 ledQueue
// 观察 SPI1 和 RCLK 信号
```

### 6. 完整数据流测试
**预期行为：**
- 所有5个任务同时运行
- 数据正确流经各个队列
- HID报告包含正确的ADC数据

**测试步骤：**

1. 在PC上编写HID监视程序：
```python
import hid
import struct

device = hid.device()
device.open(0x0000, 0x0000)  # 替换为正确的VID/PID

while True:
    report = device.read(64)  # 读取IN报告
    if report:
        # 解析12字节ADC数据
        values = []
        for i in range(6):
            value = (report[i*2] << 8) | report[i*2+1]
            values.append(value)
        print(f"ADC: {values}")
```

2. 运行监视程序
3. 调整模拟输入（摇杆/滑块）
4. 观察ADC值是否随输入变化

## 调试技巧

### 使用ST-Link调试器

```bash
# 连接调试器
STM32_Programmer_CLI --connect port=swd

# 加载符号进行调试
# 在 VS Code 中：
# - 安装 "Cortex-Debug" 扩展
# - 创建 .vscode/launch.json 配置
# - F5 启动调试
```

### launch.json 示例配置

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "STM32 Debug",
      "type": "cortex-debug",
      "request": "launch",
      "servertype": "openocd",
      "cwd": "${workspaceRoot}",
      "executable": "${workspaceRoot}/build/Debug/JoyStick.elf",
      "device": "STM32F103CB",
      "interface": "swd",
      "configFiles": [
        "interface/stlink-dap.cfg",
        "target/stm32f1x.cfg"
      ],
      "preLaunchTask": "build",
      "stopAtEntry": true,
      "postLaunchCommands": [
        "monitor reset init"
      ]
    }
  ]
}
```

### 添加日志输出

在 usart.c 中使用UART进行debug输出：

```c
#include <stdio.h>

int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, 10);
  return len;
}

// 在任务中使用
printf("ADC Task: CH0=%d\r\n", adcData.channels[0]);
```

## 性能监控

### FreeRTOS实时统计

启用以下配置（在FreeRTOSConfig.h中）：
```c
#define configGENERATE_RUN_TIME_STATS 1
#define configUSE_TRACE_FACILITY 1
```

然后可以获取运行时统计：
```c
#include "freertos_debug.h"

char stats[500];
vTaskGetRunTimeStats(stats);
printf("%s", stats);
```

### 堆栈使用情况

```c
UBaseType_t watermark = uxTaskGetStackHighWaterMark(adcTaskHandle);
printf("ADC Task stack watermark: %d bytes\r\n", watermark * 4);
```

## 部署检查清单

在烧录到生产设备前，验证以下项目：

- [ ] 所有5个任务正常运行
- [ ] ADC数据正确采集
- [ ] 编码器能检测旋转
- [ ] 按钮能检测按压
- [ ] HID报告正确发送
- [ ] 无内存泄漏（堆栈未溢出）
- [ ] USB设备稳定枚举
- [ ] 系统LED正常闪烁
- [ ] 无任务崩溃或死锁
- [ ] 温度和功耗在规格内

## 后续改进

- [ ] 实现LED控制回路（从USB接收LED命令）
- [ ] 添加输入验证和滤波
- [ ] 实现错误恢复机制
- [ ] 扩展HID报告以包含所有数据
- [ ] 添加低功耗模式支持
- [ ] 实现固件更新功能

## 技术支持资源

- STM32F103 参考手册
- STM32CubeMX HAL库文档
- FreeRTOS官方文档
- USB HID规范 (usb.org)
