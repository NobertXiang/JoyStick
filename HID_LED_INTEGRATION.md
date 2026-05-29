/**
 * @file    HID_LED_Integration.md
 * @brief   Integration guide for HID LED control
 * 
 * HID REPORT PROTOCOL
 * ===================
 * 
 * ## Current Implementation (ADC-Only Report)
 * 
 * OUT Report (from PC to device) - To be implemented:
 *   - 3 bytes of LED control data (24 LEDs)
 *   - This report type will be received in the HID callback
 * 
 * IN Report (from device to PC) - Currently implemented:
 *   - Bytes 0-1: ADC CH0 (uint16_t, MSB first)
 *   - Bytes 2-3: ADC CH1
 *   - Bytes 4-5: ADC CH2
 *   - Bytes 6-7: ADC CH3
 *   - Bytes 8-9: ADC CH4
 *   - Bytes 10-11: ADC CH5
 *   - Total: 12 bytes
 * 
 * ## Recommended Full Protocol
 * 
 * For complete implementation with encoder and button data:
 * 
 * IN Report Structure (34 bytes):
 *   - Bytes 0-1: ADC CH0
 *   - Bytes 2-3: ADC CH1
 *   - Bytes 4-5: ADC CH2
 *   - Bytes 6-7: ADC CH3
 *   - Bytes 8-9: ADC CH4
 *   - Bytes 10-11: ADC CH5
 *   - Bytes 12-13: Encoder 1 count (uint16_t)
 *   - Bytes 14-15: Encoder 2 count (uint16_t)
 *   - Bytes 16-17: Encoder 3 count (uint16_t)
 *   - Bytes 18-19: Encoder 4 count (uint16_t)
 *   - Bytes 20-22: Button states (3 bytes)
 *   - Bytes 23: Reserved/Checksum
 * 
 * OUT Report Structure (3 bytes):
 *   - Bytes 0-2: LED control states (24 LEDs in 3 bytes)
 * 
 * LED CONTROL FROM HID
 * ====================
 * 
 * ## Receiving LED Data from USB
 * 
 * The HID callback function receives OUT reports. You need to modify:
 * File: USB_DEVICE/App/usbd_custom_hid_if.c
 * 
 * Add this to the HID interface:
 * 
 * ```c
 * static uint8_t USBD_CUSTOM_HID_OutEvent_FS(uint8_t event_idx, uint8_t state)
 * {
 *   // Check if this is an OUT report event
 *   if (event_idx == 0x01)  // Assuming OUT report ID is 1
 *   {
 *     // Get the report data
 *     extern uint8_t USBD_CUSTOM_HID_ReportDesc_FS[];
 *     extern uint8_t hUsbDeviceFS_CUSTOM_HID_Report_FS[];
 *     
 *     // Create LED data structure
 *     LedData_t ledData;
 *     ledData.states[0] = hUsbDeviceFS_CUSTOM_HID_Report_FS[0];
 *     ledData.states[1] = hUsbDeviceFS_CUSTOM_HID_Report_FS[1];
 *     ledData.states[2] = hUsbDeviceFS_CUSTOM_HID_Report_FS[2];
 *     
 *     // Send to LED queue
 *     extern osMessageQueueId_t ledQueueHandle;
 *     osMessageQueuePut(ledQueueHandle, &ledData, 0, 0);
 *   }
 *   
 *   return USBD_OK;
 * }
 * ```
 * 
 * ## HID Descriptor Requirements
 * 
 * The HID descriptor must include:
 * 
 * 1. Input Report (IN):
 *    - 12 bytes for current ADC-only implementation
 *    - Can be extended to 34 bytes for full data
 * 
 * 2. Output Report (OUT):
 *    - 3 bytes for LED control
 *    - Must be defined for PC to send data to device
 * 
 * Example HID Report Descriptor modification:
 * 
 * ```c
 * // Current (ADC only - Input)
 * 0x95, 0x0C,  // Report Count (12 bytes)
 * 0x15, 0x00,  // Logical Minimum (0)
 * 0x25, 0xFF,  // Logical Maximum (255)
 * 0x75, 0x08,  // Report Size (8 bits)
 * 0x81, 0x02,  // Input (Data,Var,Abs)
 * 
 * // New (LED control - Output)
 * 0x95, 0x03,  // Report Count (3 bytes)
 * 0x15, 0x00,  // Logical Minimum (0)
 * 0x25, 0xFF,  // Logical Maximum (255)
 * 0x75, 0x08,  // Report Size (8 bits)
 * 0x91, 0x02,  // Output (Data,Var,Abs)
 * ```
 * 
 * DATA FLOW FOR LED CONTROL
 * ==========================
 * 
 * ```
 * ┌─────────────┐
 * │  USB Host   │
 * │  (PC App)   │
 * └──────┬──────┘
 *        │ LED control (3 bytes)
 *        ▼
 * ┌──────────────────────────┐
 * │  USB Device / HID Layer  │
 * │  usbd_custom_hid_if.c    │
 * └──────┬───────────────────┘
 *        │ OSOutEvent callback
 *        ▼
 * ┌─────────────────────────┐
 * │  LED Queue              │
 * │  (ledQueueHandle)       │
 * └──────┬──────────────────┘
 *        │ osMessageQueueGet
 *        ▼
 * ┌─────────────────────────┐
 * │  LED Task               │
 * │  (Low Priority)         │
 * └──────┬──────────────────┘
 *        │ HAL_SPI_Transmit
 *        ▼
 * ┌─────────────────────────┐
 * │  SPI1 to 74HC595        │
 * │  (Output Shift Register)│
 * │  24 LED outputs         │
 * └─────────────────────────┘
 * ```
 * 
 * IMPLEMENTATION STEPS
 * ====================
 * 
 * 1. Modify HID Descriptor:
 *    - Add OUTPUT report definition to include LED control
 *    - Update report sizes if expanding to full data protocol
 * 
 * 2. Implement OUT Event Callback:
 *    - Modify usbd_custom_hid_if.c
 *    - Extract LED data from OUT report
 *    - Put data into ledQueueHandle
 * 
 * 3. Test LED Control:
 *    - Write USB host application (Windows/Linux/Mac)
 *    - Send LED control commands (3 bytes patterns)
 *    - Verify LED outputs change accordingly
 * 
 * 4. Expand Report (Optional):
 *    - Modify HID descriptor to include encoder/button data
 *    - Update HID task packing logic
 *    - Test with full protocol
 * 
 * TROUBLESHOOTING
 * ===============
 * 
 * Problem: LED outputs don't respond
 *   - Check: Is OSOutEvent callback registered?
 *   - Check: Is LED queue being filled?
 *   - Check: Can SPI1 communicate with 74HC595?
 *   - Check: Is RCLK pulse happening on PB2?
 * 
 * Problem: ADC/Encoder/Button data not received on PC
 *   - Check: Is HID task sending reports?
 *   - Check: Is USB device enumerated?
 *   - Check: Are queues populated with data?
 * 
 * Problem: HID report not received by PC
 *   - Check: Report descriptor matches code
 *   - Check: Report size is correct
 *   - Check: No USB handshake errors
 * 
 * REFERENCES
 * ==========
 * 
 * - usbd_custom_hid_if.h: HID interface definitions
 * - usbd_desc.c: USB device descriptors
 * - freertos.c: HID Task implementation (HidTask function)
 * - joystick_rtos.h: Data structures
 * - main.h: GPIO pin definitions
 */
