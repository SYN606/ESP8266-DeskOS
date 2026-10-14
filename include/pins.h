#pragma once

#include <Arduino.h>

// ============================================================================
// ESP32 DeskOS - Central Pin Definitions
// ============================================================================

#if defined(ESP8266)
// ESP8266 NodeMCU Pinout
// Standard I2C configuration: SDA -> D2 (GPIO4), SCL -> D1 (GPIO5)
#ifndef D2
#define D2 4
#endif
#ifndef D1
#define D1 5
#endif

constexpr uint8_t PIN_I2C_SDA = D2;  // GPIO4
constexpr uint8_t PIN_I2C_SCL = D1;  // GPIO5

#elif defined(ESP32)
// Standard ESP32 Dev Board Pinout
constexpr uint8_t PIN_I2C_SDA = 21;  // GPIO21 (Standard ESP32 I2C Data)
constexpr uint8_t PIN_I2C_SCL = 22;  // GPIO22 (Standard ESP32 I2C Clock)

#else
constexpr uint8_t PIN_I2C_SDA = 4;
constexpr uint8_t PIN_I2C_SCL = 5;
#endif

// Display Hardware Reset Pin (-1 if tied to RST or not present)
constexpr int8_t PIN_OLED_RST = -1;
