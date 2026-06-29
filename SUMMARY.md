# SoLyP — Summary (v1.1.15)

## Версия
**1.1.15** — единственный источник истины в `CMakeLists.txt`, в коде через `SOLYP_VERSION`.

---

## Позиционирование

- `lyricsViewArea` — поле класса, чистая область текста: `getLocalBounds().reduced(40, 20)`. Обновляется в `resized()`. Используется везде вместо `bounds`.
- `topLimit` — Y верхней строки в стопе (центровка): `lyricsViewArea.getY() + (height - totalH) / 2`. Используется как порог shift в scrollCallback.
- `topYlastLine` — Y последней строки в стопе: `topLimit + (N - 1) * lineHeight`. Единый порог shift: `slots[N-1].y + lineHeight < topYlastLine`.
- `lyricsViewArea.getY()` (= 20px) — верхняя граница чистой области, используется как `startY` при центровке.
- `lyricsViewArea.getBottom()` — нижняя граница чистой области (высота - 40 + 20).

---

## Скролл (scrollCallback)

- **Playing**: все слоты крутятся 1×, shift при `slots[N-1].y + lineHeight < topYlastLine`
- **Paused (fast)**: только fast-изображение крутится на `boost`×, слоты не трогает
- После shift: `slots[N-1].y = slots[N-2].y + lineHeight` — gap = lineHeight
- `boost = 15` — множитель скорости для fast-фазы и сообщения паузы (вместо `5.0`)

---

## Пауза

1. **fast-изображение** — захват старых слотов как `juce::Image`, очистка этих слотов. Уезжает на `boost`×.
2. **TimerScroll** — живёт пока fastImage не уедет (`stopTimer(TimerScroll)` после `fastImage = {}`)
3. **preLinesCallback** (slow) — крутит pre-строки 1×, один shift (`pauseShiftCount` = 0→1), стоп когда `slots[N-1] <= topYlastLine`
4. **Paused case** — проверяет `stateBeforeCountdown` и `stateBeforeSection` — пропускает init при восстановлении после отсчёта/секции

---

## Отсчёт

- Независимый — не запускает Play
- `countdownPhase--` (3→2→1), `timeDifference` вместо `elapsed`
- `late = 50` — компенсация задержки таймера (вычитается из `countdownPhaseDuration`)
- `restoreAfterCountdown()` возвращает предыдущее состояние (Stopped или Paused)
- `stateBeforeCountdown` — сохраняется перед переходом в Countdown

---

## Смена секции

- `SectionChanged` — новый элемент `TransportState`
- `case SectionChanged` — отдельный кейс в switch, убирает fastImage, останавливает таймеры
- `switchHybrid()` → два вызова: `SectionChanged` → `Playing`
- `stateBeforeSection` / `restoreAfterSection()`
- `sectionJumped` — флаг: если в Стопе нажали секцию → Play не сбрасывает `nextLineIndex`

---

## Сериализация

- `lineSpacingInt` в JSON (int = `lineSpacing * 100`) вместо `lineSpacing` (float с ошибками представления)
- Загрузка: `lineSpacing = lineSpacingInt / 100.0f`; миграция со старого `lineSpacing`
- `fontSize` — `std::round()` при save/load, `std::floor()` при расчёте из visibleLines (не теряет строку)

---

## UI

- `g.setFont(juce::Font(FontOptions(14.0f)))` вместо `g.setFont(14.0f)` — нет утечки шрифта из `paintCountdown`
- `status.countdown` в статус-баре показывает "Обратный отсчёт"

---

## MIDI

- `processMidiMessage` — убран `onStateChanged()` (вызывался на каждую MIDI-ноту)
- `switch*` функции сами вызывают `onStateChanged` при смене состояния

---

## Мёртвый код (удалён)

- `preScroll` — был накопитель доли строки, заменён Y-условием
- `startY` — заменён на `topLimit`
- `maxAvailableHeight` — заменён на `(float)lyricsViewArea.getHeight()`
- `entryY` — переименован в `topYlastLine`
- `bounds` — переименован в `lyricsViewArea`
- Старые section target handlers после switch — заменены `case SectionChanged`

---

## OpenCode (с 2026-06-29)

### AGENTS.md
- Сокращён до 49 строк (было 151). Убрано правило «код меняем только если без этого не получить результат».
- Оставлено: русский язык, нейминг (переменные/файлы/пути), списки >3 → JSON.

### Skills (`.opencode/skills/`)
Доступно 11 скиллов — загружаются AI по ситуации:
- `cpp-api-first` — реализовать API как описал пользователь, не усложнять
- `cpp-encoding` — UTF-8, MSVC /utf-8, JUCE String
- `cpp-single-source` — Theme.h, SettingsManager, пути, ресурсы
- `cpp-versioning` — версия только в CMakeLists.txt
- `cpp-autosave` — SettingsManager::save/load
- `cpp-localization` — многоязычность, склонения
- `cpp-file-sizes` — ≤350 строк, дробление
- `cpp-github` — пуш, каждые 5 файлов, релизы
- `cpp-project-structure` — порядок описания папок
- `juce-transport-state` — TransportState enum, switch, без флагов
- `juce-timer-separation` — один таймер — одна ответственность

### Ресурсы
- `resources/themes/` — темы/скины (было `Documents/SoLyP/themes/`)
- `resources/langs/` — локали (было `langs/`)
- Редактировать только `resources/`. При установке копируются в `Documents/SoLyP/`.
- Обновлены CMakeLists.txt и I18n.cpp.
