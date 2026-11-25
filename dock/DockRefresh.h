#pragma once

#include <cstdint>

// Dock refresh: while the device sleeps on USB power, wake it on a timer,
// re-enter sleep at once and so re-run sleep.enter plugin events (e.g. a
// weather/calendar plugin that downloads /sleep.bmp) and repaint the sleep
// screen. No UI: enabled by a plain-text file on the SD card holding the
// interval in minutes, e.g. "120". Missing file, 0 or junk = disabled.
namespace dockrefresh {

constexpr const char* CONFIG_FILE = "/.crosspoint/dock-refresh.txt";
constexpr uint32_t MIN_MINUTES = 15;
constexpr uint32_t MAX_MINUTES = 24 * 60;

// Read the interval from the SD card. Call once after Storage.begin().
void load();

// Interval in minutes, 0 when disabled.
uint32_t intervalMinutes();

// True when this boot is a timer wake from deep sleep.
bool isTimerWake();

// Arm the timer wake source if enabled and USB power is present.
// Call right before the final deep-sleep entry.
void armIfDocked(bool usbConnected);

}  // namespace dockrefresh
