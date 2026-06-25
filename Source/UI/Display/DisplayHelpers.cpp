#include "../../Display.h"

#include "../Theme.h"
#include "Data/SettingsManager.h"
#include "Data/I18n.h"

// путь к папке songs в Документах
juce::String getDefaultSongDir()
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("SoLyP").getChildFile("songs").getFullPathName();
}

// собирает текст для редактора из textSong[] (плоский → [секция]\nстроки)
juce::String songToText(const Song& song)
{
    juce::String result;
    int currentSid = -1;
    for (auto& tl : song.textSong)
    {
        if (tl.sectionId != currentSid)
        {
            if (currentSid >= 0) result += "\n";
            currentSid = tl.sectionId;
            result += "[" + tl.sectionName + "]\n";
        }
        result += tl.text + "\n";
    }
    return result.trimEnd();
}

// ищет первую строку в displayLines[] с заданным sectionId, возвращает индекс или -1
int findSectionStart(const Song& song, int sectionId) {
    for (int i = 0; i < song.displayLines.size(); ++i)
        if (song.displayLines[i].sectionId == sectionId)
            return i;
    return -1;
}

// ── инициализация ──────────────────────────────────────────────────────────

// инициализация слотов при загрузке песни: создаёт массив, заполняет центрированными строками
void SoLyPAudioProcessorEditor::initSlots() {
    const auto& song = processor.getCurrentSong();
    if (song.displayLines.isEmpty()) { slots.clear(); return; }

    int N = SettingsManager::visibleLines;
    auto bounds = getLocalBounds().reduced(40, 20);
    float lineHeight = getRealLineHeight(SettingsManager::fontSize);

    slots.resize(N);
    nextLineIndex = 0;
    useCenterY = true;

    int linesAvail = song.displayLines.size() - nextLineIndex;
    int linesToShow = std::min(N, linesAvail);
    float totalH = linesToShow * lineHeight;
    float startY = bounds.getY() + (bounds.getHeight() - totalH) / 2.0f;
    for (int i = 0; i < N; ++i) {
        int idx = nextLineIndex + i;
        if (i < linesToShow && idx < song.displayLines.size()) {
            slots[i].text = song.displayLines[idx].text;
            slots[i].y = startY + i * lineHeight;
        } else {
            slots[i].text = {};
            slots[i].y = startY + linesToShow * lineHeight;
        }
    }

    if (processor.getTransportState() != SoLyPAudioProcessor::TransportState::Stopped) {
        useCenterY = false;
        nextLineIndex = std::min(SettingsManager::preLinesOnPause, N);
    }
    lastScrollTime = juce::Time::getMillisecondCounterHiRes();
}

// подготовка предварительно показанных строк: нижние pre слотов заполняются текстом, верхние — пусто
void SoLyPAudioProcessorEditor::setupPreLines() {
    const auto& song = processor.getCurrentSong();
    int N = (int)slots.size();
    if (N <= 0 || song.displayLines.isEmpty()) return;

    auto bounds = getLocalBounds().reduced(40, 20);
    float lineHeight = getRealLineHeight(SettingsManager::fontSize);

    useCenterY = false;
    int pre = std::min(SettingsManager::preLinesOnPause, N);
    for (int i = 0; i < N; ++i) {
        int idx = nextLineIndex + i - (N - pre);
        if (idx >= 0 && idx < song.displayLines.size())
            slots[i].text = song.displayLines[idx].text;
        else
            slots[i].text = {};
        slots[i].y = bounds.getBottom() - (float)(N - i) * lineHeight;
    }
}

// скорость скролла: время прохода одной строки через окно (мс) — зависит от BPM и parts
double SoLyPAudioProcessorEditor::getTimePerLine() {
    const auto& song = processor.getCurrentSong();
    if (song.displayLines.isEmpty()) return -1.0;
    double bpm = SettingsManager::manualBpmEnabled
        ? (double)SettingsManager::manualBpmValue
        : processor.getCurrentBpm();
    if (bpm <= 0.0) bpm = 120.0;
    int idx = nextLineIndex - SettingsManager::preLinesOnPause;
    if (idx < 0) idx = 0;
    if (idx >= song.displayLines.size()) return -1.0;
    int parts = song.displayLines[idx].parts;
    if (parts <= 0) parts = 1;
    double barDuration = 60.0 / bpm * (double)SettingsManager::timeSignature;
    if (barDuration <= 0.0) return -1.0;
    return barDuration * (double)SettingsManager::barsPerLine / (double)parts;
}

// ── отрисовка ──────────────────────────────────────────────────────────────

// проверка перед отрисовкой: пересобрать displayLines если изменился шрифт/ширина
void SoLyPAudioProcessorEditor::rebuildDisplayLines()
{
    const auto& song = processor.getCurrentSong();
    auto bounds = getLocalBounds().reduced(40, 20);
    float availW = (float)bounds.getWidth();
    auto state = processor.getTransportState();

    bool needRebuild = song.displayLines.isEmpty()
        || song.lastBuildWidth != availW
        || song.lastBuildFontSize != SettingsManager::fontSize;
    if (needRebuild && state != SoLyPAudioProcessor::TransportState::Playing)
        processor.getCurrentSong().rebuildDisplayLines(availW, SettingsManager::fontSize);
}

// приветственный экран: "Song Lyrics Prompter", "(by MM)", версия, "Загрузите песню..."
void SoLyPAudioProcessorEditor::initView(juce::Graphics& g)
{
    auto area = getLocalBounds();
    g.setColour(Theme::textActiveLine);
    g.setFont(juce::FontOptions(28.0f));
    g.drawText("Song Lyrics Prompter",
               juce::Rectangle<int>(0, area.getY() + area.getHeight() / 2 - 80, area.getWidth(), 40),
               juce::Justification::centred, false);
    g.setColour(Theme::textHint);
    g.setFont(juce::FontOptions(20.0f));
    g.drawText("(by MM)",
               juce::Rectangle<int>(0, area.getY() + area.getHeight() / 2 - 45, area.getWidth(), 30),
               juce::Justification::centred, false);
    g.setFont(juce::FontOptions(16.0f));
    g.drawText("v." + String(SOLYP_VERSION),
               juce::Rectangle<int>(0, area.getY() + area.getHeight() / 2 - 20, area.getWidth(), 25),
               juce::Justification::centred, false);
    g.setFont(juce::FontOptions(24.0f));
    g.drawText(I18n::get("empty.title"),
               juce::Rectangle<int>(0, area.getY() + area.getHeight() / 2 + 25, area.getWidth(), 35),
               juce::Justification::centred, false);
}

// статическая отрисовка после загрузки: слоты по центру, без анимации
void SoLyPAudioProcessorEditor::initPaint(juce::Graphics& g)
{
    const auto& song = processor.getCurrentSong();
    if (song.textSong.isEmpty()) { initView(g); return; }
    if (slots.empty()) return;

    auto bounds = getLocalBounds().reduced(40, 20);
    float lineHeight = getRealLineHeight(SettingsManager::fontSize);
    int N = (int)slots.size();

    for (int i = 0; i < N; ++i)
    {
        if (slots[i].text.isEmpty()) continue;
        float y = (float)slots[i].y;
        if (y + lineHeight < (float)bounds.getY() || y > (float)bounds.getBottom()) continue;
        g.setColour(Theme::textOnButton);
        g.setFont(juce::FontOptions(SettingsManager::fontSize));
        g.drawText(slots[i].text,
            juce::Rectangle<float>((float)bounds.getX(), y, (float)bounds.getWidth(), lineHeight),
            juce::Justification::centred, false);
    }
}

// отрисовка скролла (Play/Pause/Countdown/Stop): Y из slots[i].y
void SoLyPAudioProcessorEditor::paintScroll(juce::Graphics& g)
{
    const auto& song = processor.getCurrentSong();
    if (song.textSong.isEmpty()) { initView(g); return; }
    if (slots.empty()) return;

    auto bounds = getLocalBounds().reduced(40, 20);
    float lineHeight = getRealLineHeight(SettingsManager::fontSize);
    int N = (int)slots.size();

    for (int i = 0; i < N; ++i)
    {
        if (slots[i].text.isEmpty()) continue;

        float y = (float)slots[i].y;
        if (y + lineHeight < (float)bounds.getY() || y > (float)bounds.getBottom()) continue;

        g.setColour(Theme::textOnButton);
        g.setFont(juce::FontOptions(SettingsManager::fontSize));
        g.drawText(slots[i].text,
            juce::Rectangle<float>((float)bounds.getX(), y, (float)bounds.getWidth(), lineHeight),
            juce::Justification::centred, false);
    }
}

// оверлей паузы: сообщение — движется снизу вверх, замирает наверху
// поддерживает 1-2 строки, шрифт авто-подбирается по ширине окна
void SoLyPAudioProcessorEditor::paintPauseText(juce::Graphics& g)
{
    auto state = processor.getTransportState();
    if (state != SoLyPAudioProcessor::TransportState::Paused || !showPauseText) return;

    // получить текст сообщения (из настроек или язык по умолчанию)
    juce::String msgText = SettingsManager::pauseText.isNotEmpty()
        ? SettingsManager::pauseText
        : I18n::get("pause.text");
    if (msgText.isEmpty()) return;

    auto lines = juce::StringArray::fromLines(msgText);
    if (lines.isEmpty()) return;

    auto bounds = getLocalBounds().reduced(40, 20);
    float availW = (float)bounds.getWidth() - 40.0f; // gap 20px с каждой стороны

    // авто-подбор размера шрифта: вписать самую широкую строку в availW
    float refSize = 14.0f;
    juce::Font refFont{juce::FontOptions(refSize)};
    float maxLineW = 0.0f;
    for (auto& ln : lines)
    {
        float w = refFont.getStringWidthFloat(ln);
        if (w > maxLineW) maxLineW = w;
    }

    float pauseFontSize = refSize;
    if (maxLineW > 0.0f)
        pauseFontSize = refSize * (availW / maxLineW);
    pauseFontSize = juce::jlimit(14.0f, 72.0f, pauseFontSize);

    float lineHeight = getRealLineHeight(pauseFontSize);
    float totalH = (float)lines.size() * lineHeight;

    float pauseY = (float)(bounds.getBottom() + pauseMsgY);
    if (pauseY < (float)bounds.getY()) pauseY = (float)bounds.getY();
    if (pauseY + totalH > (float)bounds.getBottom()) return;

    g.setColour(Theme::textPause);
    g.setFont(juce::FontOptions(pauseFontSize));

    for (int i = 0; i < lines.size(); ++i)
    {
        float y = pauseY + (float)i * lineHeight;
        g.drawText(lines[i],
            juce::Rectangle<float>((float)bounds.getX(), y, (float)bounds.getWidth(), lineHeight),
            juce::Justification::centred, false);
    }
}

// статус-бар: название песни, секция, состояние, MIDI-нота, версия
void SoLyPAudioProcessorEditor::paintStatusBar(juce::Graphics& g)
{
    const auto& song = processor.getCurrentSong();
    auto state = processor.getTransportState();

    int cs = 0;
    juce::String secName;
    if (nextLineIndex > 0 && nextLineIndex - 1 < song.displayLines.size())
    {
        const auto& dl = song.displayLines[nextLineIndex - 1];
        cs = dl.sectionId;
        secName = dl.sectionName;
    }

    g.setColour(Theme::textStatusBar);
    g.setFont(14.0f);

    juce::String leftInfo;
    if (song.fileTitle.isNotEmpty())
        leftInfo = song.fileTitle + "  |  ";

    if (cs >= 0)
    {
        leftInfo += I18n::get("status.section") + " " + secName;

        // BPM
        double bpm = SettingsManager::manualBpmEnabled
            ? (double)SettingsManager::manualBpmValue
            : processor.getCurrentBpm();
        if (bpm > 0.0)
            leftInfo += "  |  " + juce::String((int)bpm) + " bpm";

        leftInfo += "  |  " + (state == SoLyPAudioProcessor::TransportState::Playing
                ? I18n::get("status.playing")
                : state == SoLyPAudioProcessor::TransportState::Paused
                ? I18n::get("status.paused")
                : state == SoLyPAudioProcessor::TransportState::Countdown
                ? I18n::get("status.countdown")
                : I18n::get("status.stop"));
    }

    auto note = processor.getLastMidiNote();
    if (note >= 0)
    {
        auto elapsed = juce::Time::getMillisecondCounterHiRes() - processor.getLastMidiNoteTime();
        if (elapsed < 2000.0)
        {
            static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
            int octave = (note / 12) - 1 - SettingsManager::octaveSystem;
            leftInfo += "  |  " + juce::String(noteNames[note % 12]) + juce::String(octave) + " (" + juce::String(note) + ")";
        }
    }

    g.drawText(leftInfo, getLocalBounds().reduced(10, 5), juce::Justification::bottomLeft);
    g.drawText("v" + juce::String(SOLYP_VERSION), getLocalBounds().reduced(10, 5), juce::Justification::bottomRight);
}

// оверлей обратного отсчёта: цифра 3-2-1 + прогресс-бар на ширину текста
void SoLyPAudioProcessorEditor::paintCountdown(juce::Graphics& g)
{
    if (countdownPhase <= 0 || countdownPhase > 3) return;

    double now = juce::Time::getMillisecondCounterHiRes();
    double totalElapsed = (countdownPhase - 1) * countdownPhaseDuration
        + (now - countdownPhaseStart);
    double totalDuration = countdownPhaseDuration * 3.0;
    double progress = juce::jlimit(0.0, 1.0, totalElapsed / totalDuration);

    auto area = getLocalBounds();
    int textY = area.getCentreY() - 50;

    g.setColour(Theme::textPause);
    g.setFont(juce::FontOptions(72.0f, juce::Font::bold));
    g.drawText(juce::String(4 - countdownPhase),
        area.withY(textY).withHeight(80), juce::Justification::centred);

    int barW = getWidth() - 80;
    int barX = 40;
    int barY = textY + 90;
    int barH = 8;

    g.setColour(Theme::textPause.withAlpha(0.3f));
    g.fillRect((float)barX, (float)barY, (float)barW, (float)barH);
    g.setColour(Theme::textPause);
    g.fillRect((float)barX, (float)barY, (float)(progress * barW), (float)barH);
}

// оверлей ошибки: сообщение внизу экрана
void SoLyPAudioProcessorEditor::paintError(juce::Graphics& g)
{
    g.setColour(Theme::textError);
    g.setFont(18.0f);
    g.drawText(lastError, getLocalBounds().removeFromBottom(60), juce::Justification::centred);
}

// сохранение: текст редактора → Song::fromText → toJson → файл
void SoLyPAudioProcessorEditor::doSave(const juce::String& name, const juce::String& title)
{
    auto text = textEditor->getText();
    Song song = Song::fromText(text);
    if (!song.validationError.isEmpty()) { lastError = song.validationError; repaint(); return; }

    auto filename = name;
    if (filename.isEmpty()) filename = "untitled";
    if (!filename.endsWith(".json")) filename += ".json";
    lastFilename = filename;
    lastSongTitle = title;
    song.fileTitle = title;

    auto jsonText = song.toJson();
    if (title.isNotEmpty())
    {
        auto parsed = juce::JSON::parse(jsonText);
        if (auto* obj = parsed.getDynamicObject())
        {
            obj->setProperty("title", title);
            jsonText = juce::JSON::toString(parsed, false);
        }
    }

    auto dir = juce::File(getDefaultSongDir());
    dir.createDirectory();
    auto file = dir.getChildFile(filename);

    if (file.existsAsFile())
    {
        juce::NativeMessageBox::showOkCancelBox(
            juce::MessageBoxIconType::WarningIcon,
            I18n::get("overwrite.title"),
            I18n::get("overwrite.message").replace("{name}", filename),
            this,
            juce::ModalCallbackFunction::create([this, file, jsonText, song](int result) mutable
            {
                if (result != 1) return;
                file.replaceWithText(jsonText, false, false, "\n");
                processor.loadSong(song);
                textModified = false;
                pendingChanges = false;
                exitEditMode();
            }));
        return;
    }

    file.replaceWithText(jsonText, false, false, "\n");
    processor.loadSong(song);
    textModified = false;
    pendingChanges = false;
    exitEditMode();
}

// диалог загрузки песни из .json
void SoLyPAudioProcessorEditor::loadSongFromFile()
{
    auto dir = juce::File(getDefaultSongDir());
    dir.createDirectory();

    auto chooser = std::make_shared<juce::FileChooser>(I18n::get("load.title"), dir, "*.json");
    chooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser&)
        {
            auto file = chooser->getResult();
            if (!file.existsAsFile()) return;
            auto jsonText = file.loadFileAsString();
            Song song = Song::fromJson(jsonText);
            { auto b = getLocalBounds().reduced(40, 20); song.rebuildDisplayLines((float)b.getWidth(), SettingsManager::fontSize); }
            if (!song.validationError.isEmpty()) { lastError = song.validationError; repaint(); return; }

            if (textModified)
            {
                auto* alert = new juce::AlertWindow(
                    I18n::get("confirm.unsaved"),
                    I18n::get("confirm.loadFile"),
                    juce::AlertWindow::QuestionIcon);
                alert->setColour(juce::AlertWindow::backgroundColourId, Theme::bgPanel);
                alert->setColour(juce::AlertWindow::textColourId, Theme::textPrimary);
                alert->setColour(juce::AlertWindow::outlineColourId, Theme::accentBorder);
                alert->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
                alert->addButton(juce::String::fromUTF8("Отмена"), 0,
                    juce::KeyPress(juce::KeyPress::escapeKey));
                alert->enterModalState(true,
                    juce::ModalCallbackFunction::create(
                        [this, alert, song, file](int r) mutable {
                            if (r != 1) { delete alert; return; }
                            delete alert;
                            applySongLoad(song, file);
                        }));
            }
            else { applySongLoad(song, file); }
        });
}

// завершение загрузки: заголовок, loadSong, обновить редактор если открыт
void SoLyPAudioProcessorEditor::applySongLoad(Song& song, const juce::File& file)
{
    if (song.fileTitle.isEmpty())
        song.fileTitle = file.getFileNameWithoutExtension();
    lastFilename = file.getFileNameWithoutExtension() + ".json";
    lastSongTitle = song.fileTitle;
    processor.loadSong(song);
    pendingChanges = false;
    if (editMode)
        textEditor->setText(songToText(song), juce::dontSendNotification);
}

// максимальное количество строк, которое влезает в окно при заданном шрифте
int calcFittingLines(int height, float fontSize, const Song& song)
{
    float lineHeight = getRealLineHeight(fontSize);
    int maxTheoretical = static_cast<int>((height - 20) / lineHeight);
    int actualLines = song.displayLines.size();
    return juce::jlimit(2, juce::jmin(20, maxTheoretical), actualLines);
}
