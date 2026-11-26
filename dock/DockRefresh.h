#pragma once

#include <cstdint>

// Dock refresh: wake on a timer from deep sleep, re-enter sleep at once and so
// re-run sleep.enter plugin events (e.g. a weather/calendar plugin that
// downloads /sleep.bmp) and repaint the sleep screen.
//
// Configured by /.crosspoint/dock-refresh.txt on the SD card, key=value lines:
//
//   usb=120        # minutes between refreshes on USB power   (0/missing = off)
//   battery=1440   # minutes between refreshes on battery      (0/missing = off)
//   min_battery=30 # no battery timer below this charge (default 30 %)
//
// A file holding only a number ("120") means usb=120. Xteink X3/X4 (ESP32-C3)
// cut battery power in sleep, so "battery" only works on S3 boards (X4 Pro...).
namespace dockrefresh {

constexpr const char* CONFIG_FILE = "/.crosspoint/dock-refresh.txt";
constexpr uint32_t MIN_MINUTES = 15;
constexpr uint32_t MAX_MINUTES = 7 * 24 * 60;

// Read the config from the SD card. Call once after Storage.begin().
void load();

// True when any interval is configured.
bool enabled();

// True when this boot is a timer wake from deep sleep.
bool isTimerWake();

// Arm the timer wake source for the current power state.
// Call right before the final deep-sleep entry.
void arm(bool usbConnected, uint16_t batteryPercent);

}  // namespace dockrefresh
