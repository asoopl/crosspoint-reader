#include "DockRefresh.h"

#include <HalStorage.h>
#include <Logging.h>
#include <esp_sleep.h>
#include <esp_system.h>

#include <cstdlib>
#include <cstring>
#include <string>

namespace dockrefresh {
namespace {
uint32_t usbMinutes = 0;
uint32_t batteryMinutes = 0;
uint16_t minBattery = 30;

uint32_t clampMinutes(long v) {
  if (v <= 0) return 0;
  if (v < static_cast<long>(MIN_MINUTES)) return MIN_MINUTES;
  if (v > static_cast<long>(MAX_MINUTES)) return MAX_MINUTES;
  return static_cast<uint32_t>(v);
}

void parseLine(const char* key, const char* value) {
  const long v = strtol(value, nullptr, 10);
  if (strcmp(key, "usb") == 0) {
    usbMinutes = clampMinutes(v);
  } else if (strcmp(key, "battery") == 0) {
    batteryMinutes = clampMinutes(v);
  } else if (strcmp(key, "min_battery") == 0) {
    minBattery = static_cast<uint16_t>(v < 20 ? 20 : (v > 100 ? 100 : v));
  }
}
}  // namespace

void load() {
  usbMinutes = batteryMinutes = 0;
  minBattery = 30;
  if (!Storage.exists(CONFIG_FILE)) return;
  std::string text;
  if (!Storage.readFileToString("DOCK", CONFIG_FILE, 512, text)) return;

  if (text.find('=') == std::string::npos) {
    usbMinutes = clampMinutes(strtol(text.c_str(), nullptr, 10));  // legacy: bare number
  } else {
    size_t pos = 0;
    while (pos < text.size()) {
      size_t end = text.find('\n', pos);
      if (end == std::string::npos) end = text.size();
      std::string line = text.substr(pos, end - pos);
      pos = end + 1;
      const size_t hash = line.find('#');
      if (hash != std::string::npos) line.resize(hash);
      const size_t eq = line.find('=');
      if (eq == std::string::npos) continue;
      std::string key = line.substr(0, eq);
      std::string value = line.substr(eq + 1);
      const auto trim = [](std::string& s) {
        const size_t b = s.find_first_not_of(" \t\r");
        const size_t e = s.find_last_not_of(" \t\r");
        s = b == std::string::npos ? std::string() : s.substr(b, e - b + 1);
      };
      trim(key);
      trim(value);
      parseLine(key.c_str(), value.c_str());
    }
  }
  LOG_INF("DOCK", "usb=%lu battery=%lu min_battery=%u", static_cast<unsigned long>(usbMinutes),
          static_cast<unsigned long>(batteryMinutes), minBattery);
}

bool enabled() { return usbMinutes > 0 || batteryMinutes > 0; }

bool isTimerWake() {
  return esp_reset_reason() == ESP_RST_DEEPSLEEP && esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER;
}

void arm(const bool usbConnected, const uint16_t batteryPercent) {
  const uint32_t minutes = usbConnected ? usbMinutes : batteryMinutes;
  if (minutes == 0) return;
  if (!usbConnected && batteryPercent < minBattery) {
    LOG_INF("DOCK", "Battery %u%% < %u%%, no timer wake", batteryPercent, minBattery);
    return;
  }
  const uint64_t seconds = static_cast<uint64_t>(minutes) * 60;
  esp_sleep_enable_timer_wakeup(seconds * 1000ULL * 1000ULL);
  LOG_INF("DOCK", "Timer wake in %lu s (%s)", static_cast<unsigned long>(seconds), usbConnected ? "usb" : "battery");
}

}  // namespace dockrefresh
