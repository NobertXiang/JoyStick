/**
 * @file    FreeRTOS V2 Architecture Implementation Guide
 * @brief   Complete multi-task architecture for STM32F103 Joystick with FreeRTOS
 * 
 * ARCHITECTURE OVERVIEW
 * =====================
 * 
 * This implementation provides 5 concurrent tasks plus the default USB task:
 * 
 * 1. ADC Task (High Priority, 256 bytes stack)
 *    - Polls ADC DMA buffer every 10ms
 *    - Sends raw ADC values to adcQueueHandle
 *    - No blocking - always completes quickly
 * 
 * 2. Encoder Task (Normal Priority, 256 bytes stack)
 *    - Reads TIM1/2/3/4 counter values every 10ms
 *    - Sends encoder counts to encoderQueueHandle
 *    - Simple counter reads, no blocking
 * 
 * 3. Button Task (Normal Priority, 256 bytes stack)
 *    - Reads 74HC165 shift registers via SPI2 every 20ms
 *    - Latches input on PB12, reads 3 bytes via SPI
 *    - Sends 24-bit button state to buttonQueueHandle
 * 
 * 4. LED Task (Low Priority, 256 bytes stack)
 *    - Waits on ledQueueHandle for LED control data
 *    - Writes 3 bytes to 74HC595 shift registers via SPI1
 *    - Latches output on PB2
 * 
 * 5. HID Task (Real-time Priority, 512 bytes stack)
 *    - Main communication thread with USB host (highest priority)
 *    - Receives ADC, Encoder, Button data non-blocking
 *    - Packs data into HID report (currently 12 bytes of ADC)
 *    - Sends HID report via USBD_CUSTOM_HID_SendReport_FS
 *    - Would receive LED control commands from USB in future
 * 
 * MEMORY ALLOCATION
 * =================
 * 
 * STM32F103 SRAM: 20 KB (20480 bytes)
 * 
 * FreeRTOS Heap: 3072 bytes
 *   - Queue storage: ~330 bytes
 *     * ADC Queue (4 × 24B): 96B
 *     * Encoder Queue (4 × 16B): 64B
 *     * Button Queue (4 × 3B): 12B
 *     * LED Queue (4 × 3B): 12B
 *     * HID Data Queue (2 × 34B): 68B
 *     * Overhead: ~78B
 *   - Task Control Blocks: ~250 bytes (5 tasks)
 *   - Timer task: ~50 bytes
 *   - FreeRTOS kernel: ~400 bytes
 *   - Remaining buffer: ~700 bytes
 * 
 * Task Stacks (from main stack):
 *   - ADC Task: 1024 bytes (256×4)
 *   - Encoder Task: 1024 bytes (256×4)
 *   - Button Task: 1024 bytes (256×4)
 *   - LED Task: 1024 bytes (256×4)
 *   - HID Task: 2048 bytes (512×4)
 *   - Default Task: 512 bytes (128×4)
 *   - Total: ~7680 bytes
 * 
 * TOTAL USAGE: ~10.75 KB / 20 KB (53.7% - Safe margin maintained)
 * 
 * QUEUE DEFINITIONS
 * =================
 * 
 * AdcData_t (24 bytes):
 *   - channels[6]: uint16_t (12 bytes total)
 * 
 * EncoderData_t (16 bytes):
 *   - encoders[4]: uint32_t (16 bytes total)
 * 
 * ButtonData_t (3 bytes):
 *   - states[3]: uint8_t (3 bytes total)
 *   - Represents 24 button inputs from 3×74HC165
 * 
 * LedData_t (3 bytes):
 *   - states[3]: uint8_t (3 bytes total)
 *   - Represents 24 LED outputs to 3×74HC595
 * 
 * HidReportData_t (34 bytes):
 *   - adc: AdcData_t (24 bytes)
 *   - encoders: EncoderData_t (16 bytes)
 *   - buttons: ButtonData_t (3 bytes)
 *   - (Note: This is only used as working buffer, actual HID report is 12 bytes)
 * 
 * TIMING DIAGRAM
 * ==============
 * 
 * Time (ms)   0    5   10   15   20   25   30   35   40   45   50
 * ADC Task    [+   ]    [+   ]    [+   ]    [+   ]    [+   ]
 * Encoder     [+   ]    [+   ]    [+   ]    [+   ]    [+   ]
 * Button      [+       ]    [+       ]    [+       ]    [+       ]
 * LED         [   Wait for data (event-driven)        ]
 * HID         [+       ]    [+       ]    [+       ]    [+       ]
 * 
 * HARDWARE CONNECTIONS
 * ====================
 * 
 * SPI1 (74HC595 - Output Shift Register for LEDs):
 *   - PA5/PA6/PA7: SCK/MISO/MOSI (standard SPI)
 *   - PB2: RCLK (Latch pin for 74HC595)
 *   - Chain 3 units for 24 LED outputs
 * 
 * SPI2 (74HC165 - Input Shift Register for Buttons):
 *   - PB13/PB14/PB15: SCK/MISO/MOSI (standard SPI)
 *   - PB12: RCLK (Latch pin for 74HC165)
 *   - Chain 3 units for 24 button inputs
 * 
 * Timers (Encoders):
 *   - TIM1: Encoder 1 (quadrature input on PA8, PA9)
 *   - TIM2: Encoder 2 (quadrature input on PA0, PA1)
 *   - TIM3: Encoder 3 (quadrature input on PA6, PA7)
 *   - TIM4: Encoder 4 (quadrature input on PD12, PD13)
 * 
 * ADC (Joystick Analog Inputs):
 *   - 6 channels via DMA (continuous conversion)
 *   - Channels: CH0-CH5 (PA0-PA5 range)
 * 
 * GPIO:
 *   - PC13: System LED (toggle every 500ms in default task)
 *   - PB2: SPI1 RCLK (74HC595 output latch)
 *   - PB12: SPI2 RCLK (74HC165 input latch)
 * 
 * USB:
 *   - PA11/PA12: USB D-/D+ (Custom HID device)
 * 
 * CONFIGURATION
 * =============
 * 
 * FreeRTOS Config:
 *   - TOTAL_HEAP_SIZE: 3072 bytes
 *   - MAX_PRIORITIES: 56
 *   - TICK_RATE: 1000 Hz (1ms tick)
 *   - Timer task enabled
 *   - Preemption enabled
 * 
 * Priority Levels (higher number = higher priority):
 *   - HID Task: osPriorityRealtime (48) - top priority for real-time USB
 *   - ADC Task: osPriorityHigh (40) - fast ADC sampling
 *   - Encoder Task: osPriorityAboveNormal (32) - encoder reading
 *   - Button Task: osPriorityNormal (24) - button polling
 *   - LED Task: osPriorityBelowNormal (16) - LED output (passive)
 *   - Default Task: osPriorityNormal (24) - USB init + system LED
 * 
 * DATA FLOW DIAGRAM
 * =================
 * 
 *  ADC HW ─────────┐
 *  (DMA)           │
 *                  ├──→ ADC Task ──→ adcQueue ──┐
 *                  │                            │
 *  TIM1/2/3/4──→ Encoder Task ──→ encoderQueue ──┤
 *  (Counters)                                     ├──→ HID Task ──→ USB Host
 *                                                 │
 *  SPI2+74HC165→ Button Task ──→ buttonQueue ──┘
 *  (Input SR)
 *
 *  USB Host ──→ HID Task ──→ ledQueue ──→ LED Task ──→ SPI1+74HC595
 *  (LED cmds)                                        (Output SR)
 * 
 * FUTURE ENHANCEMENTS
 * ===================
 * 
 * 1. HID Report Expansion:
 *    - Currently only 12 bytes (ADC data)
 *    - Can extend to include encoder and button data in same report
 *    - May need larger report size or multiple reports
 * 
 * 2. Input Validation:
 *    - Add hysteresis filtering for analog inputs
 *    - Debounce button inputs
 *    - Handle encoder edge cases
 * 
 * 3. Error Handling:
 *    - Add watchdog timer
 *    - Monitor queue overflow conditions
 *    - Fallback behaviors for SPI communication failures
 * 
 * 4. Performance Monitoring:
 *    - Use configUSE_TRACE_FACILITY for profiling
 *    - Monitor stack high water marks
 *    - Log queue usage statistics
 * 
 * 5. Power Management:
 *    - Implement tickless idle mode
 *    - Reduce task execution frequencies based on activity
 * 
 * FILES MODIFIED
 * ==============
 * 
 * Created:
 *   - Core/Inc/joystick_rtos.h: Data structures and queue declarations
 * 
 * Modified:
 *   - Core/Src/freertos.c: Complete rewrite with all 5 tasks
 *   - Core/Src/main.c: Added encoder timer startup
 * 
 * Unchanged (still functional):
 *   - Core/Src/adc.c: ADC initialization, used by ADC task
 *   - Core/Src/spi.c: SPI initialization, used by Button/LED tasks
 *   - Core/Src/tim.c: Timer initialization, encoders read in encoder task
 *   - Core/Src/gpio.c: GPIO initialization
 */
