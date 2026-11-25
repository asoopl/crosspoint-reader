#include "DockRefresh.h"

#include <HalStorage.h>
#include <Logging.h>
#include <esp_sleep.h>
#include <esp_system.h>

#include <cstdlib>
#include <string>

namespace dockrefresh {
namespace {
uint32_t minutes = 0;
}  // namespace

void load() {
  minutes = 0;
  if (!Storage.exists(CONFIG_FILE)) return;
  std::string text;
  if (!Storage.readFileToString("DOCK", CONFIG_FILE, 32, text)) return;
  const long value = strtol(text.c_str(), nullptr, 10);
  if (value <= 0) return;
  uint32_t m = static_cast<uint32_t>(value);
  if (m < MIN_MINUTES) m = MIN_MINUTES;
  if (m > MAX_MINUTES) m = MAX_MINUTES;
  minutes = m;
  LOG_INF("DOCK", "Dock refresh every %lu min while on USB power", static_cast<unsigned long>(minutes));
}

uint32_t intervalMinutes() { return minutes; }

bool isTimerWake() {
  return esp_reset_reason() == ESP_RST_DEEPSLEEP && esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER;
}

void armIfDocked(const bool usbConnected) {
  if (minutes == 0 || !usbConnected) return;
  const uint64_t us = static_cast<uint64_t>(minutes) * 60ULL * 1000ULL * 1000ULL;
  esp_sleep_enable_timer_wakeup(us);
  LOG_INF("DOCK", "Timer wake armed in %lu min", static_cast<unsigned long>(minutes));
}

}  // namespace dockrefresh
