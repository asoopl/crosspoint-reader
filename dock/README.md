# CrossPoint „dock refresh”

Czytnik stoi na docku (zasilanie USB) i co N minut sam odświeża ekran uśpienia.
Robi to bez naciskania przycisku, a wtyczki `sleep.enter` (pogoda, kalendarz)
pobierają przy tym świeży `/sleep.bmp`.

## Jak to działa

1. Przy zasypianiu na zasilaniu USB firmware ustawia budzenie timerem ESP32.
2. Po wybudzeniu timerem pomija splash i Home i od razu wchodzi w sen:
   zdarzenie `sleep.enter` → WiFi → wtyczka pobiera obrazek → render ekranu
   uśpienia → timer ustawiony ponownie.
3. Na baterii nic się nie zmienia. Na X4/X3 bez USB firmware i tak odcina
   baterię przy uśpieniu, a na S3 bez USB timer nie jest ustawiany.
4. Ręczne wybudzenie przyciskiem działa jak zwykle, łącznie z powrotem do książki.

## Włączenie na czytniku

Na karcie SD utwórz plik `/.crosspoint/dock-refresh.txt` z liczbą minut, np.:

```
120
```

Zakres to 15–1440 minut. Brak pliku albo `0` wyłącza funkcję. Plik jest
czytany przy każdym starcie.

Do tego potrzebujesz:
- wtyczki ze zdarzeniem `sleep.enter`, która pobiera obrazek (np. Calendar sleep
  screen albo własnej);
- trybu ekranu uśpienia, który pokazuje ten plik (np. Custom → `/sleep.bmp`);
- zapisanej sieci WiFi. Firmware łączy się przy zasypianiu tylko przy baterii ≥20%.

## Fork z automatycznymi buildami

1. Zrób fork `crosspoint-reader/crosspoint-reader` na GitHubie.
2. Na domyślną gałąź forka (`develop`) wrzuć:
   - `dock/` (apply.py, DockRefresh.h, DockRefresh.cpp, ten README)
   - `.github/workflows/dock-release.yml`
3. W `dock-release.yml` ustaw `DOCK_TARGETS` na swój model:
   - X3/X4: `gh_release:x3-x4`
   - X4 Pro: `x4pro-gh_release:x4pro`
   - X4 Classic: `x4c-gh_release:x4c`
4. Actions → *Dock build* → *Run workflow*: pierwszy build.
5. Pierwszy raz wgraj `.bin` ręcznie: przez aktualizację z karty SD albo przez
   web flasher. Potem *Settings → Check for updates* pobiera już buildy z forka.

Workflow codziennie sprawdza najnowsze stabilne wydanie upstream. Jeśli w forku
nie ma jeszcze release'u z tym tagiem, nakłada patch, buduje i publikuje.
Na czytniku wersja ma dopisek `-dock`, np. `2.1.0-dock`.

Jeśli upstream zmieni kod tak, że któraś kotwica w `apply.py` zniknie,
workflow się wywali (GitHub wyśle maila) i nie opublikuje częściowo spatchowanego
firmware. Wtedy trzeba poprawić kotwicę w `apply.py`.

## Ograniczenia

- Buildy z forka nie mają klucza `FREEINK_LCP_RELEASE_KEY`, więc nie otworzą
  książek LCP (wypożyczenia z zabezpieczeniem). Reszta działa normalnie.
- Niesprawdzone na sprzęcie: zachowanie X4/X3 w deep sleep na USB z
  ustawionym timerem. Kod firmware sugeruje, że układ zostaje zasilony z USB,
  ale trzeba to potwierdzić testem (np. z interwałem 15 min).
