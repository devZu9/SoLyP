# SoLyP — техническое описание

## Модель данных

### DisplayLine
Весь текст песни хранится в одном плоском массиве `Array<DisplayLine>`:
```cpp
struct DisplayLine {
    String text;         // текст строки (одна визуальная часть)
    int sectionIndex;    // индекс секции, к которой принадлежит
    int parts;           // количество визуальных фрагментов в оригинальной строке
};
```
- Одна оригинальная строка песни может быть разбита на несколько `DisplayLine` (перенос по словам).
- Все фрагменты одной оригинальной строки имеют одинаковое значение `parts`.
- `parts` используется для расчёта скорости скролла: чем больше фрагментов, тем быстрее едет строка.

### scrollHead
Непрерывная позиция в `displayLines` (double). Целая часть — индекс текущей строки, дробная — смещение внутри неё (0..1).

### Формула времени
```
timePerLine = (60 / BPM × timeSignature) / (linesPerBar × parts)
```
- `BPM` — из DAW или ручной
- `timeSignature` — 4/4, 3/4 или 2/4
- `linesPerBar` — настройка 0.5–4.0

---

## Режимы (TransportState)

```cpp
enum class TransportState { Stopped, Playing, Paused, Countdown };
```

### STOP
`scrollHead = -1`. Текст рисуется по центру, от первой строки текущей секции. Никакого скролла.

### Play
`timerTick()` (20 Гц) продвигает `scrollHead` по формуле `timePerLine`. `paintLyrics()` — стандартная формула стрима:
```
idx = dispIdx + preLines - visLines + i
y = bottom - (visLines - i + frac) × lineHeight
```
- `preLines = SettingsManager::preLinesOnPause` — количество строк, показанных ниже текущей.
- `visLines = SettingsManager::visibleLines` — всего строк в вьюпорте.
- При `idx < resumeSkipIdx` строка не рисуется (механизм resume, см. ниже).

### Pause
1. При входе: `pauseLineDisplayIdx = (int)(scrollHead + preLines) + 1` (на 1 дальше, чтобы пауза-сообщение въезжало снизу, а не поверх текста).
2. `timerTick()` продолжает работать — `scrollHead` плавно продвигается.
3. Отрисовка — три отдельных прохода в `paintPausedLyrics()`:
   - **Стрим (normal lines):** `streamIdx < pauseIdx` — свободный скролл.
   - **Pre-lines:** `p = 0..preLines-1`, `displayLines[pauseIdx + p]`, позиция `y = bottom - (preLines - p) × lh`, заморожены внизу. С анимацией: `if (naturalY < bottomY) naturalY = bottomY`.
   - **Пауза-сообщение:** `"(ждём, не поём)"`, gold (`Theme::textPause`). Рассчитывается `naturalY`, замирает наверху когда `naturalY + lh < top`.
4. При Landmark во время паузы — `pauseLineDisplayIdx` пересчитывается под новую секцию.

### Снятие с паузы (resume)
```cpp
resumeSkipIdx = pauseLineDisplayIdx;
scrollHead = pauseLineDisplayIdx;
```
- `resumeSkipIdx` — в `paintLyrics()` (Play) строки с `idx < resumeSkipIdx` не рисуются
- При старте с C1: `resumeSkipIdx = 0` → поведение как раньше
- При возобновлении: `resumeSkipIdx = 8` → старый текст (idx 4–7) пропущен, видны только pre-lines (idx 8–9)
- Сбрасывается в `loadSong()` и при обычном Play (не из Paused)

---

## MIDI

### Команды
| Команда | Действие |
|---|---|
| Play | `Stopped/Paused → Playing` |
| Pause | `Playing → Paused` |
| Stop | `→ Stopped` (scrollHead = -1) |
| NextSection | Следующая секция |
| Hybrid | NextSection + Play |
| Countdown3 | = Play |
| Landmark | `switchToSection(n)` |

### Октавы
- **Октава ориентиров (landmark):** note % 12 = номер секции (C=0, C#=1...)
- **Октава триггеров (trigger):** каждая нота — конкретная команда
- Формула: `octave = note / 12 - 1 - octaveSystem`
  - `octaveSystem = 0` — Standard (C4 = Middle C)
  - `octaveSystem = 1` — Ableton (C3 = Middle C)

### DAW
`processBlock()` только запускает Play при `Stopped/Paused`. Никогда не останавливает — только MIDI Pause/Stop.

---

## Отрисовка (paintLyrics)

Формула стрима (Play и STOP-bottom):
```
idx = dispIdx + preLines - visLines + i
```
- `i = 0` → верх вьюпорта, `idx = dispIdx + preLines - visLines`
- `i = visLines - preLines - 1` → `idx = dispIdx` (текущая строка в середине)
- `i = visLines - 1` → низ, `idx = dispIdx + preLines - 1`

При `idx < 0` или `idx < resumeSkipIdx` — строка не рисуется. Это даёт пустые строки вверху при старте (как после C1) и при возобновлении из паузы.

---

## PreLinesOnPause

Двойная роль:
1. В **Play**: сдвиг стрима — сколько строк показано ниже текущей в вьюпорте
2. В **Pause**: количество pre-lines, замороженных внизу

Одно значение из `SettingsManager`. При pause-входе `pauseLineDisplayIdx` учитывает этот сдвиг (`scrollHead + preLinesOnPause`), чтобы пауза-сообщение въезжало после уже видимых pre-lines.

---

## Настройки

Все в `SettingsManager` (namespace + inline переменные):
- **Файл:** `Documents/SoLyP/settings.json`
- **Автосохранение:** при любом изменении
- **noStartSave:** блокирует сохранение при инициализации окна
- **Загрузка:** один раз при старте, `load()` заполняет все поля

---

## Цвета

Все в `Theme.h` / `Theme.cpp`, загружаются из `Documents/SoLyP/themes/dark.json`. Никаких хардкодов `juce::Colours::*`.

---

## Локализация

- Файлы: `Documents/SoLyP/langs/ru.json`, `en.json` (UTF-8 без BOM)
- `I18n::setLanguage()` + `onLanguageChanged()` — живое переключение
- Все строки UI через `I18n::get("ключ")`

---

## Версионирование

- Единственный источник: `CMakeLists.txt` → `project(SoLyP VERSION x.y.z ...)`
- В коде: `SOLYP_VERSION` (определяется в CMake через `target_compile_definitions`)

---

## Кодировка

- Все файлы UTF-8 без BOM
- MSVC: `/utf-8` в CMakeLists.txt
- Кириллица: `juce::String::fromUTF8()` для литералов

---

## Совместимость с Ableton Live

### Проблема
Ableton Live отказывается загружать VST3 с ошибкой:
```
VST3: plugin has an effect category, but no valid audio input bus
VST3: No valid input bus could be found
VST3: Failed: SoLyP
```

### Корень проблемы
Ableton **требует аудио-вход** (`withInput`) для VST3-эффектов. Без него плагин загружается в SAVIHost, Bitwig, но не в Ableton.

### Решение
В `PluginProcessor.cpp`, конструктор `AudioProcessor`, `BusesProperties` должен содержать **оба** bus'а:
```cpp
: AudioProcessor(BusesProperties()
        .withInput("Input", AudioChannelSet::stereo(), true)
        .withOutput("Output", AudioChannelSet::stereo(), true))
```

### Что ещё влияет на загрузку в Ableton
1. **`IS_MIDI_EFFECT TRUE`** — Ableton **не загружает** VST3 с этим флагом. Решение: `IS_MIDI_EFFECT FALSE`.
2. **`isMidiEffect()` override** — должен возвращать `false` (синхронизация с CMake).
3. **`isBusesLayoutSupported()`** — проверяет, что Ableton-запрошенная конфигурация bus'ов допустима.
4. **`producesMidi()`** — должен возвращать `true` для MIDI-плагинов.
5. **Лог Ableton:** `%APPDATA%\Ableton\Live X.Y.Z\Preferences\Log.txt` — содержит detailed ошибки загрузки VST3.
