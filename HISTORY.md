# History

Хронология разработки SoLyP — VST3/Standalone MIDI промптер текста.

---

## v1.0.0 — 2026-06-13

Создание проекта. JUCE 8.0.6, CMake, C++20, Windows.
VST3 + Standalone сборка. Базовый UI: текстовое поле, MIDI-вход.

Первый коммит — просто «чтоб работало». Никакой архитектуры.

---

## v1.0.1–1.0.3 — 2026-06-13..06-15

**Темы, локализация, сохранение настроек.**

- Файл `PluginEditor.cpp` разбит на `Helpers`
- `Theme.h` — цвета в коде, загрузка из JSON
- `SettingsManager` — автосохранение слайдеров
- `ru.json` / `en.json` — вынесены в отдельные файлы, UTF-8 без BOM
- MSVC-флаг `/utf-8` в CMake (иначе кириллица — кракозябры)
- `juce::String::fromUTF8()` для русских строк

Проблемы:
- Окно не сохраняло размер — исправлено в v1.0.6
- Язык не переключался живьём — исправлено в v1.0.5
- `noStartSave` — защита от перезаписи настроек при старте

---

## v1.0.4 — 2026-06-16

**Ленивая перестройка displayLines.**

Строки пересобираются при изменении ширины окна или шрифта,
а не на каждом repaint. `lastBuildWidth` + `lastBuildFontSize`.

---

## v1.0.6 — 2026-06-18

**Окно и таймеры.**

- Окно сохраняется правильно (Position + Size)
- Удалён древний костыльный таймер
- `noStartSave` — проверка (не сохранять настройки при старте)

---

## v1.0.7 — 2026-06-18..06-19

**MIDI + курсор.**

- MIDI-нота в статус-баре (перерисовка в реальном времени)
- Свой курсор: SVG-иконка в стиле Miskamo
- Экспоненциальная скорость вращения курсора
- Всегда-поверх (`alwaysOnTop`)
- Защита от повторного запуска (InterProcessLock)

---

## v1.0.8 — 2026-06-19

**Кнопки-иконки, тултипы, несохранённые изменения.**

- SVG-иконки для кнопок (Load, Edit, Save, Settings)
- Тултипы с собственным стилем (только `drawTooltip` — JUCE 8)
- `pendingChanges` — флаг несохранённых изменений, диалог при выходе
- `arrow-circle-left` иконка для Back

---

## v1.0.9 — 2026-06-21

**Рефакторинг скролла.**

- `scrollHead` — новая модель скролла через абстрактный индекс
- `parts` — плавный скролл (скорость зависит от BPM, timeSignature,
  barsPerLine, parts)
- `calcFittingLines` — сколько строк влезает
- Фикс: MIDI-нота Play не запускала проигрывание
- Фикс: локаль не применялась к некоторым строкам

---

## v1.0.10 — 2026-06-21

**Пауза + анимация.**

- `pauseScroll` — анимация скролла на паузе (старые строки уезжают)
- `paintPausedLyrics` — три прохода отрисовки
- `resumeSkipIdx` — пропуск уже прочитанных строк
- Countdown5 → Stop запуск

Очень сырая реализация — пауза была одной из самых сложных фич.

---

## v1.0.11 — 2026-06-22

**Рефакторинг `enter*` → `switch*`.**

- `enterPlay`/`enterPause`/`enterStop` → `switchPlay`/`switchPause`/`switchStop`
- `previousState` удалён (заменён проверками внутри switch)
- `showPauseText` — флаг отображения сообщения
- Stopped → Pause блокирован (проверка в switchPause)
- `switchToSection` — прыжок без дёрганий

---

## v1.0.12 — 2026-06-22

**Отсчёт 3-2-1.**

- `countdownPhase`, `countdownPhaseDuration`
- Полоска прогресса
- Stopped/Paused → Countdown (Playing → Countdown блокирован)
- Auto-Play после отсчёта
- `enterPlay`/`enterPause` с корректным scrollHead

---

## v1.0.14 — 2026-06-23

**Ableton VST3.**

- `IS_MIDI_EFFECT = FALSE` — Ableton не грузил как MIDI-эффект
- Stereo input bus для VST3 (Fx категория)
- Экран приветствия (название, версия, автор)
- VST3 ссылка в `C:\Program Files\Common Files\VST3\`

---

## v1.0.17 — 2026-06-23

**Пауза + предстроки (первая адекватная версия).**

- `preLinesOnPause` — количество предстрок
- Фикс: при возобновлении Play pre-строки не дублировались
- Фикс: Stop не замораживал слоты
- ROADMAP.md добавлен

---

## v1.0.18–1.0.19 — 2026-06-23

**Параллельный отсчёт + скролл.**

- Countdown и Scroll — независимые таймеры
- 3× скорость скролла для анимации паузы

---

## v1.1.2–1.1.3 — 2026-06-24..06-25

**Slot model + рефакторинг песни.**

- `struct Slot { juce::String text; double y; }` — каждый слот хранит свой Y
- Нет `scrollOffset` и сложных формул пересчёта
- `rebuildDisplayLines` при загрузке песни (раньше был пустой экран)

---

## v1.1.4–1.1.6 — 2026-06-25

**Слайдеры, пауза-сообщение, тултипы.**

- Слайдер строк: 2–20
- Слайдер зазора: 0.75–2.0 (потом 0.0–2.0, шаг 0.05)
- Зависимые слайдеры (Lines ↔ Size ↔ Gap)
- Кастомное пауза-сообщение (многострочное, авто-шрифт)
- FlexBox кнопки в редакторе и настройках
- `barsPerLine` — тактов на строку
- Тултипы в теме

---

## v1.1.7 — 2026-06-25

**Разделение файлов + структура.**

- `PluginProcessor` → `Logic`
- `PluginEditor` → `Display`
- `Editor/` → `Display/`
- `DisplayCallbacks.cpp`, `DisplayActions.cpp`, `DisplayHelpers.cpp`
- `TimerScroll`, `TimerPause`, `TimerPreLines`, `TimerCountdown`, `TimerCursor`
- Каждый таймер = отдельный callback

---

## v1.1.9 — 2026-06-26

**Высота строки и предстроки.**

- `getRealLineHeight()` — fontSize × 0.6 × (1 + gap)
- `pauseMsgSpeed` — скорость сообщения паузы
- `stepPerFramePx` — пикселей за кадр
- `lineSpacingInt` в JSON (исправление ошибок представления float)

---

## v1.1.10 — 2026-06-26

**Рефакторинг паузы: topLimit.**

- `bounds` → `lyricsViewArea` (поле класса)
- `startY` удалён, `topLimit` с полной формулой
- `topYlastLine` — Y последней строки (entryY переименован)
- Единое условие shift: `slots[N-1].y + lineHeight < topYlastLine`
- `maxAvailableHeight` удалён
- `std::floor` для fontSize (не теряет строку при reload)
- `lineSpacingInt` миграция

---

## v1.1.11 — 2026-06-26

**Fast-изображение для паузы.**

Старый текст захватывается как `juce::Image` и уезжает на boost×.
Предстроки крутятся независимо (медленно, 1×).
`preScroll` удалён, `pauseShiftCount` — один shift.
`boost = 15` (вместо 5.0).

---

## v1.1.12 — 2026-06-26

**Section target → отдельный case.**

- `SectionChanged` — новый элемент `TransportState`
- `case SectionChanged` — обработка смены секции
- `switchHybrid()` → два вызова: SectionChanged → Playing
- `stateBeforeSection` / `restoreAfterSection()`
- Старые обработчики смены секции после switch удалены

---

## v1.1.13 — 2026-06-28

**Отсчёт: независимость + late.**

- Отсчёт не запускает Play
- `countdownPhase--` (3→2→1)
- `timeDifference` вместо `elapsed`
- `late = 50` — компенсация задержки таймера
- `restoreAfterCountdown()` возвращает Стоп или Паузу
- Paused case проверяет `stateBeforeCountdown` и `stateBeforeSection`
- `processMidiMessage` — убран `onStateChanged()` (вызывался на каждую MIDI-ноту)

---

## v1.1.14 — 2026-06-28

**sectionJumped + шрифт статусбара.**

- `sectionJumped` — при Stop→Play уважает выбранную секцию
- `setupPreLines()` чистит верхние слоты
- `g.setFont(FontOptions(...))` — нет утечки шрифта из paintCountdown
- TimerScroll останавливается после очистки fastImage
- preLinesCallback не стопает TimerScroll

---

## v1.1.15 — 2026-06-28

**Финал сессии.**

- `late` добавлен в Countdown
- `SUMMARY.md` — документация текущего состояния
- `HISTORY.md` — этот файл (полная хронология)
- Все известные баги по паузе, отсчёту и смене секции исправлены
