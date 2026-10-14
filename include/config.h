#pragma once

#include <Arduino.h>

// ============================================================================
// ESP32 DeskOS - Firmware Configuration
// ============================================================================

#define DESKOS_VERSION "v0.1"
#define DESKOS_NAME    "ESP32 DeskOS"

// Serial console speed
constexpr unsigned long SERIAL_BAUD = 115200;

// I2C bus clock speed in Hz (standard fast mode: 400kHz, standard: 100kHz)
constexpr uint32_t I2C_CLOCK_SPEED = 400000;

// Display controller selection:
// 1 = SH1106 (common for 1.3" 128x64 OLEDs)
// 0 = SSD1306 (common for 0.96" 128x64 OLEDs)
#define OLED_CONTROLLER_SH1106 1

// Display dimensions
constexpr uint8_t SCREEN_WIDTH  = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
