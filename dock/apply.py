#!/usr/bin/env python3
"""Apply the dock-refresh changes to a clean CrossPoint checkout.

Usage: python3 dock/apply.py <path-to-crosspoint-checkout> [owner/repo-for-ota]

Edits are anchored on stable code lines, not diff context, so they survive
small upstream changes. Any missing anchor aborts with a non-zero exit so the
release workflow fails loudly instead of shipping a half-patched build.
"""
import re
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent


def die(msg: str) -> None:
    print(f"dock/apply.py: {msg}", file=sys.stderr)
    sys.exit(1)


def sub_once(text: str, pattern: str, repl: str, what: str) -> str:
    new, n = re.subn(pattern, repl, text, count=1, flags=re.M)
    if n != 1:
        die(f"anchor not found: {what}")
    return new


def main() -> None:
    if len(sys.argv) < 2:
        die("usage: apply.py <checkout> [owner/repo]")
    root = Path(sys.argv[1]).resolve()
    ota_repo = sys.argv[2] if len(sys.argv) > 2 else None

    # 1. New module
    dst = root / "src" / "util"
    if not dst.is_dir():
        die(f"{dst} missing - not a CrossPoint checkout?")
    shutil.copy(HERE / "DockRefresh.h", dst / "DockRefresh.h")
    shutil.copy(HERE / "DockRefresh.cpp", dst / "DockRefresh.cpp")

    # 2. main.cpp hooks
    main_cpp = root / "src" / "main.cpp"
    s = main_cpp.read_text()
    if "dockrefresh::" in s:
        die("main.cpp already patched")

    s = sub_once(s, r'^(#include "util/ButtonNavigator\.h"\n)',
                 r'\1#include "util/DockRefresh.h"\n', "ButtonNavigator include")

    s = sub_once(
        s,
        r'^(void enterDeepSleep\(bool fromTimeout = false\) \{\n)',
        "// True when this boot is only a dock-refresh pass (timer wake on USB power).\n"
        "static bool dockRefreshPass = false;\n\n\\1",
        "enterDeepSleep definition")

    s = sub_once(
        s,
        r'^([ \t]*)(APP_STATE\.lastSleepFromReader = activityManager\.isReaderActivity\(\);)',
        r"\1// A dock-refresh pass never opened an activity; keep the real last-sleep context.\n"
        r"\1if (!dockRefreshPass) \2",
        "lastSleepFromReader assignment")

    s = sub_once(
        s,
        r'^([ \t]*)(powerManager\.startDeepSleep\(gpio, wakeOnHeldButton\);)',
        r"\1dockrefresh::armIfDocked(gpio.isUsbConnected());\n\1\2",
        "final startDeepSleep call")

    s = sub_once(
        s,
        r'^([ \t]*)(APP_STATE\.loadFromFile\(\);\n)(\s*const bool isSleepWake =)',
        r"\1\2\1dockrefresh::load();\n"
        r"\1dockRefreshPass = dockrefresh::isTimerWake() && dockrefresh::intervalMinutes() > 0;\n\3",
        "APP_STATE.loadFromFile before isSleepWake")

    s = sub_once(
        s,
        r'^([ \t]*)(setupDisplayAndFonts\(resume != BootResume::Splash\);)',
        r"\1if (dockRefreshPass) {\n"
        r"\1  // Timer wake while docked: no splash, no Home. Re-enter sleep at once, which\n"
        r"\1  // fires sleep.enter (plugins fetch a fresh /sleep.bmp), repaints the sleep\n"
        r"\1  // screen and re-arms the timer while USB power is present.\n"
        r'\1  LOG_INF("DOCK", "Dock refresh pass");' "\n"
        r"\1  setupDisplayAndFonts(true);\n"
        r"\1  enterDeepSleep();\n"
        r"\1  return;  // not reached\n"
        r"\1}\n\n\1\2",
        "setupDisplayAndFonts(resume...) in setup()")
    main_cpp.write_text(s)

    # 3. OTA source repo (optional)
    if ota_repo:
        ota = root / "src" / "network" / "OtaUpdater.cpp"
        t = ota.read_text()
        t = sub_once(t, r'api\.github\.com/repos/crosspoint-reader/crosspoint-reader/releases/latest',
                     f"api.github.com/repos/{ota_repo}/releases/latest", "OTA release URL")
        ota.write_text(t)

    print("dock/apply.py: OK" + (f" (OTA -> {ota_repo})" if ota_repo else ""))


if __name__ == "__main__":
    main()
